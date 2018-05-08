
/* Threads and Semaphore - Project 2
Course: Operating systems */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

#define Total_Customers 50
#define Total_PostalWorkers 3


/* Defining Semaphores */

sem_t mutex1, mutex2, mutex3;
sem_t max_capacity;
sem_t customer_ready, postalWorker_ready, scales_ready, finished[Total_Customers], askOrder[Total_Customers], placeOrder[Total_PostalWorkers];



/* Defining Pipes */

int customer_ID[2];
int postal_Worker_ID[2];
int order_number[2];

/* Function prototypes */

void enqueue(int,int);
int dequeue(int);
void init_Semaphores();
void create_Queues();


/* Customer Thread Code */

void *customer(void* id)
{
    int customer_ID = *(int*)id;
    printf("Customer %d created\n",customer_ID);
	fflush(stdout);
	int order_Num, PostalWorker_ID;
	sem_wait(&max_capacity);
	printf("Customer %d enters post office\n",customer_ID);
	sem_wait(&postalWorker_ready);
	sem_wait(&mutex1);
	order_Num = rand()%3; /* Randomly generate an order number */
	enqueue(0, customer_ID);
	sem_post(&customer_ready);
	sem_post(&mutex1);
	sem_wait(&askOrder[customer_ID]);
	sem_wait(&mutex2);
	PostalWorker_ID = dequeue(1);
	sem_post(&mutex2);
	printf("Customer %d asks postal worker %d to ", customer_ID, PostalWorker_ID);
	fflush(stdout);
	switch(order_Num)
	{
		case 0:
			printf("Buy stamps\n");
			break;
		case 1:
			printf("Mail a letter\n");
			break;
		case 2:
			printf("Mail a package\n");
			break;
	}
	fflush(stdout);
	sem_wait(&mutex3);
	enqueue(2, order_Num);
	sem_post(&placeOrder[PostalWorker_ID]);
	sem_post(&mutex3);
	sem_wait(&finished[customer_ID]);
	switch(order_Num)
	{
		case 0:
			printf("Customer %d finished buying stamps\n", customer_ID);
			break;
		case 1:
			printf("Customer %d finished mailing letter\n", customer_ID);
			break;
		case 2:
			printf("Customer %d finished mailing package\n", customer_ID);
			break;
	}
	fflush(stdout);
	printf("Customer %d leaves post office\n",customer_ID);
	fflush(stdout);
	sem_post(&max_capacity);
    pthread_exit(id);
}

/* Postal worker thread */

void *postalWorker(void* id)
{
	int postalWorkerID = *(int *)id;
    printf("Postal worker %d created\n",postalWorkerID);
	
	while(1)
	{
		fflush(stdout);
		int pwcustomer_ID,pworder_Num;
		struct timespec req = {0};

		sem_post(&postalWorker_ready);
		sem_wait(&customer_ready);
		sem_wait(&mutex1);
		pwcustomer_ID = dequeue(0);
		sem_post(&mutex1);
		printf("Postal worker %d serving customer %d\n",postalWorkerID, pwcustomer_ID);
		fflush(stdout);
		sem_wait(&mutex2);
		enqueue(1, postalWorkerID);
		sem_post(&askOrder[pwcustomer_ID]);
		sem_post(&mutex2);
		sem_wait(&placeOrder[postalWorkerID]);
		sem_wait(&mutex3);
		pworder_Num = dequeue(2);
		sem_post(&mutex3);
		switch(pworder_Num)
		{
			case 0:
				req.tv_sec = 1;
				req.tv_nsec = 0L;
				nanosleep(&req, (struct timespec *)NULL);
				break;
			case 1:
				req.tv_sec = 1;
				req.tv_nsec = 500 * 1000000L;
				nanosleep(&req, (struct timespec *)NULL);
				break;
			case 2:
				sem_wait(&scales_ready);
				printf("Scales in use by postal worker %d\n",postalWorkerID);
				fflush(stdout);
				req.tv_sec = 2;
				req.tv_nsec = 0L;
				nanosleep(&req, (struct timespec *)NULL);
				printf("Scales released by postal worker %d\n",postalWorkerID);
				sem_post(&scales_ready);
				fflush(stdout);
				break;
			default:
				printf("Incorrect");
		}
		printf("Postal worker %d finished serving Customer %d\n", postalWorkerID, pwcustomer_ID);
		fflush(stdout);
		sem_post(&finished[pwcustomer_ID]);
	}
}


/* Main Thread */

int main(int argc,char *argv[])
{
    int worker;
    pthread_t customerThreads[Total_Customers], postalWorkerThreads[Total_PostalWorkers];
    int customer_number, postalWorker_number;
	int custids[Total_Customers] , pwids[Total_PostalWorkers] ;
    int errcode, *status ;
  
	//Semaphore Initialization
	init_Semaphores();
	create_Queues();

    // Creation of Customer threads
	for (customer_number=0; customer_number<Total_Customers; customer_number++) 
    {
		custids[customer_number]=customer_number;
        // create thread
        if (errcode = pthread_create(&customerThreads[customer_number],NULL,customer,&custids[customer_number]))
        {
            printf("Thread couldn't be created");
        }
	}
	
	// Creation of Postal worker threads
    for (postalWorker_number=0; postalWorker_number<Total_PostalWorkers; postalWorker_number++) 
    {
		pwids[postalWorker_number]=postalWorker_number;
        // create thread
        if (errcode = pthread_create(&postalWorkerThreads[postalWorker_number],NULL,postalWorker,&pwids[postalWorker_number]))
        {
            printf("Thread couldn't be created");
        }
    }

	//Joining Customer threads when they exit
    for (customer_number=0; customer_number<Total_Customers; customer_number++)
    {
        if(errcode=pthread_join(customerThreads[customer_number],(void*)&status))
        {
           printf("Thread couldn't joined");
        }
        
      //check thread's exit status, should be the same as the thread number for this example
        if (*status != customer_number) 
        {
			printf("Customer thread %d terminated abnormally\n\n",customer_number);
			exit(1);
        }
		else
		{
			printf("Customer %d joined\n",*status);
		}
    }
	    return(0);
}


void init_Semaphores()
{
	int count;
	sem_init (&max_capacity, 0, 10);
	sem_init (&customer_ready, 0, 0);
	sem_init (&postalWorker_ready, 0, 0);
	for (count=0 ;count<Total_Customers ; count++)
	{
		sem_init (&finished[count], 0, 0);
		sem_init (&askOrder[count], 0, 0);
	}
	
	for (count=0 ;count<Total_PostalWorkers ;count++ )
	{
		sem_init (&placeOrder[count], 0, 0);
	}

	sem_init (&scales_ready, 0, 1);
	sem_init (&mutex1, 0, 1);
	sem_init (&mutex2, 0, 1);
	sem_init (&mutex3, 0, 1);
}


void enqueue(int queue, int value)
{
	switch(queue)
	{
		case 0:
			write(customer_ID[1], &value, sizeof(int));
			break;
		case 1:
			write(postal_Worker_ID[1], &value, sizeof(int));
			break;
		case 2:
			write(order_number[1], &value, sizeof(int));
			break;
	}
}

int dequeue(int queue)
{
	int value;
	switch(queue)
	{
		case 0:
			read(customer_ID[0], &value, sizeof(int));
			break;
		case 1:
			read(postal_Worker_ID[0], &value, sizeof(int));
			break;
		case 2:
			read(order_number[0], &value, sizeof(int));
			break;
	}
	return value;
}

void create_Queues()
{
	if (pipe(customer_ID) == -1) {
		printf("creation failed");
        exit(1);
    }

	if (pipe(postal_Worker_ID) == -1) {
		printf("creation failed");
		exit(1);
    }

	if (pipe(order_number) == -1) {
		printf("creation failed");
		exit(1);
    }
}



