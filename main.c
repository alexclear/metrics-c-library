#include "prometheus.h"
#include <stdio.h>

int main(int arc, char** argv) {
	char* label_names[2];
	int i;
	label_names[0] = "hostname";
	label_names[1] = "username";

	char* labels[2];
	labels[0] = "127.0.0.1";
	labels[1] = "student";

	if(!new_counter_vec("number_of_starts", "This is a number of program starts", label_names, 2)) {
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

	double margins[50];
	margins[0] = 1;
	margins[1] = 2;
	margins[2] = 3;
	margins[3] = 4;
	margins[4] = 5;
	margins[5] = 6;
	margins[6] = 7;
	margins[7] = 8;
	margins[8] = 9;
	margins[9] = 10;
	margins[10] = 11;
	margins[11] = 12;
	margins[12] = 13;
	margins[13] = 16;
	margins[14] = 18;
	margins[15] = 20;
	margins[16] = 25;
	margins[17] = 30;
	margins[18] = 35;
	margins[19] = 40;
	margins[20] = 45;
	margins[21] = 50;
	margins[22] = 55;
	margins[23] = 60;
	margins[24] = 65;
	margins[25] = 70;
	margins[26] = 80;
	margins[27] = 90;
	margins[28] = 100;
	margins[29] = 120;
	margins[30] = 140;
	margins[31] = 160;
	margins[32] = 180;
	margins[33] = 200;
	margins[34] = 220;
	margins[35] = 240;
	margins[36] = 260;
	margins[37] = 280;
	margins[38] = 300;
	margins[39] = 350;
	margins[40] = 400;
	margins[41] = 450;
	margins[42] = 500;
	margins[43] = 1000;
	margins[44] = 1500;
	margins[45] = 2000;
	margins[46] = 3000;
	margins[47] = 5000;
	margins[48] = 10000;
	margins[49] = 15000;
	if(!new_histogram_vec("latency_example", "This is a sample for a histogram metric", label_names, 2, margins, 50)) {
		fprintf(stderr, "new_histogram_vec failed\n");
		return(-1);
	}
	fprintf(stderr, "new_histogram_vec succeeded (in main(...))\n");

	if(!observe_histogram("latency_example", labels, 2, 20.3)) {
		fprintf(stderr, "observe_histogram failed\n");
		return(-1);
	}
	fprintf(stderr, "observe_histogram succeeded\n");

	if(!observe_histogram("latency_example", labels, 2, 41.2)) {
		fprintf(stderr, "observe_histogram failed\n");
		return(-1);
	}
	fprintf(stderr, "observe_histogram succeeded\n");

	if(!observe_histogram("latency_example", labels, 2, 55.8)) {
		fprintf(stderr, "observe_histogram failed\n");
		return(-1);
	}
	fprintf(stderr, "observe_histogram succeeded\n");

	if(!increment_counter("number_of_starts", labels, 2)) {
		fprintf(stderr, "increment_counter failed\n");
		return(-1);
	}
	fprintf(stderr, "increment_counter succeeded\n");

	print_metrics();

	return(0);
}
