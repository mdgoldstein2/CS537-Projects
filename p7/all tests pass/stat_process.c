// Program that reads out worker thread data from a simple web server

#include "helper.h"

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
void getargs(char **shm_name, int *sleeptime_ms, int *num_threads, int argc, char *argv[]) {
    if (argc != 4) {  // check # args
        fprintf(stderr, "Usage: %s <shm_name> <sleeptime_ms> <num_threads>\n", argv[0]);
        exit(1);
    }

    // transfer args from argv
    *shm_name = argv[1];
    *sleeptime_ms = atoi(argv[2]);
    *num_threads = atoi(argv[3]);

    if (*sleeptime_ms <= 0) {  // check if sleep time <= 0, exit if so
        fprintf(stderr, "Sleep time must be > 0.\n");
        exit(1);
    }

    if (*num_threads <= 0) {  // check if number of threads is > 0
        fprintf(stderr, "Number of threads must be > 0.\n");
        exit(1);
    }
}

/*
 * Main function of program. Infinetly loops to get worker thread info from server.
 *
 * argc: number of command line args
 * argv: array of command line args as strings
 * Return: 0 to end program (not reached)
 */
int main(int argc, char *argv[]) {
    char *shm_name;  // name of shared region
    int sleep_time;  // how long program should sleep between iterations
    int num_threads;  // number of worker threads in server

    getargs(&shm_name, &sleep_time, &num_threads, argc, argv);  // get arguments

     // initialize shared memory
    int shmfd = shm_open(shm_name, O_RDWR, S_IRUSR | S_IWUSR);

    // make sure memory initialized correctly
    if (shmfd == -1) {  // there was an error
        fprintf(stderr, "shm_open failed.\n");  // print message
        exit(1);  // exit
    }

    // map memory to pointer
    shm_entry *shared_mem = (shm_entry*) mmap(NULL, getpagesize(), PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shared_mem == (shm_entry*) -1) {  // if error
        fprintf(stderr, "mmap failed.\n");  // print message
        exit(1);  // exit
    }

    // time structure for waiting
    struct timespec sleep;
    sleep.tv_sec = sleep_time / 1000;  // get number of whole seconds
    sleep.tv_nsec = (sleep_time % 1000) * 1000000;  // put remainder into nanoseconds

    int iteration_count = 1; // counts number of iterations

    // infinite loop to get data
    while (1) {
        // sleep for determined amount of time
        if (nanosleep(&sleep, NULL) == -1) {  // make sure no error
            fprintf(stderr, "nanosleep failed.\n");
            exit(1);
        }

        printf("%i\n", iteration_count);  // print iteration number

        // loop over threads, print data
        for (int index = 0; index < num_threads; index++) {
            printf("%li : %i %i %i\n", shared_mem[index].thread_ID,
                shared_mem[index].total_reqs, shared_mem[index].static_reqs,
                    shared_mem[index].dynamic_reqs);
        }

        printf("\n");  // space out with empty line
        iteration_count++;  // increment count
    }

    return 0;  // Never reached
}
