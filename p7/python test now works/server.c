#include "helper.h"
#include "request.h"
#include <pthread.h>

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

pthread_mutex_t lock;  // lock used to in consumer/producer
pthread_cond_t cons;  // condition variable for consumers (workers)
pthread_cond_t prod;  // condition variable for producer
int num_full;  // tracks number of full buffer entries
int num_empty;  // tracks number of empty buffer entries
int buffers;  // number of buffers
int listenfd;

void sigint_handler(int sig) {
    printf("Captured signal SIGINT %d\n", sig);
    Close(listenfd);
    exit(0);
}

/**
 * Function that gets arguments from the command line.
 *
 * port: pointer to the port number to be used by the server
 * threads: pointer to the number of worker threads to be used by the server
 * buffers: pointer to the size of the input request buffer to be used
 * shm_name: pointer to the name of the shared memory address zone
 * argc: number of arguments
 * argv: array of command line arguments
 */
void getargs(int *port, int *threads, int *buffers, char **shm_name, int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <port> <threads> <buffers> <shm_name>\n", argv[0]);
        exit(1);
    }

    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffers = atoi(argv[3]);
    *shm_name = argv[4];
}

/**
 * Function that finds and returns the value of the first full element in the
 * buffer. Also marks that element in progress and records its index
 *
 * buffer: pointer for buffer array
 * index_ptr: pointer to index of found full element
 * Return: the value of the element
 */
int get_full(int *buffer, int *index_ptr) {
    for (int index = 0; index < buffers; index++) {  // loop over buffer
        if (buffer[index] != -2 && buffer[index] != -1) {  // if not free or in progress
            *index_ptr = index;  // record index
            int temp = buffer[index];  // get temp
            buffer[index] = -1;  // mark in progress
            num_full--;  // decrease number full
            return temp;  // return temp
        }
    }

    return -1; // logically should never be reached
}

/**
 * Function thatreturns the value of the first free element in the
 * buffer. Also marks that element in progress.
 *
 * buffer: pointer for buffer array
 * Return: index of free element
 */
int get_empty(int *buffer) {
    for (int index = 0; index < buffers; index++) {  // loop over buffer
        if (buffer[index] == -2) {  // if free
            buffer[index] = -1;  // mark in progress
            num_empty--;  // decrease number empty
            return index;  // return index
        }
    }

    return -1; // logically should never be reached
}


/**
 * Function used by worker threads to handle requests from the buffer
 *
 * arg: pointer to buffer of threads
 * return: 0 to end thread
 */
void *worker(void *arg) {
    int *buf = (int*) arg;  // get buffer pointer in int form

    // run infinite loop
    while(1) {
        pthread_mutex_lock(&lock);  // get lock
        while (num_full == 0) {  // sleep if no available requests
            pthread_cond_wait(&cons, &lock);
        }

        int req_index;  // index of request for marking complete later
        int req = get_full(buf, &req_index);  // find request to handle
        pthread_mutex_unlock(&lock);  // unlock

        requestHandle(req);  // handle request
        Close(req);  // close connection

        pthread_mutex_lock(&lock);  // get lock
        buf[req_index] = -2;  // mark request slot free
        num_empty++;  // increment num_empty
        pthread_cond_signal(&prod);  // wake producer
        pthread_mutex_unlock(&lock);  // unlock
    }

    return 0;  // return 0 to end thread
}

int main(int argc, char *argv[]) {
    int connfd, port, threads, clientlen;  // aded threads, buffers
    struct sockaddr_in clientaddr;
    char *shm_name;  // string name of shared memory area

    signal(SIGINT,  sigint_handler);  // add signal handler

    getargs(&port, &threads, &buffers, &shm_name, argc, argv);  // added additional args

    // make sure the number of worker threads and buffers is greater than 0 and port > 2000
    if (threads <= 0 || buffers <= 0 || port <= 2000) {
        exit(1);
    }

    num_full = 0;  // 0 full buffers
    num_empty = buffers;  // all empty buffers

    //
    // CS537 (Part B): Create & initialize the shared memory region...
    //  

    pthread_t thread_pool[threads];  // array of threads
    int fd_arr[buffers];  // array of files

    // initialize lock, exit if it fails
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Unable to initialize lock.\n");
        exit(1);
    }

    // initialize buffers
    for (int index = 0; index < buffers; index++) {
        fd_arr[index] = -2;  // -2 indicates a free slot, -1 indicates in progress
    }

    // initialize threads
    for (int index = 0; index < threads; index++) {
        if (pthread_create(&thread_pool[index], NULL, worker, (void*) fd_arr) != 0) {
            printf("Unable to create new thread.\n");
            exit(1);  // exit if unable to create thread
        }
    }

    listenfd = Open_listenfd(port);
    while (1) {
        pthread_mutex_lock(&lock);  // get lock
        while (num_empty == 0) {  // sleep if no free buffer slots
            pthread_cond_wait(&prod, &lock);
        }

        int req_index = get_empty(fd_arr);  // find request to handle
        pthread_mutex_unlock(&lock);  // unlock

        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        pthread_mutex_lock(&lock);  // get lock
        fd_arr[req_index] = connfd;  // add connection fd
        num_full++;  // increment num_full
        pthread_cond_signal(&cons);  // wake workers
        pthread_mutex_unlock(&lock);  // unlock
    }

    return 0;
}
