#include "image_rotation.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
 
 
//Global integer to indicate the length of the queue??
//Global integer to indicate the number of worker threads
//Global file pointer for writing to log file in worker??
//Might be helpful to track the ID's of your threads in a global array
//What kind of locks will you need to make everything thread safe? [Hint you need multiple]
//What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
//How will you track the requests globally between threads? How will you ensure this is thread safe?
//How will you track which index in the request queue to remove next?
//How will you update and utilize the current number of requests in the request queue?
//How will you track the p_thread's that you create for workers?
//How will you know where to insert the next request received into the request queue?





int total_files_processed = 0;
int total_files_found = 0;
int all_files_enqueued = 0;
// Declare the request queue
request_queue_t request_queue;

// Initialize the request queue
void initialize_request_queue(request_queue_t *q) {
    if (q != NULL) {
        q->head = NULL;
        q->tail = NULL;
        q->size++;
    }
}

// Cleanup the request queue
void cleanup_request_queue(request_queue_t *q) {
    if (q != NULL) {
        request_node_t *current = q->head;
        request_node_t *next;

        while (current != NULL) {
            next = current->next;
            free(current->item.file_path); // assuming file_path was dynamically allocated
            free(current);
            current = next;
        }

        q->head = NULL;
        q->tail = NULL;
    }
}

// Declare mutex and condition variables
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
//pthread_cond_t processing_done_cond = PTHREAD_COND_INITIALIZER;
//pthread_cond_t work_cond = PTHREAD_COND_INITIALIZER;

/*
    The Function takes:
    to_write: A file pointer of where to write the logs. 
    requestNumber: the request number that thacae thread just finished.
    file_name: the name of the file that just got processed. 

    The function output: 
    it should output the threadId, requestNumber, file_name into the logfile and stdout.
*/
void log_pretty_print(FILE* to_write, int threadId, int requestNumber, char * file_name){
   
}


/*

    1: The processing function takes a void* argument called args. It is expected to be a pointer to a structure processing_args_t 
    that contains information necessary for processing.

    2: The processing thread need to traverse a given dictionary and add its files into the shared queue while maintaining synchronization using lock and unlock. 

    3: The processing thread should pthread_cond_signal/broadcast once it finish the traversing to wake the worker up from their wait.

    4: The processing thread will block(pthread_cond_wait) for a condition variable until the workers are done with the processing of the requests and the queue is empty.

    5: The processing thread will cross check if the condition from step 4 is met and it will signal to the worker to exit and it will exit.

*/

// Function to enqueue a request into the request queue
void enqueue_request(request_queue_t *queue, request_item_t *item) {
    if (queue == NULL || item == NULL) {
        // Handle error: either the queue or the item is NULL
        return;
    }

    // Create a new queue node
    request_node_t *new_node = (request_node_t *)malloc(sizeof(request_node_t));
    if (new_node == NULL) {
        // Handle error: malloc failed
        return;
    }

    // Copy the item data into the new node
    new_node->item.file_path = item->file_path;  // Assumes that file_path is already allocated
    new_node->item.rotation_angle = item->rotation_angle;
    new_node->next = NULL;

    // If the queue is empty, the new node is the head and the tail
    if (queue->tail == NULL) {
        queue->head = queue->tail = new_node;
    } else {
        // Otherwise, add the new node at the end of the queue and update the tail
        queue->tail->next = new_node;
        queue->tail = new_node;
    }

    // Increment the size of the queue
    queue->size++;
}

void *processing(void *args)
{
    processing_args_t* processing_args = (processing_args_t*)args;

    DIR* dir;
    struct dirent* entry;

    // const char *path = processing_args->input_dir;
    // if (access(path, F_OK)!=0){
    //     perror("Error openning dir");
    //     return EXIT_FAILURE;
    // }

    // Open the directory for reading
    dir = opendir(processing_args->input_dir);
    if (dir == NULL) {
        perror("Error opening directory");
        pthread_exit(NULL);
    }
    //Traverse a given dictionary and add its files into the shared queue
    while ((entry = readdir(dir)) != NULL) {
        // Only consider .png file
        if (strstr(entry->d_name, ".png")) {
            char *file_path = malloc(PATH_MAX);
            if (file_path == NULL) {
                perror("malloc failed");
                continue;
            }
            snprintf(file_path, PATH_MAX, "%s/%s", &processing_args->input_dir, entry->d_name);

            // Create a new request and add it to the queue
            request_item_t *new_request = malloc(sizeof(request_item_t));
            if (new_request == NULL) {
                free(file_path);
                perror("malloc failed");
                continue;
            }

            new_request->file_path = file_path;
            new_request->rotation_angle = processing_args->rotation_angle;

            enqueue_request(&request_queue, new_request);
            printf("Enqueued picture %d\n", total_files_found+1);
            total_files_found++; // count # of png file found
            
        }
    }

    closedir(dir);
    pthread_exit(NULL);

}

/*
    1: The worker threads takes an int ID as a parameter

    2: The Worker thread will block(pthread_cond_wait) for a condition variable that there is a requests in the queue. 

    3: The Worker threads will also block(pthread_cond_wait) once the queue is empty and wait for a signal to either exit or do work.

    4: The Worker thread will processes request from the queue while maintaining synchronization using lock and unlock. 

    5: The worker thread will write the data back to the given output dir as passed in main. 

    6:  The Worker thread will log the request from the queue while maintaining synchronization using lock and unlock.  

    8: Hint the worker thread should be in a While(1) loop since a worker thread can process multiple requests and It will have two while loops in total
        that is just a recommendation feel free to implement it your way :) 
    9: You may need different lock depending on the job.  

*/

// Only for intermediate submission
void *worker(void *args){

    worker_arg_t* workerArg = (worker_arg_t*)args;
    int threadID = workerArg->threadID;
    
    printf("work thread %d is requesting\n", threadID);
    pthread_exit(NULL);

}



/*
    Main:
        Get the data you need from the command line argument 
        Open the logfile
        Create the threads needed
        Join on the created threads
        Clean any data if needed. 


*/


int main(int argc, char* argv[])
{
    if(argc != 5)
    {
        fprintf(stderr, "Usage: File Path to image dirctory, File path to output dirctory, number of worker thread, and Rotation angle\n");
        return EXIT_FAILURE;
    }
    
    // Parse command line arguments
    char* input_dir = argv[1];
    char* output_dir = argv[2];
    int num_workers = atoi(argv[3]);
    int rotation_angle = atoi(argv[4]);

    // Have assumed he number of worker threads will be less than the number of requests

    if (rotation_angle != 180 && rotation_angle != 270) {
        fprintf(stderr, "Rotation angle must be 180 or 270\n");
        return EXIT_FAILURE;
    }
    

    // Initialize the request queue
    initialize_request_queue(&request_queue);

    // Create and start the processing thread
    pthread_t proc_thread;
    processing_args_t proc_args = {input_dir, num_workers, rotation_angle}; // arguments of input
    if (pthread_create(&proc_thread, NULL, processing, &proc_args) != 0) {
        perror("Failed to create processing thread");
        exit(EXIT_FAILURE);
    }


    // Create and start the worker threads
    //Initialize thread pool
    pthread_t workers[num_workers];
    worker_arg_t args[num_workers];
    for (int i = 0; i < num_workers; i++) {
        args[i].threadID = i + 1;  // Assign an ID starting from 1
        if (pthread_create(&workers[i], NULL, (void*)worker, (void*)&args[i]) != 0) {
            perror("Failed to create worker thread");
            exit(EXIT_FAILURE);
        }
    }

    // Join threads (wait for them to finish)
    pthread_join(proc_thread, NULL);
    for (int i = 0; i < num_workers; ++i) {
        pthread_join(workers[i], NULL);
    }

    // Cleanup the request queue
    cleanup_request_queue(&request_queue);
    return 0;

}