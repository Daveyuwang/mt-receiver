/**
 * mt_client.c - multi-threaded TCP client implementation
 * 
 * This implementation demonstrates a simple multi-threaded TCP client that creates
 * multiple concurrent connections to a server. It uses a thread pool pattern
 * where each thread maintains its own connection to receive data independently.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"   // Server IP address to connect to
#define SERVER_PORT 8080        // Server port number
#define BUFFER_SIZE 1024        // Size of the buffer for receiving data
#define NUM_CLIENT_THREADS 4    // Number of client threads to create

// structure to pass socket information to the thread
typedef struct {
    int sockfd;
    int thread_id;
} ThreadArgs;

/**
 * Send thread function:
 * - Sends a sequence of messages to the server.
 * - Each message is sent with a delay of 1 second between them.
 * - If the send operation fails, it breaks the loop and exits.
 */
void *client_send_thread(void *arg){
    ThreadArgs *args = (ThreadArgs *)arg;
    int sockfd = args->sockfd;
    int thread_id = args->thread_id;
    char message[BUFFER_SIZE];
    int counter = 0;

    while(1){
        snprintf(message, BUFFER_SIZE, "Client %d message #%d", thread_id, counter++);
        if(send(sockfd, message, strlen(message), 0) < 0){
            printf("[CLIENT] thread %d: Failed to send data\n", thread_id);
            break;
        }
        printf("[CLIENT] thread %d: Sent message #%d\n", thread_id, counter);
        sleep(1);
    }
    free(arg);
    pthread_exit(NULL);
    return NULL;
}

/**
 * Client thread function:
 * - Each thread creates a TCP socket.
 * - It then connects to the server specified by SERVER_IP and SERVER_PORT.
 * - Once connected, the thread continuously calls recv() to receive data
 *   from the server and prints the received data.
 */
void *client_thread(void *arg){
    int thread_id = *(int *)arg;    // get the thread id
    free(arg);                      // free the argument

    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    // Create socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("[CLIENT] thread %d: Failed to create socket\n", thread_id);
        return NULL;
    }

    // Configure server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0){
        printf("[CLIENT] thread %d: Invalid address\n", thread_id);
        close(sockfd);
        return NULL;
    }

    // Connect to server
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("[CLIENT] thread %d: Failed to connect to server\n", thread_id);
        close(sockfd);
        return NULL;
    }

    printf("[CLIENT] thread %d: Connected to server\n", thread_id);

    // Create send thread
    pthread_t send_thread;
    ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
    args->sockfd = sockfd;
    args->thread_id = thread_id;

    if(pthread_create(&send_thread, NULL, client_send_thread, args) != 0){
        printf("[CLIENT] thread %d: Failed to create send thread\n", thread_id);
        free(args);
        close(sockfd);
        return NULL;
    }
    pthread_detach(send_thread);

    // Receive data from the server
    while(1){
        while((n = recv(sockfd, buffer, BUFFER_SIZE-1, 0)) > 0){
            buffer[n] = '\0';
            printf("[CLIENT] thread %d: Received %zd bytes: %s\n", thread_id, n, buffer);
        }
        if(n < 0){
            printf("[CLIENT] thread %d: Failed to receive data\n", thread_id);
        }else{
            printf("[CLIENT] thread %d: Connection closed by server\n", thread_id);
        }
    }
    close(sockfd);
    return NULL;
}
/**
 * Main function:
 * - Creates NUM_CLIENT_THREADS threads.
 * - Each thread is assigned to the client_receive_thread function.
 * - Waits for all threads to finish before exiting.
 */
int main(int argc, char *argv[]){
    pthread_t threads[NUM_CLIENT_THREADS];

    // Create client threads
    for(int i = 0; i < NUM_CLIENT_THREADS; i++){
        int *thread_id = (int *)malloc(sizeof(int));
        *thread_id = i;
        if(pthread_create(&threads[i], NULL, client_thread, thread_id) != 0){
            printf("Failed to create thread %d\n", i);
            free(thread_id);
            continue;
        }
        pthread_detach(threads[i]);
    }

    // keep the main thread alive (or wait for user input) to let client threads work
    while(1){
        sleep(1);
    }
    return 0;
}