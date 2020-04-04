#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../my_vm.h"

#define SIZE 5
#define ARRAY_SIZE 400

int main(){

    void * a = a_malloc(sizeof(int));
    printf("a:%p\n",a);
    int x = 9;
    int y = 0;
    put_value(a,(void*)&x,sizeof(int));
    get_value(a,(void*)&y,sizeof(int));
    printf("fevdvd\n");
    printf("y:%d\n",y);
    printf("a:%p\n",a);
    a_free(a,sizeof(int));

}