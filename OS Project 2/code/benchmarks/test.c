#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../rpthread.h"

/* A scratch program template on which to call and
 * test rpthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

rpthread_mutex_t mutex;

void* foo(void * args){

	int x = *((int *)args);
	int y = 0;
	for(y=0;y<10000;y++){

		printf("foo->%d\n",y);

	}

	int * ret = malloc(sizeof(int));
	*(ret) = x+y;
	rpthread_exit((void *)ret);

}
void* poo(void * args){

	int x = *((int *)args);
	int y = 0;
	for(y=0;y<10000;y++){

		printf("poo->%d\n",y);

	}

	int * ret = malloc(sizeof(int));
	*(ret) = x+y;
	rpthread_exit((void *)ret);

}
void * bar(void * args){

	int x = *((int *)args);
	int y = 0;
	for(y=0;y<10000;y++){

		printf("bar->%d\n",y);

	}

	int * ret = malloc(sizeof(int));
	*(ret) = x+y;
	rpthread_exit((void *)ret);

}
void * yeet(void * args){

	int x = *((int *)args);
	int y = 0;
	for(y=0;y<10000;y++){

		printf("yeet->%d\n",y);

	}

	int * ret = malloc(sizeof(int));
	*(ret) = x+y;
	rpthread_exit((void *)ret);

}

int counter = 0;
void * m_test(void * args) {

	rpthread_mutex_lock(&mutex);

	unsigned long i = 0;
	counter += 1;
	printf("\n Job %d started\n", counter);

	for (i = 0; i < (0xFFFFFFFF); i++);
	printf("\n Job %d finished\n", counter);

	rpthread_mutex_unlock(&mutex);

	rpthread_exit(NULL);
}

int main(int argc, char **argv) {
	rpthread_mutex_init(&mutex, NULL);
	printf("Mutex Initialized\n");
	rpthread_t thread_id; 
	rpthread_t thread_id2;
	rpthread_t thread_id3;
	rpthread_t thread_id4;
	void * p = malloc(sizeof(int));
	*((int*)p) = 50;
	void * p2 = malloc(sizeof(int));
	*((int*)p2) = 70;
	void * p3 = malloc(sizeof(int));
	*((int*)p3) = 80;
	void * p4 = malloc(sizeof(int));
	*((int*)p4) = 20;
	rpthread_create(&thread_id2, NULL, bar, p2);
    rpthread_create(&thread_id, NULL, foo, p);
    rpthread_create(&thread_id3, NULL, yeet, p3);
    rpthread_create(&thread_id4, NULL, poo, p4);
    void *y;
    void *y2;
    void *y3;
    void *y4;

    rpthread_join(thread_id2,&y2);
    rpthread_join(thread_id,&y);
    rpthread_join(thread_id,&y4);
    rpthread_join(thread_id3,&y3);

    printf("%d\n",*((int *)y));
    printf("%d\n",*((int *)y2));
    printf("%d\n",*((int *)y3));
    printf("%d\n",*((int *)y4));

    free(p);
    free(p2);
    free(p3);
    free(p4);
	// rpthread_t t1, t2;
	// rpthread_create(&t1, NULL, m_test, NULL);
	// rpthread_create(&t1, NULL, m_test, NULL);
	// rpthread_join(t1, NULL);
	// rpthread_join(t2, NULL);
	// printf("Destroy\n");
	// rpthread_mutex_destroy(&mutex);

	return 0;
}
