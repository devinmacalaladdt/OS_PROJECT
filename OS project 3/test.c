#include "my_vm.h"

int main() {
	void* p1 = a_malloc(1024);
	void* p2 = a_malloc(50);
	void* p3 = a_malloc(5000);
	void* p4 = a_malloc(2500);
	a_free(p3, 5000);
	a_free(p1, 1024);
	a_free(p2, 50);
	a_free(p4, 2500);
	return 0;
}
