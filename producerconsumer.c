#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <semaphore.h>

int numProducers = 0;
int numConsumers = 0;
int numItems = 0;

pthread_mutex_t mutex;
sem_t full;
sem_t empty;

void *produce(void *param);
void *consume(void *param);

int buffer[16];

int numItemsProduced = 0;
int numItemsInBuffer = 0;

struct ThreadInfo {
	pthread_t threadID;
	int threadNumber;
	int counter;
	int numToConsume;
	int numConsumed;
	int numToProduce;
	int numProduced;
};

int main(int argc, char *argv[])
{
	struct ThreadInfo *producerThreads;
	struct ThreadInfo *consumerThreads;
	pthread_attr_t attr;

	int i = 0;

	pthread_attr_init(&attr);

	if (argc <= 3)
	{
		printf("%s\n", "3 integers are required as command line arguments!");
	}

	if (argc == 4)
	{
		printf("%s\n", "Running program...");

		numProducers = atoi(argv[1]);
		numConsumers = atoi(argv[2]);
		numItems = atoi(argv[3]);

		// Number of producer/consumer/item is 2^n, n is specified by command line argument.
		numProducers = pow(2, numProducers);
		numConsumers = pow(2, numConsumers);
		numItems = pow(2, numItems);

		printf("Producers %d Consumers %d Items %d\n", numProducers, numConsumers, numItems);

		// Initialize semaphores.
		if (sem_init(&full, 0, 0) == -1)
		{
			printf("%s%d\n", "An error occured while initializing semaphore: ", errno);
		}

		if(sem_init(&empty, 0, 16) == -1)
		{
			printf("%s%d\n", "An error occured while initializing semaphore: ", errno);
		}

		// Initialize mutex.
		if (pthread_mutex_init(&mutex, NULL) != 0)
		{
			printf("%s\n", "An error occured while initializing mutex!");
		}

		// Allocate memory, each producer/consumer gets a thread.
		producerThreads = calloc(numProducers, sizeof(struct ThreadInfo));
		consumerThreads = calloc(numConsumers, sizeof(struct ThreadInfo));

		printf("Creating %d producer threads and %d consumer threads.\n", numProducers, numConsumers);

		// Spawn producer threads.
		i = 0;
		for (i; i < numProducers; i++)
		{
			producerThreads[i].threadNumber = i;
			producerThreads[i].counter = 0;
			producerThreads[i].numToProduce = numItems / numProducers;
			producerThreads[i].numProduced = 0;

			int created = pthread_create(&producerThreads[i].threadID, &attr, produce, &producerThreads[i]);
			//printf("Created producer thread %d\n", i);

			if (created != 0)
			{
				printf("%s %d\n", "ERROR CREATING THREAD!", created);
			}
		}

		// Spawn consumer threads.
		i = 0;
		for (i; i < numConsumers; i++)
		{
			consumerThreads[i].threadNumber = i;
			consumerThreads[i].numToConsume = numItems / numConsumers;
			consumerThreads[i].numConsumed = 0;

			int created = pthread_create(&consumerThreads[i].threadID, &attr, consume, &consumerThreads[i]);
			//printf("Created consumer thread %d that will consume %d items\n", i, consumerThreads[i].numToConsume);


			if (created != 0)
			{
				printf("%s %d\n", "ERROR CREATING THREAD!", created);
			}
		}

		// Wait for threads to exit.
		i = 0;
		for (i; i < numProducers; i++)
		{
			pthread_join(producerThreads[i].threadID, NULL);
		}
		i = 0;
		for (i; i < numConsumers; i++)
		{
			pthread_join(consumerThreads[i].threadID, NULL);
		}


		printf("All threads joined...\n");

	}

	if (argc >= 5)
	{
		printf("%s\n", "3 integers are required as command line arguments!");
	}


}

void putItemIntoBuffer(int item, struct ThreadInfo *thread)
{
	// Check to see if buffer is full.
	if (numItemsInBuffer < 16)
	{
		if (buffer[numItemsInBuffer] <= 0)
		{
			//printf("(%d)Placing %d at index %d", thread->threadNumber , item, numItemsInBuffer);
			printf("%s%d\n", ">>>>>>>>>Producing item ", item);
			buffer[numItemsInBuffer] = item;
			numItemsInBuffer++;
			//printf(" p# items in buffer = %d\n", numItemsInBuffer);			
		}
	}
	else
	{
		printf("Buffer is full! numItemsInBuffer is %d\n", numItemsInBuffer);
		printf("Cannot place item %d into buffer!\n", item);
	}
}

void removeItemFromBuffer(struct ThreadInfo *thread)
{
	if (numItemsInBuffer > 0)
	{
		//printf("(%d)Removing %d at index %d", thread->threadNumber , buffer[numItemsInBuffer-1], numItemsInBuffer-1);
		printf("%s%d\n", "Consuming item ", buffer[numItemsInBuffer-1]);
		buffer[numItemsInBuffer-1] = -1;
		numItemsInBuffer--;
		//printf(" c# items in buffer = %d\n", numItemsInBuffer);
	}
}


void *produce(void *param)
{
	struct ThreadInfo *thread = param;

	while (thread->numProduced != thread->numToProduce)
	{

		// Decrement empty
		sem_wait(&empty);

		// Put item into buffer
		pthread_mutex_lock(&mutex);

		if (numItemsProduced == numItems)
		{
			pthread_mutex_unlock(&mutex);
			sem_post(&empty);
			break;
		}

		putItemIntoBuffer(thread->threadNumber * thread->numToProduce + thread->counter, thread);
		numItemsProduced++;
		thread->numProduced++;
		thread->counter++;

		pthread_mutex_unlock(&mutex);

		// Increment full
		sem_post(&full);
	}

	//printf("%s%d\n", "Exiting producer thread ", thread->threadNumber);
	pthread_exit(0);
}


void *consume(void *param)
{
	struct ThreadInfo *thread = param;

	while (thread->numConsumed != thread->numToConsume)
	{
		// Decrement full
		sem_wait(&full);

		// Consume item from buffer
		pthread_mutex_lock(&mutex);

		if (thread->numConsumed < thread->numToConsume)
		{
			removeItemFromBuffer(thread);
			thread->numConsumed++;			
		}

		pthread_mutex_unlock(&mutex);

		// Increment empty
		int semsuccess2 = sem_post(&empty);

	}

	//printf("%s%d\n", "Exiting consumer thread ", thread->threadNumber);
	pthread_exit(0);
}