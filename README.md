# LIFO-FIFO

This project involves programming a print server to manage print jobs from user processes and assign them to printer threads. The goal is to simulate a global print queue handling multiple producers and consumers. The queue has a maximum size of 30 jobs and supports two strategies: First-Come-First-Served (FCFS) and Shortest Job First (SJF) using priority queues. This two strategies are also referred to as LIFO and FIFO.

User processes act as producers, submitting print requests to the queue, with each request having a random size. Users can submit up to 30 print jobs, and randomization determines job counts. Printers, acting as consumers, process print jobs from the queue. The main process initializes the queue and manages interactions through a command console.
The project employs synchronization tools like semaphores to ensure efficient queue operations. Threads handle printers, processes manage producers, and signal handling ensures smooth thread termination. The main process interprets command-line inputs to start processes and threads, using sleep utilities to introduce delays between consecutive print jobs from the same user. Upon completion, resources are deallocated.

In the Final312Report.pdf I reported execution times and average waiting times based on varying user and printer counts, as well as queue size. The result show print job statistics and performance metrics, comparing the effectiveness between LIFO and FIFO.
