/**
 * test_sender.c
 * Test data sender for mt_server and mt_client testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define TEST_DATA_SIZE 100

int main(){
    int sockfd;
    struct sockaddr_in server_addr;
    char test_data[TEST_DATA_SIZE];
    int counter = 0;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Connect to server
    if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("Failed to connect to server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[TEST_SENDER] Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // send test data in a loop
    while(1){
        snprintf(test_data, TEST_DATA_SIZE, "Test message #%d from sender", counter++);
        
        // send data
        if(send(sockfd, test_data, strlen(test_data), 0) < 0){
            perror("Failed to send data");
            break;
        }

        printf("[TEST_SENDER] Sent: %s\n", test_data);
        sleep(1);
    }

    close(sockfd);
    return 0;

}