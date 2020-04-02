#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "../my_vm.h"

#define SIZE 5
#define ARRAY_SIZE 400

int main(){


    set_physical_mem();
    page_map(0,0);

}