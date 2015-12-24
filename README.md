# producer-consumer

A multithreaded Linux program that simulates the producer-consumer problem. Compile using gcc, -pthread, and -lm. Program takes three command line arguments (# of producers, # of consumers, # of items to produce). Uses semaphores and mutexes to allow multiple threads to share a single buffer which holds items (integers). 

Program output: Items produced and Items consumed.
