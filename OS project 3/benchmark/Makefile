
all : test
test: ../my_vm.h
	gcc test.c -L../ -lmy_vm -m32 -lm -g -o test

clean:
	rm -rf test
