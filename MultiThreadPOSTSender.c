#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS 10 // Number of threads to use
#define MAX_REQUESTS 0 // Set to 0 for infinite requests

pthread_mutex_t lock;
int total_request_count = 0;
time_t start_time = 0;

void *send_requests(void *arg) {
    char *ip_address = "172.16.10.243"; // IP address to send the request to
    int port = 4080; // Port to send the request to
    char *path = "/api"; // The path to the resource to request
    char *data = ""; // The data to include in the request

    int request_count = 0;
    while (MAX_REQUESTS == 0 || request_count < MAX_REQUESTS) {
        // Create a socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            exit(1);
        }

        // Create a server address structure
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip_address);
        serv_addr.sin_port = htons(port);

        // Connect to the server
        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("connect");
            exit(1);
        }

        // Build the request message
        char request[1024];
        snprintf(request, sizeof(request), "POST %s HTTP/1.1\r\n"
                                             "Host: %s:%d\r\n"
                                             "Content-Type: application/x-www-form-urlencoded\r\n"
                                             "Content-Length: %d\r\n"
                                             "\r\n"
                                             "%s",
                 path, ip_address, port, strlen(data), data);

        // Send the request
        int bytes_sent = send(sockfd, request, strlen(request), 0);
        if (bytes_sent < 0) {
            perror("send");
            exit(1);
        }

        // Close the socket
        close(sockfd);

        // Update the total request count
        pthread_mutex_lock(&lock);
        total_request_count++;
        pthread_mutex_unlock(&lock);

        request_count++;
    }

    pthread_exit(NULL);
}

void *calculate_requests_per_second(void *arg) {
    while (1) {
        sleep(1);
        pthread_mutex_lock(&lock);
        time_t elapsed_time = time(NULL) - start_time;
        float requests_per_second = (float) total_request_count / (float) elapsed_time;
        printf("Requests per second: %.2f\n", requests_per_second);
        pthread_mutex_unlock(&lock);
    }
}

int main(int argc, char *argv[]) {
    pthread_t threads[NUM_THREADS];
    int rc;

    // Create the mutex lock
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("ERROR: mutex init failed\n");
        exit(1);
    }

    // Start the request threads
    for (int i = 0; i < NUM_THREADS; i++) {
        rc = pthread_create(&threads[i], NULL, send_requests, NULL);
        if (rc) {
            printf("ERROR: return code from pthread_create() is %%d\n", rc);
exit(1);
}
}
// Start the thread that calculates requests per second
rc = pthread_create(&threads[NUM_THREADS], NULL, calculate_requests_per_second, NULL);
if (rc) {
    printf("ERROR: return code from pthread_create() is %d\n", rc);
    exit(1);
}

// Record the start time
start_time = time(NULL);

// Wait for all threads to complete
for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
}

// Destroy the mutex lock
pthread_mutex_destroy(&lock);

return 0;
}