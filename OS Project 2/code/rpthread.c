// File:	rpthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "rpthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE


/* create a new thread
* return 0 for success, 1 for error
*/

runqueue * rq_ptr = NULL;
runqueue rq;

int rpthread_create(rpthread_t * thread, pthread_attr_t * attr,
	void *(*function)(void*), void * arg) {
	// Create Thread Control Block
	// Create and initialize the context of this thread
	// Allocate space of stack for this thread to run
	// after everything is all set, push this thread int
	// YOUR CODE HERE
	tcb * _tcb = initializeTCB();
	if (_tcb == NULL)
		return 1;
	if(rq_ptr==NULL){
		rq_ptr = (runqueue*)(malloc(sizeof(runqueue)));
		setup_runqueue(rq_ptr);
		rq = *rq_ptr;
		struct sigaction sa;
		memset (&sa, 0, sizeof (sa));
		sa.sa_handler = &schedule;
		sigaction (SIGPROF, &sa, NULL);

		struct itimerval timer;

		timer.it_interval.tv_usec = 5000;
		timer.it_interval.tv_sec = 0;

		timer.it_value.tv_usec = 0;
		timer.it_value.tv_sec = 5000;

		setitimer(ITIMER_PROF, &timer, NULL);
	}
	makecontext(_tcb->stackPtr,function,1,void*);
	_tcb->state = READY;
	enqueue(rq_ptr);


    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int rpthread_yield() {
	
	// Change thread state from Running to Ready
	// Save context of this thread to its thread control block
	// switch from thread context to scheduler context

	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void rpthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
};


/* Wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr) {
	
	// Wait for a specific thread to terminate
	// De-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int rpthread_mutex_init(rpthread_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//Initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int rpthread_mutex_lock(rpthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // When the mutex is acquired successfully, enter the critical section
        // If acquiring mutex fails, push current thread into block list and 
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int rpthread_mutex_unlock(rpthread_mutex_t *mutex) {
	// Release mutex and make it available again. 
	// Put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int rpthread_mutex_destroy(rpthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in rpthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library 
	// should be contexted switched from thread context to this 
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE




// schedule policy
#ifndef MLFQ
	// Choose STCF
#else 
	// Choose MLFQ
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE

/*
* Return a pointer to the initialized TCB
* Null for error
*/
tcb * initializeTCB() {
	tcb * tcb = malloc(sizeof(tcb));
	if (tcb == NULL)
		return NULL;
	tcb->TiD = openTiD++;
	tcb->priority = DEFAULT_PRIORITY;
	stackPtr = (void *)malloc(SIGSTKSZ);
	if (stackPtr == NULL)
		return NULL;
	tcb->stackPtr = stackPtr;
	ucontext_t context;
	if (getcontext(&context) < 0)
		return NULL;
	context.uc_link = NULL;
	context.uc_stack.ss_sp = stackPtr;
	context.uc_stack.ss_size = SIGSTKSZ;
	context.uc_stack.ss_flags = 0;
	tcb->context = context;
	tcb->state = INITIALIZATION;
}

void setup_runqueue(runqueue * rq){

	rq->size=0;
	rq->head = NULL;
	rq->tail = NULL;

}

tcb *peek(runqueue * rq){

	return (rq->head)->t;

}

void enqueue(runqueue * rq, tcb * t){

	if(rq->head==NULL){

		rq->head = (tcb_node*)(malloc(sizeof(tcb_node)));
		(rq->head)->t = t;
		(rq->head)->next = NULL;
		rq->tail = rq->head;
		rq->size++;
		return;

	}

	(rq->tail)->next = (tcb_node*)(malloc(sizeof(tcb_node)));
	((rq->tail)->next)->t = t;
	rq->tail = (rq->tail)->next;
	rq->size++;

}

tcb *dequeue(runqueue * rq){

	if(rq->head==NULL){

		return NULL;

	}

	tcb *tmp = (rq->head)->t;
	tcb_node *tmp2 = rq->head;
	rq->head = (rq->head)->next;
	free(tmp2);
	rq->size--;
	return tmp;


}
