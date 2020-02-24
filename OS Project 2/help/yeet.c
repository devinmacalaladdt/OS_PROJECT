#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <ucontext.h>
#include <string.h>
#include <sys/time.h>

#define STACK_SIZE SIGSTKSZ

ucontext_t fctx,bctx,nctx;

int check = 0;

void swapper(int signum){

	check = check==0?1:0;

	if(check==0){

		swapcontext(&fctx,&bctx);

	}else{

		swapcontext(&bctx,&fctx);		

	}
	

}

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

int main(){

	void *stackf=malloc(STACK_SIZE);
	void *stackb=malloc(STACK_SIZE);
	
    if (getcontext(&fctx) < 0){
		perror("getcontext");
		exit(1);
	}
	if (getcontext(&bctx) < 0){
		perror("getcontext");
		exit(1);
	}

	fctx.uc_link=NULL;
	fctx.uc_stack.ss_sp=stackf;
	fctx.uc_stack.ss_size=STACK_SIZE;
	fctx.uc_stack.ss_flags=0;

	bctx.uc_link=NULL;
	bctx.uc_stack.ss_sp=stackb;
	bctx.uc_stack.ss_size=STACK_SIZE;
	bctx.uc_stack.ss_flags=0;
	
	
	makecontext(&fctx,(void *)&foo,0);
	makecontext(&bctx,(void *)&bar,0);

	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &swapper;
	sigaction (SIGPROF, &sa, NULL);

	struct itimerval timer;

	timer.it_interval.tv_usec = 500;
	timer.it_interval.tv_sec = 0;

	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 1;

	setitimer(ITIMER_PROF, &timer, NULL);

	swapcontext(&nctx,&bctx);

	while(1);

	return 0;

}