#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../my_vm.h"

#define SIZE 5
#define ARRAY_SIZE 400

void print_thread(void* start){

    printf("a\n");
    void * a = a_malloc(sizeof(int));
printf("%u\n", ((unsigned) a));
    printf("b\n");
    int x = *((int*)start);
    int y = 0;
    put_value(a,(void*)&x,sizeof(int));
    printf("c\n");
    get_value(a,(void*)&y,sizeof(int));
    printf("d\n");
    printf("%d\n",y);
    a_free(a,sizeof(int));
    printf("e\n");

}

int main(){

    pthread_t threads[10];
    int c = 0;
    for(c=0;c<10;c++){
	void * num = malloc(sizeof(int));
	*((int*)num) = c;
        pthread_create(&(threads[c]), NULL, print_thread, num); 

    }



}
