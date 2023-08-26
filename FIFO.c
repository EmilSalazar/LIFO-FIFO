#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <wait.h> 
#include <errno.h>

#define SIZE 20
#define NUMB_THREADS 20

//int *buffer_index;
int shmid1, shmid2;
sem_t *buffer_mutex;
sem_t *full_sem;
sem_t *empty_sem;

typedef struct
{
    int userID;
    int jobSIZE;
    int count;
    double number;
    int pid;
    clock_t begin;
    clock_t end;
    int value;
}buffer_t;

typedef struct 
{
    buffer_t buffer[SIZE];
    int in;
    int out;
}global_buffer_t;

global_buffer_t *my_buffer;

buffer_t processID;

buffer_t averageTime;

void insertbuffer(buffer_t value) 
{
    if (my_buffer->in - my_buffer->out < SIZE) 
    {  
        value.pid = getpid(); //save process ID into our value struct
        my_buffer->buffer[my_buffer->in % SIZE] = value; //setting the value to the specific location in array determined by the index
        my_buffer->in++; //move up in the array
    } 
    else
    {
        printf("Buffer overflow\n");
    }
}
buffer_t dequeuebuffer() 
{
    buffer_t job;
    job.value = 0;
    if (my_buffer->in > my_buffer->out) 
    {
        job = my_buffer->buffer[my_buffer->out % SIZE];
        my_buffer->out++; //Go up in the array 
    } 
    else 
    {
        printf("Buffer underflow\n");
    }
    return job;
}
 
void *producer(buffer_t processID) 
{
    buffer_t var;
    int PRODUCER_LOOPS = (rand() % 20) + 1;
    for(int i = 0; i < PRODUCER_LOOPS; i++)
    {
        srand(time(NULL));
        sleep(2);
        var.value = rand() % 901 + 100;
        averageTime.begin = clock();
        sem_wait(full_sem); 
        sem_wait(buffer_mutex); /* protecting critical section */
        insertbuffer(var);
        sem_post(buffer_mutex);
        sem_post(empty_sem); // post (increment) emptybuffer semaphore
        printf("Producer <%d> added <%d> to buffer\n", processID.pid, var.value);
    }
    exit(0);
}
 
void *consumer(void *thread_n) 
{
    int thread_numb = *(int *)thread_n;
    buffer_t val;
    sleep(1);
    int i = 1;
    while (1) 
    {
        sleep(2);
        sem_wait(empty_sem);
        sem_wait(buffer_mutex);
        val = dequeuebuffer(); //(value)
        sem_post(buffer_mutex);
        sem_post(full_sem); // post (increment) fullbuffer semaphore
        averageTime.end = clock();
        averageTime.number += ((double)averageTime.end - averageTime.begin)/CLOCKS_PER_SEC;
        printf("Consumer <%ld> dequeue <%d, %d> from buffer\n", pthread_self(), val.pid, val.value);
        averageTime.count += 1;
        i++;
        if (val.value == 0) 
        {
            break;
        }
   }
    pthread_exit(0);
}

void sigint_handler(int sig) 
{
    sem_unlink("/full_semac");
    sem_unlink("/empty_semac");
    sem_unlink("/buffer_mutexac");
    shmctl(shmid2, IPC_RMID, NULL);
    exit(0);
}
 
int main(int argc, int **argv) 
{
    int numProducers = atoi(argv [1]); //takes in first int argument to represent producer
    int numConsumers = atoi(argv [2]); //takes in first int argument to represent consumer
    buffer_t totalTime; 
	totalTime.begin = clock(); //start clock timer
    averageTime.number = 0;
    averageTime.count = 0;
    signal(SIGINT, sigint_handler); //signal 

    full_sem = sem_open("/full_semac", O_CREAT, 0600, SIZE); 
    if(full_sem == SEM_FAILED) 
    {
	    perror("sem_open/full_semac");
	    exit(EXIT_FAILURE);	
    }
    empty_sem = sem_open("/empty_semac", O_CREAT, 0600, 0);
    if(empty_sem == SEM_FAILED) 
    {
	    perror("sem_open/empty_semac");
	    exit(EXIT_FAILURE);	
    }
	buffer_mutex = sem_open("/buffer_mutexac", O_CREAT, 0600, 1);
    if(buffer_mutex == SEM_FAILED) 
    {
	    perror("sem_open/buffer_mutexac");
	    exit(EXIT_FAILURE);	
    }
    /*shmid1 = shmget(1328, sizeof(int), IPC_CREAT | 0600);
	buffer_index = shmat(shmid1, NULL, 0); //shared memory for buffer inex*/
    shmid2 = shmget(1329, sizeof(global_buffer_t), IPC_CREAT | 0600);
	my_buffer = shmat(shmid2, NULL, 0); //shared memory for global buffer
    my_buffer->in = 0;
    my_buffer->out = 0;

    //*buffer_index = 0; //index set to 0 to later be called in array to represent the initial value 

    pthread_t thread[NUMB_THREADS];
    int thread_numb[NUMB_THREADS];

    for(int i = 0; i < numConsumers;) //Consumer 
    {
        thread_numb[i] = i;
        pthread_create(&thread[i], NULL, consumer, &thread_numb[i]);
        i++;
    }
    int pid;
    buffer_t processID;
    for (int i = 0; i < numProducers; ) //Producer
    {
        pid = fork();
        processID.pid = getpid(); //getting process id value
        if (pid == -1) 
        {
            printf("Failed to create process\n");
            exit(1);
        }
        else if (pid == 0) //child process
        {
            // child process executes the producer function
            producer(processID);
            exit(0);
        }
        wait(NULL); //wait for child process
        i++;
    }

    totalTime.end = clock(); //end clock timer
	double duration = ((double)totalTime.end - totalTime.begin)/CLOCKS_PER_SEC; //calculate time for total execution
	printf("Total execution time : %f\n", duration);
    double average = averageTime.number / averageTime.count; 
    printf("Average execution time: %f\n", average);

    sem_unlink("/full_semac");
    sem_unlink("/empty_semac");
    sem_unlink("/buffer_mutexac");
	
    //shmdt(buffer_index);
    shmdt(my_buffer);
    //shmctl(shmid1, IPC_RMID, NULL);
    shmctl(shmid2, IPC_RMID, NULL);
    return 0;
}