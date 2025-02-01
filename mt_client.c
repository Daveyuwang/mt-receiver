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

/**
 * Client thread function:
 * - Each thread creates a TCP socket.
 * - It then connects to the server specified by SERVER_IP and SERVER_PORT.
 * - Once connected, the thread continuously calls recv() to receive data
 *   from the server and prints the received data.
 */
void *client_receive_thread(void *arg){
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    // Create a socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to create socket");
        pthread_exit(NULL);
    }

    // Configure the server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0){
        perror("Invalid address or address not supported");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Connect to the server
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Connection failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    printf("[CLIENT] thread %ld connected to server %s:%d\n", pthread_self(), SERVER_IP, SERVER_PORT);


    // Receive data from the server in a loop
    while((n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0){
        buffer[n] = '\0';
        printf("[CLIENT] thread %ld received: %s\n", pthread_self(), buffer);
    }
    if(n < 0){
        perror("Failed to receive data");
    }else{
        printf("[CLIENT] thread %ld: Connection closed by server\n", pthread_self());
    }

    // Cleanup
    close(sockfd);
    pthread_exit(NULL);
}

/**
 * Main function:
 * - Creates NUM_CLIENT_THREADS threads.
 * - Each thread is assigned to the client_receive_thread function.
 * - Waits for all threads to finish before exiting.
 */
int main(int argc, char *argv[]){
    pthread_t threads[NUM_CLIENT_THREADS];

    // Create multiple threads to simulate concurrent receiving
    for(int i = 0; i < NUM_CLIENT_THREADS; i++){
        if(pthread_create(&threads[i], NULL, client_receive_thread, NULL) != 0){
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
        // optionally detach the thread so that resources are freed automatically
        pthread_detach(threads[i]);
    }

    // keep the main thread alive (or wait for user input) to let client threads work
    while(1){
        sleep(1);
    }

    return 0;
}