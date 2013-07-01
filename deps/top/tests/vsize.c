#include <stdlib.h>
#include <unistd.h>

int main() {
	malloc(127 * 1024 * 1024L * 1024L * 1024L); sleep(5000);
	return EXIT_SUCCESS;
}
