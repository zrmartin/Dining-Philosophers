#ifndef DPHIL_H
#define DPHIL_H


#define THINKING 0
#define HUNGRY 1
#define EATING 2
#define MEMKEY 10
#define SEMKEY 20

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
}arg;


int createSemaphores(int n, union semun arg, int knives);
int createSharedMemory(int n);
int *attachAndInitialize(int memid, int n);
void philosopher(int n, int i, int time, int semid, int *memaddr, int knives);
void test(int i, int left, int right, int semid, int *memaddr);
void take_forks(int n, int i, int left, int right, int semid, int *memaddr, int knives);
void put_forks(int n, int i, int left, int right, int semid, int *memaddr, int knives);


#endif
