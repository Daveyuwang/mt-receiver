/**
 * mt_server.c
 * Multi-threaded server implementation
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define PORT 8080   // server listens on this port
#define BACKLOG 10 // maximum number of pending connections
#define BUFFER_SIZE 1024 // buffer size for receiving data
#define NUM_WORKER_THREADS 4 // number of worker threads
#define MAX_CLIENTS 100 // maximum number of clients


// define a connection node structure for queue
typedef struct conn_node{
    int connfd;
    struct conn_node *next;
} conn_node_t;

// define a queue structure for connection nodes
typedef struct{
    conn_node_t *head;
    conn_node_t *tail;
    pthread_mutex_t mutex;  // mutex to protect queue operations
    pthread_cond_t cond;    // condition variable to signal available data
} conn_queue_t;

// structure to manage client connections
typedef struct{
    int *client_fds;    // dynamic array of client file descriptors
    int count;          // current connection count
    pthread_mutex_t mutex; // mutex to protect connection list
} client_manager_t;

client_manager_t clients;   // global client manager
conn_queue_t queue; // global queue for connections

// initialize the client manager
void init_client_manager(client_manager_t *cm){
    cm->client_fds = malloc(MAX_CLIENTS * sizeof(int));
    if(cm->client_fds == NULL){
        perror("Failed to allocate memory for client file descriptors");
        exit(EXIT_FAILURE);
    }
    cm->count = 0;
    pthread_mutex_init(&cm->mutex, NULL);
}

/**
 * Add a client to the client manager
 * @param cm: pointer to the client manager
 * @param fd: the client file descriptor to add
 */
void add_client(client_manager_t *cm, int fd){
    pthread_mutex_lock(&cm->mutex);
    if(cm->count < MAX_CLIENTS){
        cm->client_fds[cm->count++] = fd;
        printf("New client added. Total clients: %d\n", cm->count);
    }else{
        printf("Warning: Maximum clients reached, connection rejected\n");
    }
    pthread_mutex_unlock(&cm->mutex);
}

/**
 * Remove a client from the client manager
 * @param cm: pointer to the client manager
 * @param fd: the client file descriptor to remove
 */
void remove_client(client_manager_t *cm, int fd){
    pthread_mutex_lock(&cm->mutex);
    for(int i = 0; i < cm->count; i++){
        if(cm->client_fds[i] == fd){
            cm->client_fds[i] = cm->client_fds[cm->count - 1];
            cm->count--;
            printf("Client %d removed. Total clients: %d\n", fd, cm->count);
            break;
        }
    }
    pthread_mutex_unlock(&cm->mutex);
}

/**
 * Broadcast message to all connected clients
 * @param cm: pointer to the client manager
 * @param data: the message to broadcast
 * @param len: the length of the message
 */
void broadcast_to_clients(client_manager_t *cm, const char *data, size_t len){
    pthread_mutex_lock(&cm->mutex);
    for(int i = 0; i < cm->count; i++){
        if(send(cm->client_fds[i], data, len, 0) < 0){
            perror("Failed to send data to client");
        }
    }
    pthread_mutex_unlock(&cm->mutex);
}

/**
 * Initialize the connection queue
 * @param q: pointer to the queue
 */
void init_queue(conn_queue_t *q){
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

/**
 * Enqueue a connection file descriptor into the queue
 * @param q: pointer to the queue
 * @param connfd: the connection file descriptor to enqueue
 */
void enqueue(conn_queue_t *q, int connfd){
    conn_node_t *node = (conn_node_t *)malloc(sizeof(conn_node_t));
    if(!node){
        perror("Failed to allocate memory for connection node");
        return;
    }
    node->connfd = connfd;
    node->next = NULL;

    pthread_mutex_lock(&q->mutex);
    if(q->tail==NULL){  // if queue is empty
        q->head = q->tail = node; // set head and tail to the new node
    } else {
        q->tail->next = node; // add the new node to the end of the queue
        q->tail = node; // update the tail to the new node
    }
    
    // signal one waiting worker thread that a new connection is available
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

/**
 * Dequeue a connection file descriptor from the queue
 * @param q: pointer to the queue
 */
int dequeue(conn_queue_t *q){
    pthread_mutex_lock(&q->mutex);
    while(q->head==NULL){   // if queue is empty, wait for a new connection
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    conn_node_t *node = q->head; // get the head node
    int connfd = node->connfd; // get the connection file descriptor
    q->head = node->next; // remove the head node from the queue
    if(q->head==NULL){
        q->tail = NULL; // if the queue is empty, set the tail to NULL
    }
    free(node); // free the memory allocated for the node
    pthread_mutex_unlock(&q->mutex);
    return connfd; // return the connection file descriptor
}

/**
 * Sender thread function: continuously send test messages to the client
 * @param arg: pointer to the thread argument (unused)
 */
void* sender_thread(void* arg){
    int connfd = *(int*)arg;
    free(arg);
    char test_message[BUFFER_SIZE];
    for(int i = 0; i < BUFFER_SIZE; i++){
        test_message[i] = 'A' + (i % 26);
    }
    int counter = 0;
    while(1){
        snprintf(test_message, BUFFER_SIZE, "Server test message #%d", counter++);

        // send the test message
        if(send(connfd, test_message, strlen(test_message), 0) < 0){
            perror("Failed to send data");
            break;
        }
        printf("[SERVER] Sent to client %d: %s\n", connfd, test_message);
        sleep(1);
    }
    close(connfd);
    return NULL;
}

/**
 * Worker thread function: continuously dequeue a connection and process data
 * @param arg: pointer to the thread argument (unused)
 */
void *worker_thread(void *arg){
    (void)arg;
    char buffer[BUFFER_SIZE];
    ssize_t n;
    while(1){
        // get a connection from the shared queue (blocking if none available)
        int connfd = dequeue(&queue);
        printf("[SERVER]Worker thread processing connection %d\n", connfd);

        // create a sender thread for this connection
        pthread_t send_thread;
        int *conn_arg = malloc(sizeof(int));
        *conn_arg = connfd;
        if(pthread_create(&send_thread, NULL, sender_thread, conn_arg) != 0){
            perror("Failed to create sender thread");
            free(conn_arg);
        }else{
            pthread_detach(send_thread); // detach the sender thread
        }

        // Process the data from the connection
        while((n = recv(connfd, buffer, BUFFER_SIZE - 1, 0)) > 0){
            // // read data from the connection
            buffer[n] = '\0'; // null-terminate the buffer
            printf("[SERVER] Received %zd bytes from connection %d: %s\n", n, connfd, buffer);
    }
    if(n < 0){
        perror("Failed to receive data from connection");
    }else{
        printf("[SERVER] Connection %d closed by client\n", connfd);
        }
        close(connfd); // close the connection
    }
    return NULL;
}


/**
 * Main function: create the listening socket,
 * initialize the connection queue and thread pool,
 * accept connections and enqueue them for processing
 */
int main(){
    int opt = 1;
    int sockfd, connfd; // socket file descriptors
    // set socket options to reuse address and port
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    struct sockaddr_in serv_addr, cli_addr; // server and client addresses
    socklen_t cli_len = sizeof(cli_addr);  // client address length

    // initialize the connection queue
    init_queue(&queue);

    // create a TCP socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Configure the server address (bind to all local IP addresses)
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Failed to bind socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Set the socket to listen for incoming connections
    if(listen(sockfd, BACKLOG) < 0){
        perror("Failed to listen for connections");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Create a pool of worker threads for concurrent processing
    pthread_t workers[NUM_WORKER_THREADS];
    for(int i = 0; i < NUM_WORKER_THREADS; i++){
        if(pthread_create(&workers[i], NULL, worker_thread, NULL) != 0){
            perror("pthread_create failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        pthread_detach(workers[i]); // detach threads for independent cleanup
    }

    printf("Server is listening on port %d...\n", PORT);
    
    // Main loop: accept incoming connections and enqueue them for processing
    while(1){
        // accept a new connection
        connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
        if(connfd < 0){
            perror("Failed to accept connection");
            continue;
        }
        printf("Accepted connection from %s:%d\n", 
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        // enqueue the connection for processing
        enqueue(&queue, connfd);
    }

    // cleanup
    close(sockfd);
    for(int i = 0; i < NUM_WORKER_THREADS; i++){
        pthread_join(workers[i], NULL);
    }

    return 0;
}

