#ifndef IMAGE_ROTATION_H_
#define IMAGE_ROTATION_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <stdint.h>
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define CHANNEL_NUM 1

#include "stb_image.h"
#include "stb_image_write.h"



/********************* [ Helpful Macro Definitions ] **********************/
#define BUFF_SIZE 1024 
#define LOG_FILE_NAME "request_log"               //Standardized log file name
#define INVALID -1                                  //Reusable int for marking things as invalid or incorrect
#define MAX_THREADS 100                             //Maximum number of threads
#define MAX_QUEUE_LEN 100                           //Maximum queue length



/********************* [ Helpful Typedefs        ] ************************/

// Define the structure for queue elements
typedef struct {
    char* file_path;  // The path to the image file
    int rotation_angle;  // The angle to rotate the image (180 or 270)
} request_item_t;

// Assuming the request queue is a simple linked list:
typedef struct request_node {
    request_item_t item;
    struct request_node *next;
} request_node_t;

// The queue could be represented as:
typedef struct {
    request_node_t *head;
    request_node_t *tail;
    int size;
} request_queue_t;

// Define the structure for processing thread arguments
typedef struct {
    char *input_dir;
    int num_workers;
    int rotation_angle;
} processing_args_t;

// Define the structure for worker thread arguments
typedef struct {
    int threadID;                // Unique ID for each worker thread for logging

} worker_arg_t;


/********************* [ Function Prototypes       ] **********************/
void initialize_request_queue(request_queue_t *q);
void cleanup_request_queue(request_queue_t *q);
void enqueue_request(request_queue_t *queue, request_item_t *item);
void *processing(void *args); 
void * worker(void *args); 
void log_pretty_print(FILE* to_write, int threadId, int requestNumber, char * file_name);

#endif
