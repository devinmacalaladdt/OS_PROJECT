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

	while(1){

		printf("foo\n");

	}

}
void * bar(void * args){

	while(1){

		printf("bar\n");

	}

}
int main(int argc, char **argv) {

	rpthread_t thread_id; 
	rpthread_t thread_id2; 
    printf("Before Thread\n"); 
    pthread_create(&thread_id, NULL, foo, NULL); 
    pthread_create(&thread_id2, NULL, bar, NULL); 

    while(1){}
	return 0;
}
