// File:	rpthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "rpthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

//global schedule context
ucontext_t sched_context;
//Highest available TiD (assumes no re-use)
rpthread_t openTiD = 0;
//run queues
//rq 0: 1 time slice, rq 1: 2 time slices, rq 2: 4 time slices, rq 3: 8+time slices
runqueue * rq_ptr[4] = {NULL,NULL,NULL,NULL};


//timer
struct itimerval timer;
//SJF = 0, MLFQ = 1
short mode=0;
//current running runqueue
short curr_rq = 0;
static void schedule();
//pause timer on enter of critical section
int unInterMode = 0;
static int TestAndSet(volatile int *);
static void sched_stcf();
static void sched_mlfq();

/* create a new thread
* return 0 for success, 1 for error
*/
int rpthread_create(rpthread_t * thread, pthread_attr_t * attr,
	void *(*function)(void*), void * arg) {
	// Create Thread Control Block
	// Create and initialize the context of this thread
	// Allocate space of stack for this thread to run
	// after everything is all set, push this thread int
	// YOUR CODE HERE

	if((rq_ptr[0])!=NULL){
		//pause timer
		timer.it_value.tv_usec = 0;
	}
	tcb * _tcb = initializeTCB(thread);
	if (_tcb == NULL)
		return -1;
	if((rq_ptr[0])==NULL){

		//setting schedule type
		#ifndef MLFQ
			mode = 0;
		#else 
			mode = 1;
		#endif
		//first thread, set up queue or queues if MLFQ
		(rq_ptr[0]) = (runqueue*)(malloc(sizeof(runqueue)));
		setup_runqueue((rq_ptr[0]));
		if(mode==1){

			(rq_ptr[1]) = (runqueue*)(malloc(sizeof(runqueue)));
			setup_runqueue((rq_ptr[1]));
			(rq_ptr[2]) = (runqueue*)(malloc(sizeof(runqueue)));
			setup_runqueue((rq_ptr[2]));
			(rq_ptr[3]) = (runqueue*)(malloc(sizeof(runqueue)));
			setup_runqueue((rq_ptr[3]));

		}

		//set up schedule context
		void *stackPtr = (void *)malloc(SIGSTKSZ);
		if (stackPtr == NULL)
			return -1;
		if (getcontext(&sched_context) < 0)
			return -1;
		sched_context.uc_link = NULL;
		sched_context.uc_stack.ss_sp = stackPtr;
		sched_context.uc_stack.ss_size = SIGSTKSZ;
		sched_context.uc_stack.ss_flags = 0;
		makecontext(&sched_context,(void *)&schedule,0);

		//timer setup
		struct sigaction sa;
		memset (&sa, 0, sizeof (sa));
		sa.sa_handler = &handler;
		sigaction (SIGPROF, &sa, NULL);

		timer.it_interval.tv_usec = 5000;
		timer.it_interval.tv_sec = 0;

		timer.it_value.tv_usec = 1;
		timer.it_value.tv_sec = 0;


		//context of calling function created + queued
		tcb * calling = malloc(sizeof(tcb));
		calling->TiD = openTiD++;
		//calling context already running
		calling->state = RUNNING;
		calling->quantum = 0;
		calling->quantums_since_last_schedule = 0;
		enqueue((rq_ptr[0]),setup_tcb_node(calling));

		//specified context created + queued
		makecontext(&(_tcb->context),(void(*)())function,1,arg);
		_tcb->state = READY;
		enqueue((rq_ptr[0]),setup_tcb_node(_tcb));

		//start timer
		setitimer(ITIMER_PROF, &timer, NULL);

		return 0;
	}

	//specified context created + queued
	makecontext(&(_tcb->context),(void(*)())function,1,arg);
	_tcb->state = READY;
	enqueue((rq_ptr[0]),setup_tcb_node(_tcb));
	timer.it_value.tv_usec = 1;
	setitimer(ITIMER_PROF, &timer, NULL);

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int rpthread_yield() {
	
	// Change thread state from Running to Ready
	// Save context of this thread to its thread control block

	//pause timer
	timer.it_value.tv_usec = 0;
	//set currently running to ready then context switch to scheduler
	(((rq_ptr[curr_rq])->head)->t)->state=READY;
	swapcontext(&((((rq_ptr[curr_rq])->head)->t)->context),&sched_context);

	return 0;
};

/* terminate a thread */
void rpthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread
	//pause timer
	timer.it_value.tv_usec = 0;
	
	//ready / unblock all waiting join threads if any
	tcb_node *joinList = ((rq_ptr[curr_rq])->head)->joined_on;
	while (joinList != NULL) {
		(joinList->t)->state = READY;
		if (value_ptr != NULL) //caller wants a return value
			*(joinList->joined_on_ret) = value_ptr;
		tcb_node *x = joinList->next;
		free(joinList); 
		joinList = x;
	}


	//free stack frame
	free((((rq_ptr[curr_rq])->head)->t)->stackPtr);

	//set ret val to value_ptr for future threads that join this one
	(((rq_ptr[curr_rq])->head)->ret_val) = value_ptr;
	(((rq_ptr[curr_rq])->head)->t)->state = DESTROYED;

	//release mutexes?

	//context switch to scheduler
	setcontext(&sched_context);

	// YOUR CODE HERE
};

/* Wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr) {
	
	// Wait for a specific thread to terminate
	// De-allocate any dynamic memory created by the joining thread

	//pause timer
	timer.it_value.tv_usec = 0;
	if((rq_ptr[curr_rq])==NULL){

		timer.it_value.tv_usec = 1;
		setitimer(ITIMER_PROF, &timer, NULL);
		return -1;

	}

	int set_blocked = 1;
	int found = 0;
	int count = 0;
	int bound = 1;
	if(mode==1){bound=4;}

	for(count=0;count<bound;count++){

		tcb_node * curr = (rq_ptr[count])->head;

		//iterate through runqueue, look for desired thread
		do{

			if(curr==NULL){break;}
			if(((curr->t)->TiD)==thread){

				//thread found. If no current threads are joined on it, start the list. If its destroyed,
				//set value_ptr and make sure this thread isnt set to BLOCKED. If other threads already
				//joined, append to list
				found = 1;
				if((curr->t)->state==DESTROYED){

					if(value_ptr!=NULL){

						*value_ptr = curr->ret_val;

					}
					set_blocked = 0;

				}else if(curr->joined_on==NULL){

					curr->joined_on = setup_tcb_node(((rq_ptr[curr_rq])->head)->t);

					//set the joined_on_ret to value_ptr if its not null
					if(value_ptr!=NULL){
					
						(curr->joined_on)->joined_on_ret = value_ptr;
					
					}


				}else{

					//thread found, already others joined on it, so add to back of list
					tcb_node *curr2 = curr->joined_on;
					while(curr2->next!=NULL){

						curr2 = curr2->next;

					}
					curr2->next = setup_tcb_node(((rq_ptr[curr_rq])->head)->t);
					//set the joined_on_ret to value_ptr if its not null
					if(value_ptr!=NULL){

						(curr2->next)->joined_on_ret = value_ptr;

					}



				}

				break;

			}

			curr = curr->next;

		}while(curr!=(rq_ptr[count])->head);


	}


	//error
	if(found==0){

		timer.it_value.tv_usec = 1;
		setitimer(ITIMER_PROF, &timer, NULL);
		return -1;

	}

	//set status of current thread to blocked and context switch to scheduler
	if(set_blocked==1){

		(((rq_ptr[curr_rq])->head)->t)->state = BLOCKED;

	}

	//swap to scheduler
	swapcontext(&((((rq_ptr[curr_rq])->head)->t)->context),&sched_context);
  
	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock 
0 success
other for error
*/
int rpthread_mutex_init(rpthread_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//Initialize data structures for this mutex
	//mutex = (rpthread_mutex_t*)malloc(sizeof(rpthread_mutex_t));
	if (mutex == NULL)
		return 1;
	(mutex)->condition_variable = 0; // unlocked state
	//mutex->ownerTID = (((rq_ptr[0])->head)->t)->TiD;
	if (mutexattr == NULL)
		(mutex)->mutex_attr = 0; //some default value
	(mutex)->blockingOnLock = NULL;
	//specify other attributes as needed

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int rpthread_mutex_lock(rpthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // When the mutex is acquired successfully, enter the critical section
        // If acquiring mutex fails, push current thread into block list and 
        // context switch to the scheduler thread

		//pause scheduler for non-interrupted testandset
		unInterMode = 1;
		while(TestAndSet(&(mutex->condition_variable))) { //while for spin-mode
			unInterMode = 0;
			//block until lock is available
			//set status of current thread to blocked and context switch
			(((rq_ptr[curr_rq])->head)->t)->state = BLOCKED;
			//create blocked tcb
			state_list *t = (void *)malloc(sizeof(state_list));
			t->state = &((((rq_ptr[curr_rq])->head)->t)->state);
			t->next = NULL;

			//add to list
			if (mutex->blockingOnLock == NULL) {
				mutex->blockingOnLock = t;
			}
			else //add to back
			{
				state_list * counter = mutex->blockingOnLock;
				while (counter->next != NULL)
					counter = counter->next; //find end
				counter->next = t;
			}
			timer.it_value.tv_usec = 0;
			swapcontext(&((((rq_ptr[curr_rq])->head)->t)->context), &sched_context);
		}
		unInterMode = 0;
		/*lock aquired*/
		mutex->ownerTID = (((rq_ptr[curr_rq])->head)->t)->TiD;
        // YOUR CODE HERE

		//do critical section stuff
        return 0;
};

/* release the mutex lock 
Success 0
Error 1
*/
int rpthread_mutex_unlock(rpthread_mutex_t *mutex) {
	// Release mutex and make it available again. 
	// Put threads in block list to run queue 
	// so that they could compete for mutex later.

	if (mutex->ownerTID == (((rq_ptr[curr_rq])->head)->t)->TiD){
		mutex->condition_variable = 0;
		if (mutex->blockingOnLock != NULL) { //ready first in queue
			*((mutex->blockingOnLock)->state) = READY;
			state_list * temp = (mutex->blockingOnLock)->next;
			free((void*)mutex->blockingOnLock);
			mutex->blockingOnLock = temp;
		}
		return 0;
	}
	return 1; //not owner of the lock
	// YOUR CODE HERE

};

//Implement user level test and set
static int TestAndSet(volatile int * target) {
	int oldTarget = *target;
	if (*target == 0)
		*target = 1;
	return oldTarget;
}


/* destroy the mutex 
0 success
other for error*/
int rpthread_mutex_destroy(rpthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in rpthread_mutex_init
	if (mutex == NULL)
		return 1;
	rpthread_mutex_unlock(mutex);
	mutex->ownerTID = 0;
	if (mutex->blockingOnLock != NULL)
	{
		state_list *temp;
		do {
			*((mutex->blockingOnLock)->state) = READY;
			temp = (mutex->blockingOnLock)->next;
			free((void*)mutex->blockingOnLock);
		} while (temp != NULL);
	}
	//free((void*)mutex);
	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library 
	// should be contexted switched from thread context to this 
	// schedule function

	if(mode==0){

		sched_stcf();

	}else{

		sched_mlfq();

	}

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)
	// If the current thread is running, set it to ready to be queued

	//find READY job with least quantum and set it to swap, along with what comes before to prev_swap
	tcb_node * swap = NULL;
	tcb_node * prev_swap = NULL;
	unsigned long min = (((rq_ptr[curr_rq])->head)->t)->quantum;
	tcb_node * curr = ((rq_ptr[curr_rq])->head)->next;
	tcb_node * prev = (rq_ptr[curr_rq])->head;

	while(curr!=(rq_ptr[curr_rq])->head){

		if((curr->t)->state==READY && (curr->t)->quantum<=min){

			swap = curr;
			prev_swap = prev;
			min = (curr->t)->quantum;

		}

		prev = curr;
		curr = curr->next;

	}

	//could not find job with less than or equal quantum to whats currently running, so reschedule current job
	if(swap==NULL && (((rq_ptr[curr_rq])->head)->t)->state==RUNNING){

		timer.it_value.tv_usec = 1;
		setitimer(ITIMER_PROF, &timer, NULL);
		setcontext(&((peek((rq_ptr[curr_rq])))->context));

	}

	//could not find job with less than or equal quantum to whats currently in front of queue,
	//but whats in front has either yeilded, joined, or exited, so enqueue it and schedule next runnable job 
	if(swap==NULL && (((rq_ptr[curr_rq])->head)->t)->state!=RUNNING){

		enqueue((rq_ptr[curr_rq]),dequeue((rq_ptr[curr_rq])));

		// keep enqueueing the dequeued thread until one is ready
		while(peek((rq_ptr[curr_rq]))->state!=READY){

			enqueue((rq_ptr[curr_rq]),dequeue((rq_ptr[curr_rq])));

		}
		(((rq_ptr[curr_rq])->head)->t)->state = RUNNING;
		timer.it_value.tv_usec = 1;
		setitimer(ITIMER_PROF, &timer, NULL);
		setcontext(&((peek((rq_ptr[curr_rq])))->context));

	}

	//current front of queue job has either yeilded, joined, or exited, so enqueue it and swap in
	//the job with lowest quantum
	if(swap!=NULL && (((rq_ptr[curr_rq])->head)->t)->state!=RUNNING){

		if(swap==((rq_ptr[curr_rq])->head)->next){

			enqueue((rq_ptr[curr_rq]),dequeue((rq_ptr[curr_rq])));
			(((rq_ptr[curr_rq])->head)->t)->state = RUNNING;
			timer.it_value.tv_usec = 1;
			setitimer(ITIMER_PROF, &timer, NULL);
			setcontext(&((peek((rq_ptr[curr_rq])))->context));

		}

		enqueue((rq_ptr[curr_rq]),dequeue((rq_ptr[curr_rq])));
		prev_swap->next = swap->next;
		enqueue_start((rq_ptr[curr_rq]),swap);
		(((rq_ptr[curr_rq])->head)->t)->state = RUNNING;
		timer.it_value.tv_usec = 1;
		setitimer(ITIMER_PROF, &timer, NULL);
		setcontext(&((peek((rq_ptr[curr_rq])))->context));


	}

	//current front of queue job has used its alloted runtime, so set it to READY, enqueue it and swap in
	//the job with lowest quantum
	if(swap!=NULL && (((rq_ptr[curr_rq])->head)->t)->state==RUNNING){


		if(swap==((rq_ptr[curr_rq])->head)->next){

			(((rq_ptr[curr_rq])->head)->t)->state=READY;
			enqueue((rq_ptr[curr_rq]),dequeue((rq_ptr[curr_rq])));
			(((rq_ptr[curr_rq])->head)->t)->state = RUNNING;
			timer.it_value.tv_usec = 1;
			setitimer(ITIMER_PROF, &timer, NULL);
			setcontext(&((peek((rq_ptr[curr_rq])))->context));
			
		}


		(((rq_ptr[curr_rq])->head)->t)->state=READY;
		enqueue((rq_ptr[curr_rq]),dequeue((rq_ptr[curr_rq])));
		prev_swap->next = swap->next;
		enqueue_start((rq_ptr[curr_rq]),swap);
		(((rq_ptr[curr_rq])->head)->t)->state = RUNNING;
		timer.it_value.tv_usec = 1;
		setitimer(ITIMER_PROF, &timer, NULL);
		setcontext(&((peek((rq_ptr[curr_rq])))->context));

	}


	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)


	//Only need to promote queues lower than the queue currently running,
	//as jobs in queues before are either blocked, destroyed, or have yielded
	if(curr_rq!=0){

		int c = curr_rq;
		while(c<4){

			if(rq_ptr[c]==NULL){continue;}
			tcb_node * start = (rq_ptr[c])->head;
			do{

				if((rq_ptr[c])->head==NULL){break;}
				//job should be promoted
				if(((rq_ptr[c]->head)->t)->state==READY && (((rq_ptr[c])->head)->t)->quantums_since_last_schedule>=50){

					(((rq_ptr[c])->head)->t)->quantums_since_last_schedule=0;
					enqueue(rq_ptr[c-1],dequeue(rq_ptr[c]));

				}else{
					//job shouldnt be promoted, increment its quantum since last scheduled
					//if its READY
					if(((rq_ptr[c]->head)->t)->state==READY){

						(((rq_ptr[c])->head)->t)->quantums_since_last_schedule++;

					}

					enqueue(rq_ptr[c],dequeue(rq_ptr[c]));

				}

			}while((rq_ptr[c])->head!=start);
			c++;

		}

	}

	//if the current job has blocked, yielded, or been destroyed, just put it on the end
	if((((rq_ptr[curr_rq])->head)->t)->state!=RUNNING){

		enqueue(rq_ptr[curr_rq],dequeue(rq_ptr[curr_rq]));
		
	}else{

		//set running job to ready, and place it into queue for specific quantum

		(((rq_ptr[curr_rq])->head)->t)->state=READY;

		if((((rq_ptr[curr_rq])->head)->t)->quantum<1){

			enqueue(rq_ptr[0],dequeue(rq_ptr[curr_rq]));

		}else if((((rq_ptr[curr_rq])->head)->t)->quantum<2){

			enqueue(rq_ptr[1],dequeue(rq_ptr[curr_rq]));

		}else if((((rq_ptr[curr_rq])->head)->t)->quantum<4){

			enqueue(rq_ptr[2],dequeue(rq_ptr[curr_rq]));

		}else{

			enqueue(rq_ptr[3],dequeue(rq_ptr[curr_rq]));

		}

	}

	//find next runnable job
	int rq = 0;
	for(rq=0;rq<4;rq++){

		tcb_node * start = (rq_ptr[rq])->head;
		short br = 0;
		if(start==NULL){continue;}
		do{

			if((((rq_ptr[rq])->head)->t)->state==READY){br=1;break;}
			enqueue(rq_ptr[rq],dequeue(rq_ptr[rq]));

		}while((rq_ptr[rq])->head!=start);

		if(br){break;}


	}

	//found job, set current queue, set state to RUNNING, and set quantum
	//and quantum since last schedule to 0
	curr_rq = rq;
	(((rq_ptr[curr_rq])->head)->t)->state = RUNNING;
	(((rq_ptr[curr_rq])->head)->t)->quantums_since_last_schedule=0;
	(((rq_ptr[curr_rq])->head)->t)->quantum=0;
	timer.it_value.tv_usec = 1;
	setitimer(ITIMER_PROF, &timer, NULL);
	setcontext(&((peek((rq_ptr[curr_rq])))->context));


	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE

/*
* Return a pointer to the initialized TCB
* Null for error
*/
tcb * initializeTCB(rpthread_t * thread) {
	tcb * _tcb = (tcb*)malloc(sizeof(tcb));
	if (_tcb == NULL)
		return NULL;
	*thread = openTiD++;
	_tcb->TiD = *thread;
	_tcb->priority = INITIALIZATION;
	void* stackPtr = (void *)malloc(SIGSTKSZ);
	if (stackPtr == NULL)
		return NULL;
	_tcb->stackPtr = stackPtr;
	ucontext_t context;
	if (getcontext(&context) < 0)
		return NULL;
	context.uc_link = NULL;
	context.uc_stack.ss_sp = stackPtr;
	context.uc_stack.ss_size = SIGSTKSZ;
	context.uc_stack.ss_flags = 0;
	_tcb->context = context;
	_tcb->state = INITIALIZATION;
	_tcb->quantum = 0;
	_tcb->quantums_since_last_schedule = 0;
	return _tcb;
}

void setup_runqueue(runqueue * rq){

	rq->size=0;
	rq->head = NULL;
	rq->tail = NULL;

}

tcb_node * setup_tcb_node(tcb * t){

	tcb_node *ret = (tcb_node*)(malloc(sizeof(tcb_node)));
	ret->t = t;
	ret->joined_on = NULL;
	ret->joined_on_ret = NULL;
	ret->ret_val = NULL;
	ret->next = NULL;
	return ret;

}

tcb *peek(runqueue * rq){

	return (rq->head)->t;

}

void enqueue(runqueue * rq, tcb_node * t){

	if(rq->head==NULL){

		rq->head = t;
		(rq->head)->next = NULL;
		rq->tail = rq->head;
		rq->size++;
		return;

	}

	(rq->tail)->next = t;
	rq->tail = t;
	(rq->tail)->next = rq->head;
	rq->size++;

}
void enqueue_start(runqueue * rq, tcb_node * t){

	if(rq->head==NULL){

		rq->head = t;
		(rq->head)->next = NULL;
		rq->tail = rq->head;
		rq->size++;
		return;

	}

	(rq->tail)->next = t;
	t->next = rq->head;
	rq->head = t;
	rq->size++;

}


tcb_node *dequeue(runqueue * rq){

	if(rq->head==NULL){

		return NULL;

	}

	if(rq->size==1){

		tcb_node * tmp = rq->head;
		rq->head = NULL;
		rq->tail = NULL;
		rq->size=0;
		return tmp;

	}

	tcb_node *tmp = rq->head;
	rq->head = (rq->head)->next;
	(rq->tail)->next = rq->head;
	rq->size--;
	return tmp;


}

void handler(int signum){
	while (unInterMode);//prevent context switch if in atomic critical section mode

	timer.it_value.tv_usec = 0;

	//increment quantum if current job has allowed timer to go off
	(((rq_ptr[curr_rq])->head)->t)->quantum = (((rq_ptr[curr_rq])->head)->t)->quantum + 1;

	//save current thread and context switch to scheduler
	swapcontext(&((((rq_ptr[curr_rq])->head)->t)->context),&sched_context);

}

