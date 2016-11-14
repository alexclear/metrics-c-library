#ifndef PROMETHEUS_H
#define PROMETHEUS_H
int new_counter_vec(char* name, char* help, char** labels, int nlabels);
int increment_counter(char* name, char** labels, int nlabels);
#endif
