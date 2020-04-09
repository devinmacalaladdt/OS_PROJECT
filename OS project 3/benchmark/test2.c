#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../my_vm.h"

#define SIZE 5
#define ARRAY_SIZE 400

void print_thread(void* start){

    void * a = a_malloc(sizeof(int));
    int x = *((int*)start);
    int y = 0;
    put_value(a,(void*)&x,sizeof(int));
    get_value(a,(void*)&y,sizeof(int));
    printf("s:%d, e:%d\n",x,y);
    a_free(a,sizeof(int));

}

int main(){

    pthread_t threads[10];
    int * num = malloc(sizeof(int)*10);
    int c = 0;
    for(c=0;c<10;c++){

	    num[c]=c;
        pthread_create(&(threads[c]), NULL, print_thread, num+c);

    }
    for(c=0;c<10;c++){

        pthread_join(&(threads[c]), NULL);

    }
    print_TLB_missrate();



}
