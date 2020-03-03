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
void foo(){

	while(1){

		printf("foo\n");

	}

}
void bar(){

	while(1){

		printf("bar\n");

	}

}
int main(int argc, char **argv) {

	rpthread_t thread_id; 
    printf("Before Thread\n"); 
    pthread_create(&thread_id, NULL, foo, NULL); 

    while(1){}
	return 0;
}
