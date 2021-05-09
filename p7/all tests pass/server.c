#include "helper.h"
#include "request.h"

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
int listenfd;  // fd to listen on
char *shm_name;  // name of shared memory region
int *shared_buf;  // pointer to shared buffer
shm_entry *shared_mem;  // pointer to shared mem

/**
 * Signal handler for SIGINT signal.
 *
 * sig: signal number
 */
void sigint_handler(int sig) {
    munmap( shared_mem, getpagesize());  // unmap mem
    shm_unlink(shm_name);  // unlink the shared file
    Close(listenfd);  // close listenfd to free the port
    exit(0);  // exit
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
    if (argc != 5) {  // check # args
        fprintf(stderr, "Usage: %s <port> <threads> <buffers> <shm_name>\n", argv[0]);
        exit(1);
    }

    // transfer args from argv
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
    int index = *((int*) arg);  // get index in shared mem
    shared_mem[index].thread_ID = pthread_self();  // set tid in mem
    shared_mem[index].total_reqs = 0;
    shared_mem[index].static_reqs = 0;
    shared_mem[index].dynamic_reqs = 0;

    // run infinite loop
    while(1) {
        pthread_mutex_lock(&lock);  // get lock
        while (num_full == 0) {  // sleep if no available requests
            pthread_cond_wait(&cons, &lock);
        }

        int req_index;  // index of request for marking complete later
        int req = get_full(shared_buf, &req_index);  // find request to handle
        pthread_mutex_unlock(&lock);  // unlock

        requestHandle(req, &shared_mem[index]);  // handle request
        shared_mem[index].total_reqs++;  // increment total reqs
        Close(req);  // close connection

        pthread_mutex_lock(&lock);  // get lock
        shared_buf[req_index] = -2;  // mark request slot free
        num_empty++;  // increment num_empty
        pthread_cond_signal(&prod);  // wake producer
        pthread_mutex_unlock(&lock);  // unlock
    }

    return 0;  // return 0 to end thread
}

/**
 * Main function of server. Also functions as producer thread
 *
 * argc: number of command line arguments
 * argv: array of command line arguments as strings
 * Return: 0 (never reached)
 */
int main(int argc, char *argv[]) {
    int connfd, port, threads, clientlen;  // aded threads, buffers
    struct sockaddr_in clientaddr;

    signal(SIGINT,  sigint_handler);  // add signal handler

    getargs(&port, &threads, &buffers, &shm_name, argc, argv);  // added additional args

    // make sure the number of worker threads and buffers is greater than 0 and port > 2000
    if (threads <= 0 || buffers <= 0 || port <= 2000) {
        fprintf(stderr, "Number of threads and buffers must be > 0, and the port must be > 2000.\n");
        exit(1);
    }

    num_full = 0;  // 0 full buffers
    num_empty = buffers;  // all empty buffers

    // initialize shared memory
    int shmfd = shm_open(shm_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    
    // make sure memory initialized correctly
    if (shmfd == -1) {  // there was an error
        fprintf(stderr, "shm_open failed.\n");  // print message
        exit(1);  // exit
    }
    
    // truncate
    if (ftruncate(shmfd, getpagesize()) == -1) {  // if error
        fprintf(stderr, "ftruncate failed.\n");  // print message
        exit(1);  // exit
    }

    // map memory to pointer
    shared_mem = (shm_entry*) mmap(NULL, getpagesize(), PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shared_mem == (shm_entry*) -1) {  // if error
        fprintf(stderr, "mmap failed.\n");  // print message
        exit(1);  // exit
    }

    pthread_t thread_pool[threads];  // array of threads
    int fd_arr[buffers];  // array of files (buffer)
    shared_buf = fd_arr;  // setup shared buffer

    // initialize lock, exit if it fails
    if (pthread_mutex_init(&lock, NULL) != 0) {
        fprintf(stderr, "Unable to initialize lock.\n");
        exit(1);
    }

    // initialize buffers
    for (int index = 0; index < buffers; index++) {
        shared_buf[index] = -2;  // -2 indicates a free slot, -1 indicates in progress
    }

    int index_array[threads];  // used to pass through the index of each worker thread

    // initialize threads
    for (int index = 0; index < threads; index++) {
        index_array[index] = index;  // set element to its index
        if (pthread_create(&thread_pool[index], NULL, worker, (void*) &index_array[index]) != 0) {
            fprintf(stderr, "Unable to create new thread.\n");
            exit(1);  // exit if unable to create thread
        }
    }

    listenfd = Open_listenfd(port);
    while (1) {
        pthread_mutex_lock(&lock);  // get lock
        while (num_empty == 0) {  // sleep if no free buffer slots
            pthread_cond_wait(&prod, &lock);
        }

        int req_index = get_empty(shared_buf);  // find request to handle
        pthread_mutex_unlock(&lock);  // unlock

        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        pthread_mutex_lock(&lock);  // get lock
        shared_buf[req_index] = connfd;  // add connection fd
        num_full++;  // increment num_full
        pthread_cond_signal(&cons);  // wake workers
        pthread_mutex_unlock(&lock);  // unlock
    }

    return 0;
}
