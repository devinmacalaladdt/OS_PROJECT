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
int main(int argc, char **argv) {

	rpthread_t thread_id; 
	rpthread_t thread_id2;
	rpthread_t thread_id3;
	void * p = malloc(sizeof(int));
	*((int*)p) = 50;
	void * p2 = malloc(sizeof(int));
	*((int*)p2) = 70;
	void * p3 = malloc(sizeof(int));
	*((int*)p3) = 80;
	rpthread_create(&thread_id2, NULL, bar, p2);
    rpthread_create(&thread_id, NULL, foo, p);
    rpthread_create(&thread_id3, NULL, yeet, p3);
    void *y;
    void *y2;
    void *y3;

    rpthread_join(thread_id2,&y2);
    rpthread_join(thread_id,&y);
    rpthread_join(thread_id3,&y3);

    printf("%d\n",*((int *)y));
    printf("%d\n",*((int *)y2));
    printf("%d\n",*((int *)y3));

    free(p);
    free(p2);
    free(p3);

	return 0;
}
