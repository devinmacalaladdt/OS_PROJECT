#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../my_vm.h"

#define SIZE 5
#define ARRAY_SIZE 400

int main(){

    void * a = a_malloc(sizeof(int)*2);
    printf("1 a:%p\n",a);
    int x = 9;
    int y = 0;//8049050
    put_value(a,(void*)&x,sizeof(int));
    printf("2 a:%p\n",a);
    get_value(a,(void*)&y,sizeof(int));
    printf("3 a:%p\n",a);
    printf("y:%d\n",y);
    printf("4 a:%p\n",a);
    a_free(a,sizeof(int));

}