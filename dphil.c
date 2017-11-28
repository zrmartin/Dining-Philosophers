#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include "dphil.h"

void onTerminate() {
	int memid, semid;
	
	/* Getting id of shared memory */
	if ((memid = shmget(MEMKEY, 0, 0)) == -1) {
		perror("Error getting memid");
		exit(EXIT_FAILURE);
	}
	/* Destroying shared memory segment */
	if (shmctl(memid, IPC_RMID, 0) == -1) {
		perror("Error destorying shared memory");
		exit(EXIT_FAILURE);
	}
	if ((semid = semget(SEMKEY, 0 ,0)) == -1) {
		perror("Error getting semid");
		exit(EXIT_FAILURE);
	}
	if (semctl(semid, 0, IPC_RMID) == -1) {
		perror("Error destroying array of memory");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

/* Helper function to create shared memory segment
   this will be used to hold the status of each philopsher
   and will act as a pseudo global varaible between the processes */
int createSharedMemory(int n) {
	int memid;

	/* Create shared memory segment for the status of the philosophers */
	if ((memid = shmget(MEMKEY, sizeof(int) * n, IPC_CREAT | 0666)) == -1) {
		perror("Error creating shared memory");
		exit(EXIT_FAILURE);
	}
	return memid;
}

/* Helper function to attach the shared memory segment to the
   parent process, and in turn to all the children. The shared
   memory is an array of integers and they are initialized to 0 */
int *attachAndInitialize(int memid, int n) {
	/* Pointer to the shared memory segment of integers */
	int *sharedMemory;
	if ((sharedMemory = shmat(memid, NULL, 0)) == (int *) -1) {
		perror("Error attaching to shared memory");
		exit(EXIT_FAILURE);
	}
	/* Initializing the status of each philospher to THINKING */
	for (int i = 0; i < n; i++) {
		sharedMemory[i] = THINKING;
		i++;
	}
	return sharedMemory;

}

/* Helper function to create array of semaphores
   0 thru n-1 represent semaphores for each individual philosopher
   n represents a mutex for entering and leaving critcal areas
   n + 1 represents a semaphore for knives if input given is > 0 */
int createSemaphores(int n, union semun arg, int knives) {
	int semid;
	/* One for each philosopher + one mutex */
	int numOfSemaphores = n + 1;
	/* Add a semaphore to represent knife resource */
	if (knives) {
		numOfSemaphores++;
	}
	/* Creates an array of numOfSemaphores. */
	if ((semid = semget(SEMKEY, numOfSemaphores, IPC_CREAT | 0666)) == -1) {
		perror("Error creating semaphore");
		exit(EXIT_FAILURE);
	}
	/* Set the value of the mutex to 1 */
	arg.val = 1;
	if ((semctl(semid, n, SETVAL, arg)) == -1) {
		perror("Error initizlaing value of sempahore");
		exit(EXIT_FAILURE);
	}		
	/* Set the value of the knife semaphore to knives */
	if (knives) {
		arg.val = knives;
		if ((semctl(semid, n + 1, SETVAL, arg)) == -1) {
			perror("Error initizlaing value of sempahore");
			exit(EXIT_FAILURE);
		}		
	}
	return semid;
}

/* Test to see if neighbors are already eating, if not we can pick up forks */
void test(int i, int left, int right, int semid, int *memaddr) {
	struct sembuf up = {i, 1, 0};
	if (memaddr[i] == HUNGRY && memaddr[left] != EATING && memaddr[right] != EATING) {
		memaddr[i] = EATING;
		printf("Phil %d has picked up the forks\n", i);
		/* Up the i'th philosopher's semaphore to indicate we have forks */
		if (semop(semid, &up, 1) == -1) {
			perror("Error up-ing semaphore in test");
			exit(EXIT_FAILURE);
		}
	}
}

/* Take forks and knives in order to eat */
void take_forks(int n, int i, int left, int right, int semid, int *memaddr, int knives) {
	struct sembuf up = {n, 1, 0};
	struct sembuf down = {n, -1, 0};

	/* Down mutex - Entering critical region */
	if (semop(semid, &down, 1) == -1) {
		perror("Error down-ing mutex in take_forks");
		exit(EXIT_FAILURE);
	}
	memaddr[i] = HUNGRY;
	printf("Phil %d is hungry\n", i);
	/* See if we can pick up forks */
	test(i, left, right, semid, memaddr);
	/* Up mutex - Leaving critical region */
	if (semop(semid, &up, 1) == -1) {
		perror("Error up-ing mutex in take_forks");
		exit(EXIT_FAILURE);
	}
	
	/* Set semaphore number to this current 
	   philosopher and down the semaphore */
	down.sem_num = i;
	if (semop(semid, &down, 1) == -1) {
		perror("Error down-ing semaphore in take_forks");
		exit(EXIT_FAILURE);
	}
	/* Try to aquire a knife, if none are available,
	   process will block until one is freed */
	if (knives) {
		down.sem_num = n + 1;
		if (semop(semid, &down, 1) == -1) {
			perror("Error down-ing knife semaphore in test");
			exit(EXIT_FAILURE);
		}
		printf("Phil %d has picked up a knife\n", i);
	}		
		
}

/* Put down forks and return resources to semaphores */
void put_forks(int n, int i, int left, int right, int semid, int *memaddr, int knives) {
	struct sembuf up = {n, 1, 0};
	struct sembuf down = {n, -1, 0};

	/* Down mutex - Entering critical region */
	if (semop(semid, &down, 1) == -1) {
		perror("Error down-ing mutex in put_forks");
		exit(EXIT_FAILURE);
	}
	memaddr[i] = THINKING;
	/* Done with knife, up knife semaphore */
	if (knives) {
		up.sem_num = n + 1;
		if (semop(semid, &up, 1) == -1) {
			perror("Error up-ing knife semaphore in put_forks");
			exit(EXIT_FAILURE);
		}
		printf("Phil %d has placed a knife\n", i);
		/* Return 'up' sembuf sem_num to n, which is the mutex */
		up.sem_num = n;
	}
	printf("Phil %d has placed the forks\n", i);
	/* Test and see if left or right neighbor can aquire forks & knife now */
	test(left, (left+n-1)%n, (left+1)%n, semid, memaddr);
	test(right, (right+n-1)%n, (right+1)%n, semid, memaddr);
	/* Up mutex - Leaving critical region */
	if (semop(semid, &up, 1) == -1) {
		perror("Error up-ing mutex in take_forks");
		exit(EXIT_FAILURE);
	}
}

/* Main function that each child process will run */
void philosopher(int n, int i, int time, int semid, int *memaddr, int knives) {
	int left = (i + n - 1) % n, right = (i + 1) % n;
	while (1) {
		// Thinking
		printf("Phil %d is thinking\n", i);
		//sleep(rand() % (time + 1));
		take_forks(n, i, left, right, semid, memaddr, knives);
		// Eating
		printf("Phil %d is eating\n", i);
		sleep(rand() % (time + 1));
		put_forks(n, i, left, right, semid, memaddr, knives);
	}
}


int main(int argc, char *argv[])
{
	int n = atoi(argv[1]), knives = atoi(argv[2]), time = atoi(argv[3]), status, semid, memid;
	pid_t pids[n], pid;
	union semun arg;
	int *sharedMemory;

	
	semid = createSemaphores(n, arg, knives);
	memid = createSharedMemory(n);
	sharedMemory = attachAndInitialize(memid, n);
	
	/* Creating n number of child processes and starting philosopher function */
	for (int i = 0; i < n; i++) {
		if ((pids[i] = fork()) < 0) {
			perror("Fork failed");
			exit(EXIT_FAILURE);
		}
		// Child processes 
		else if(pids[i] == 0) {
			philosopher(n, i, time, semid, sharedMemory, knives);
		}
	}
	while (1) {
		signal(SIGTSTP, onTerminate);				
	}
}
