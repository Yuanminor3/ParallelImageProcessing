
Project group number: 33

Group Member:
    Yuan Jiang ( jian0655 )
    Matthew Olson ( olso9444 )
    Thomas Nicklaus ( nickl098 )

CSELabs computer: csel-kh1250-35.cselabs.umn.edu

Any changes you made to the Makefile or existing files that would affect grading:

Add more Typedefs and functions in image_rotation.h
    
Plan outlining individual contributions for each member of your group:

    Yuan: 2.2 2.3 (All intermediate requirements)
    Matt: 2.2 2.3 (All intermediate requirements)
    Thomas: 2.2 2.3

Plan on how you are going to construct the worker threads and how you will make use of mutex
locks and unlock and Conditions variables:

Note: Below are our own ideas combined with ChatGPT, some of them may change a little when doing the final PA3.

1. Initialization:

Define a mutex lock (pthread_mutex_t) and condition variables (pthread_cond_t) for synchronizing access to the request queue. Initialize the mutex and condition variable before spawning worker threads.

2. Worker Thread Function:

Each worker thread starts by locking the mutex with pthread_mutex_lock to safely access the queue.
It then checks if the queue is empty. If it is, it waits on the condition variable using pthread_cond_wait, which atomically unlocks the mutex and waits for the condition to be signaled.
Once signaled (the processing thread has added work to the queue), or if the queue was not empty, the worker thread locks the mutex again if pthread_cond_wait was called and then proceeds to process the request.

3. Queue Monitoring and Processing:

The worker thread retrieves the next available request from the queue.
It processes the request, which includes loading the image, rotating it, and saving the result.
After processing a request, the thread checks the queue for more work. If there are no more items and the processing thread has signaled completion, the worker thread can exit.

4. Signaling:

When the processing thread places a new request in the queue, it signals a condition variable with pthread_cond_signal.
Upon completion of adding all requests to the queue, the processing thread should send a termination signal to worker threads, possibly by placing a special 'termination' request in the queue or setting a global flag.

5. Termination:

Worker threads should be joinable so that the main thread can wait for all worker threads to finish by calling pthread_join.
Once all worker threads have finished processing, clean up the mutex and condition variable.