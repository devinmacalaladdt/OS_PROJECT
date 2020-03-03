// File:	rpthread_t.h

// List all group member's name:
// username of iLab:
// iLab Server:

#ifndef RTHREAD_T_H
#define RTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_RTHREAD macro */
#define USE_RTHREAD 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <string.h>
#include <sys/time.h>

typedef uint rpthread_t;

//Constants
enum State { RUNNING = 0, READY = 1, BLOCKED = 2, INITIALIZATION = 3 }; //Initialization = set up structs but doesn't have a function to run


typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread Id
	rpthread_t* TiD;

	// thread status
	unsigned state;

	// thread context
	ucontext_t context;

	// thread stack
	void * stackPtr;

	// thread priority
	unsigned priority; //Realtime 0 - 99?

	// And more ...

	// YOUR CODE HERE
} tcb;


/* mutex struct definition */
typedef struct rpthread_mutex_t {
	/* add something here */

	// YOUR CODE HERE
} rpthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

tcb * initializeTCB(rpthread_t * thread);

typedef struct tcb_nodes{

	tcb *t;
	struct tcb_nodes *next;

}tcb_node;

typedef struct runqueues{

	tcb_node *head;
	tcb_node *tail;
	int size;

}runqueue;

void setup_runqueue(runqueue * rq);
tcb *peek(runqueue * rq);
void enqueue(runqueue * rq, tcb * t);
tcb *dequeue(runqueue * rq);
void handler(int signum);


/* Function Declarations: */

/* create a new thread */
int rpthread_create(rpthread_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int rpthread_yield();

/* terminate a thread */
void rpthread_exit(void *value_ptr);

/* wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr);

/* initial the mutex lock */
int rpthread_mutex_init(rpthread_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int rpthread_mutex_lock(rpthread_mutex_t *mutex);

/* release the mutex lock */
int rpthread_mutex_unlock(rpthread_mutex_t *mutex);

/* destroy the mutex */
int rpthread_mutex_destroy(rpthread_mutex_t *mutex);

#ifdef USE_RTHREAD
#define pthread_t rpthread_t
#define pthread_mutex_t rpthread_mutex_t
#define pthread_create rpthread_create
#define pthread_exit rpthread_exit
#define pthread_join rpthread_join
#define pthread_mutex_init rpthread_mutex_init
#define pthread_mutex_lock rpthread_mutex_lock
#define pthread_mutex_unlock rpthread_mutex_unlock
#define pthread_mutex_destroy rpthread_mutex_destroy
#endif

#endif
