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

char* input_dir;
char* output_dir;
int num_workers;
int rotation_angle;
FILE *log_file = NULL;

int total_files_processed = 0; // worker thread
int total_files_found = 0;  // processing thread
int processing_done_cond = 0; // signal for all Done condition
int workers[MAX_THREADS]; // Record number of item processed by specific worker thread

// Declare the request queue
request_queue_t request_queue;
// Declare mutex and condition variables
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex for protecting the log file write operation
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pro_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t work_cond = PTHREAD_COND_INITIALIZER;

// Initialize the request queue
void initialize_request_queue(request_queue_t *q) {
    if (q != NULL) {
        q->head = NULL;
        q->tail = NULL;
        q->size = 0;
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

/*
    The Function takes:
    to_write: A file pointer of where to write the logs. 
    requestNumber: the request number that thacae thread just finished.
    file_name: the name of the file that just got processed. 

    The function output: 
    it should output the threadId, requestNumber, file_name into the logfile and stdout.
*/
void log_pretty_print(FILE* to_write, int threadId, int requestNumber, char * file_name){

    // Lock the mutex to prevent race conditions when writing to the log file
    pthread_mutex_lock(&log_mutex);

    // Print to the log file
    if (to_write != NULL) {
        fprintf(to_write, "[%d][%d][%s]\n", threadId, requestNumber, file_name);
        fflush(to_write); // Flush the stream to ensure it's written to the file
    }

    // Print to stdout
    printf("[%d][%d][%s]\n", threadId, requestNumber, file_name);
    fflush(stdout);
    // Unlock the mutex
    pthread_mutex_unlock(&log_mutex);
}


// Function to dequeue a request from the queue
request_item_t *dequeue_request(request_queue_t *queue) {
    // Check if the queue is NULL or if the queue is empty
    if (queue == NULL || queue->head == NULL) {
        fprintf(stderr, "Error: The queue or item is NULL in enqueue_request.\n");
        return NULL;
    }

    // Take the item from the head of the queue
    request_node_t *old_head = queue->head;
    request_item_t *item = malloc(sizeof(request_item_t)); // Allocate memory for the item to return
    if (item == NULL) {
        fprintf(stderr, "Error: Memory allocation failed in enqueue_request.\n");
        return NULL;
    }
    
    *item = old_head->item; // Copy the data from the old head to the item

    // Move the head pointer to the next element in the queue
    queue->head = old_head->next;
    
    // If the queue is now empty, set the tail to NULL
    if (queue->head == NULL) {
        queue->tail = NULL;
    }

    // Decrement the size of the queue
    queue->size--;

    free(old_head); // Free the old head node

    return item; // Return the dequeued item
}
// Function to enqueue a request into the request queue
void enqueue_request(request_queue_t *queue, request_item_t *item) {
    if (queue == NULL || item == NULL) {
        fprintf(stderr, "Error: The queue or item is NULL in enqueue_request.\n");
        return;
    }

    // Create a new queue node
    request_node_t *new_node = (request_node_t *)malloc(sizeof(request_node_t));
    if (new_node == NULL) {
        // Handle error: malloc failed
        fprintf(stderr, "Error: Memory allocation failed in enqueue_request.\n");
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

/*

    1: The processing function takes a void* argument called args. It is expected to be a pointer to a structure processing_args_t 
    that contains information necessary for processing.

    2: The processing thread need to traverse a given dictionary and add its files into the shared queue while maintaining synchronization using lock and unlock. 

    3: The processing thread should pthread_cond_signal/broadcast once it finish the traversing to wake the worker up from their wait.

    4: The processing thread will block(pthread_cond_wait) for a condition variable until the workers are done with the processing of the requests and the queue is empty.

    5: The processing thread will cross check if the condition from step 4 is met and it will signal to the worker to exit and it will exit.

*/

void *processing(void *args)
{
    processing_args_t* processing_args = (processing_args_t*)args;

    DIR* dir;
    struct dirent* entry;

    // Open the directory for reading
    dir = opendir(processing_args->input_dir);
    if (dir == NULL) {
        perror("Error opening directory!!");
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
            snprintf(file_path, PATH_MAX, "%s/%s", processing_args->input_dir, entry->d_name);

            // Create a new request and add it to the queue
            request_item_t *new_request = malloc(sizeof(request_item_t));
            if (new_request == NULL) {
                free(file_path);
                perror("malloc failed");
                continue;
            }

            new_request->file_path = file_path;
            new_request->rotation_angle = processing_args->rotation_angle;
            // critical section
            pthread_mutex_lock(&queue_mutex);

            while(request_queue.size == MAX_QUEUE_LEN){
                pthread_cond_wait(&pro_cond, &queue_mutex);
            }

            enqueue_request(&request_queue, new_request);
            char* fn = strrchr(file_path, '/')+1;
            //printf("Enqueued picture %s\n", fn);
            total_files_found++; // count # of png file found
            // Signal the worker threads
            pthread_cond_signal(&work_cond);
            pthread_mutex_unlock(&queue_mutex);

        }
    }

    // Let all worker threads know the processing thread is completed
    // lock the queue
    //printf("Finished all traverse!\n");
    pthread_mutex_lock(&queue_mutex);
    //The processing thread will verify if the number of image files passed into the queue is equal to the total number of images processed by the workers
    while(total_files_processed < total_files_found){
        pthread_cond_wait(&pro_cond, &queue_mutex);
    }

    // set signal to tell all worder threads
    processing_done_cond = 1; // signal for all Done condition

    pthread_cond_broadcast(&work_cond);
    pthread_mutex_unlock(&queue_mutex);

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

void *worker(void *args){

    worker_arg_t* workerArg = (worker_arg_t*)args;
    int threadID = workerArg->threadID;
    //printf("work thread %d is requesting\n", threadID);

    while (1) {
        pthread_mutex_lock(&queue_mutex);

        while (request_queue.size <= 0) {
            // Check if termination condition is met - the queue is empty and all files have been enqueued
            if (processing_done_cond) {
                pthread_mutex_unlock(&queue_mutex);
                //fprintf(stdout, "Worker threads %d completed, exiting...\n", threadID);
                //fflush(stdout);
                pthread_exit(NULL);  // Exit the loop and terminate the thread
            }
            // Wait on cons_cond
            pthread_cond_wait(&work_cond, &queue_mutex);
        }

        request_item_t *curImage = dequeue_request(&request_queue);
        // Error handling for null item
        if (curImage == NULL) {
            fprintf(stderr, "Dequeued item is NULL.\n");
            return;
        }

        /*
            Stbi_load takes:
                A file name, int pointer for width, height, and bpp

        */
        // Variables to hold the image dimensions and color channels
        int width, height, bpp;

        // Load the image using the stb_image library
        // Replace 'curImage->file_path' with the actual path variable from your request_item_t
        uint8_t *image_result = stbi_load(curImage->file_path, &width, &height, &bpp, CHANNEL_NUM);
        if (image_result == NULL) {
            fprintf(stderr, "Failed to load the image from file name: %s\n",curImage->file_path);
            // Handle the error: image loading failed
            return;
        }        

        uint8_t **result_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
        uint8_t** img_matrix = (uint8_t **)malloc(sizeof(uint8_t*) * width);
        for(int i = 0; i < width; i++){
            result_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
            img_matrix[i] = (uint8_t *)malloc(sizeof(uint8_t) * height);
        }


        /*
        linear_to_image takes: 
            The image_result matrix from stbi_load
            An image matrix
            Width and height that were passed into stbi_load
        
        */
        linear_to_image(image_result, img_matrix, width, height);        

        ////TODO: you should be ready to call flip_left_to_right or flip_upside_down depends on the angle(Should just be 180 or 270)
        //both take image matrix from linear_to_image, and result_matrix to store data, and width and height.
        //Hint figure out which function you will call. 
        //flip_left_to_right(img_matrix, result_matrix, width, height); or flip_upside_down(img_matrix, result_matrix ,width, height);
        // Now decide which flip function to call based on the rotation angle
        if (curImage->rotation_angle == 180) {
            // Assuming 'flip_left_to_right' is defined and alters 'img_matrix' in place
            flip_left_to_right(img_matrix, result_matrix, width, height);
        } else if (curImage->rotation_angle == 270) {
            // Assuming 'flip_upside_down' is defined and alters 'img_matrix' in place
            flip_upside_down(img_matrix, result_matrix, width, height);
        } else {
            // Handle error: invalid rotation angle
            fprintf(stderr, "Invalid rotation angle: %d\n", curImage->rotation_angle);
        }

        
        ///Hint malloc using sizeof(uint8_t) * width * height
        uint8_t* img_array = malloc(sizeof(uint8_t) * width * height);
        if (img_array == NULL) {
            // Handle error: memory allocation failed for the image array
            fprintf(stderr, "Memory allocation failed for the image array.\n");
            // Free the image matrix
            for (int i = 0; i < height; ++i) {
                free(img_matrix[i]);
            }
            free(img_matrix);
            stbi_image_free(image_result);
            return;
        }

        ///TODO: you should be ready to call flatten_mat function, using result_matrix
        //img_arry and width and height; 
        flatten_mat(result_matrix, img_array, width, height);



        ///TODO: You should be ready to call stbi_write_png using:
        //New path to where you wanna save the file,
        //Width
        //height
        //img_array
        //width*CHANNEL_NUM
        char out_path[PATH_MAX];
        char* fn = strrchr(curImage->file_path, '/')+1;
    
        snprintf(out_path, PATH_MAX, "%s/%s", output_dir, fn);
        stbi_write_png(out_path, width, height, CHANNEL_NUM, img_array, width*CHANNEL_NUM);


        // free all before unlock
        for(int i = 0; i < width; ++i) {
            free(result_matrix[i]);
            free(img_matrix[i]);
        }
        free(result_matrix);
        free(img_matrix);
        stbi_image_free(image_result); // Free the memory allocated by stbi_load

        free(img_array);

        // start unlock
        total_files_processed++; // count # of png file processed
        
        // Output the threadId, requestNumber, file_name into the logfile and stdout
        workers[threadID]++;
        log_pretty_print(log_file, threadID, workers[threadID], curImage->file_path);


        // Signal the processing thread
        pthread_cond_signal(&pro_cond);
        pthread_mutex_unlock(&queue_mutex);

    }      


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
    input_dir = argv[1];
    output_dir = argv[2];
    num_workers = atoi(argv[3]);
    rotation_angle = atoi(argv[4]);

    // Have assumed he number of worker threads will be less than the number of requests

    if (rotation_angle != 180 && rotation_angle != 270) {
        fprintf(stderr, "Rotation angle must be 180 or 270\n");
        return EXIT_FAILURE;
    }
    

    // Initialize the request queue
    initialize_request_queue(&request_queue);

    // Initialize the log file

    log_file = fopen("request_log", "a"); // Open for appending to keep existing logs
    if (log_file == NULL) {
        // Handle the error
        fprintf(stderr, "Error opening log file.\n");
        exit(EXIT_FAILURE);
    }

    // Create and start the processing thread
    pthread_t proc_thread;
    processing_args_t proc_args = {input_dir, num_workers, rotation_angle}; // arguments of input
    if (pthread_create(&proc_thread, NULL, processing, &proc_args) != 0) {
        perror("Failed to create processing thread");
        exit(EXIT_FAILURE);
    }
    // Create and start the worker threads
    //Initialize thread pool and record number of item processed by specific worker thread
    pthread_t workers[num_workers];
    worker_arg_t args[num_workers];
    for (int i = 0; i < num_workers; i++) {
        args[i].threadID = i;  // Assign an ID starting from 0
        if (pthread_create(&workers[i], NULL, (void*)worker, (void*)&args[i]) != 0) {
            perror("Failed to create worker thread");
            exit(EXIT_FAILURE);
        }
    }

    // Join threads (wait for them to finish)
    pthread_join(proc_thread, NULL);
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }

    // Cleanup the request queue
    cleanup_request_queue(&request_queue);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&log_mutex);

    // close log file
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    return 0;

}