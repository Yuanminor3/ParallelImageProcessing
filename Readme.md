
Project group number: 33

Group Member and x500s:

    Yuan Jiang ( jian0655 )
    Matthew Olson ( olso9444 )
    Thomas Nicklaus ( nickl098 )

The name of the CSELabs computer that you tested your code on: 
    
    csel-kh1250-35.cselabs.umn.edu
    
Members’ individual contributions:

    Yuan: 2.2 2.3 2.4 2.5 2.6 2.7(extra points)
    Matt: 2.2 2.3 2.4 2.5 2.6 2.7(extra points)
    Thomas: 2.2 2.3 2.4

Any changes you made to the Makefile or existing files that would affect grading:

    Add some Typedefs and functions in image_rotation.h
    Add "rm -f request_log" under "clean:" in Makefile for testing
    Extra work: Add "/usr/bin/time -f "%e %P %M"" before "./image_rotation img/$$dir_num output/$$dir_num 10 180;\"

Any assumptions that you made that weren’t outlined in section 7

    N/A

How you designed your program for Parallel image processing (again, high-level pseudocode would be acceptable/preferred for this part)
(If your original design (intermediate submission) was different, explain how it changed)

    //There are one main thread and one processing thread and multiple worker threads in total.
    ...// Initializations (Queue structure, arguments from main, thread pool...)
    ...// Initializations (mutex, synchronization...)

    Processing thread:
        open(dir)
        traverse(dir){
            //critical section (lock and unlock)
            while (size == maxSize){
                wait(pro_cond)
            }
            enqueue(image_info)
            signal(work_cond)
        }
        // traverse all
        //critical section (lock and unlock)
        while (num_file_processed < num_file_added){
            wait(pro_cond)
        }
        broadcast(allTraverseDone)
        free()
        exit(pro_thread)

    worker threads: // multiple threads
        while(1){
            while(size <= 0){
                while(allTraverseDone){
                    exit(worker_thread)
                }
                wait(work_cond)
            }
            //critical section (lock and unlock)
            dequeue(image_info)
            getInfo(Image)
            rotation(Image)
            storeOutputDir(Image)

            free()

            log(Image_info)

        }

    main:
        read(arguments)
        open(logFile)

        processing_thread()
        for (){
            worker_threads()
        }
        joint_thread(processing_thread)
        joint_All_threads(worker_threads)

        destroy_all_mutex()
        free()

Any code that you used AI helper tools to write with a clear justification and explanation of the code (Please see below for the AI tools acceptable use policy)

    Most of ideas were coming from Lab 9 but we asked ChatGPT learn how to make a good Typedef especially for Queue structure

Program Analysis Document(optional)

    In Makefile, we revised line 18 to "/usr/bin/time -f "%e %P %M" ./image_rotation img/$$dir_num output/$$dir_num 10 180;\"
    Here:
        %e : Elapsed real (wall clock) time used by the process, in seconds.
        %P : Percentage of the CPU that this job got. This is just user + system times divided by the total running time. It also prints a percentage sign.
        %M : Maximum resident set size of the process during its lifetime, in Kilobytes.

    Reference: https://www.cyberciti.biz/faq/unix-linux-time-command-examples-usage-syntax/

    More details are in PDF file located the same directory as Makefile.