#include "prometheus.h"
#include <stdio.h>

int main(int arc, char** argv) {
	char* labels[2];
	int i;
	labels[0] = "hostname";
	labels[1] = "username";
	if(!new_counter_vec("number_of_starts", "This is a number of program starts", labels, 2)) {
		fprintf(stderr, "new_counter_vec failed\n");
		return(-1);
	}
	fprintf(stderr, "new_counter_vec succeeded\n");
	for(i=0; i< 10; i++) {
		if(!increment_counter("number_of_starts", labels, 2)) {
			fprintf(stderr, "increment_counter failed\n");
			return(-1);
		}
		fprintf(stderr, "increment_counter succeeded\n");
	}
	return(0);
}
