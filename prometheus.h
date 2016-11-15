#ifndef PROMETHEUS_H
#define PROMETHEUS_H
int new_counter_vec(char* name, char* help, char** labels, int nlabels);
int increment_counter(char* name, char** labels, int nlabels);
int new_histogram_vec(char* name, char* help, char** labels, int nlabels, double* bucket_margins, int nbuckets);
int observe_histogram(char* name, char** labels, int nlabels, double value);
int print_metrics();
#endif
