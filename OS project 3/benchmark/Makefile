
all : test
test: ../my_vm.h
	gcc test2.c -L../ -lmy_vm -m32 -lm -lpthread -g -o test

clean:
	rm -rf test
