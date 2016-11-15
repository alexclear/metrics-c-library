#include "prometheus.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

GHashTable *metrics_storage = NULL;

typedef struct {
	char *help;
	GHashTable *labeled_metric;
} Metric;

typedef struct {
	double margin;
} Bucket;

typedef struct {
	unsigned int number;
	Bucket** internal_buckets;
} Buckets;

int new_counter_vec(char* name, char* help, char** labels, int nlabels) {
	int i=0;
	// Initialize a hash table
	// TODO: do it only once (set a lock)
	if(metrics_storage == NULL) {
		metrics_storage = g_hash_table_new(g_str_hash, g_str_equal);
	}

	// TODO: make this thread-safe
	Metric *counter = g_hash_table_lookup(metrics_storage, name);
	if(counter != NULL) {
		fprintf(stderr, "Can't register a counter named %s because there is a metric with that name already!\n", name);
		return FALSE;
	}

	counter = (Metric*) malloc(sizeof(Metric));
	(*counter).help = help;
	(*counter).labeled_metric = g_hash_table_new(&g_string_hash, &g_string_equal);
	int* val = (int *) malloc(sizeof(int));
	(*val) = 0;
	GString *key_string = g_string_new("");
	for(i=0; i<nlabels; i++) {
		key_string = g_string_append(key_string, labels[i]);
	}
	if(g_hash_table_insert((*counter).labeled_metric, key_string, val) == FALSE) {
		return FALSE;
	}
	if(g_hash_table_insert(metrics_storage, name, counter) == FALSE) {
		return FALSE;
	}
	fprintf(stderr, "new_counter_vec succeeded\n");
}

int increment_counter(char* name, char** labels, int nlabels) {
	int i;
	GString *key_string = g_string_new("");
	for(i=0; i<nlabels; i++) {
		key_string = g_string_append(key_string, labels[i]);
	}
	// TODO: make this thread-safe
	Metric *counter = g_hash_table_lookup(metrics_storage, name);
	if(counter != NULL) {
		int* val = g_hash_table_lookup((*counter).labeled_metric, key_string);
		if(val != NULL) {
			fprintf(stderr, "Old counter value: %i\n", (*val));
		} else {
			fprintf(stderr, "Failed to lookup counter's value, it is NULL\n");
			return FALSE;
		}
		(*val)++;
		g_hash_table_replace((*counter).labeled_metric, key_string, val);
	} else {
		fprintf(stderr, "Failed to lookup a counter, it is NULL\n");
		return FALSE;
	}
	return TRUE;
}

int new_histogram_vec(char* name, char* help, char** labels, int nlabels, double* bucket_margins, int nbuckets) {
	int i=0;
	// Initialize a hash table
	// TODO: do it only once (set a lock)
        //     - not applicable in nginx environment
	if(metrics_storage == NULL) {
		metrics_storage = g_hash_table_new(g_str_hash, g_str_equal);
	}

	// TODO: make this thread-safe
	Metric *histogram = g_hash_table_lookup(metrics_storage, name);
	if(histogram != NULL) {
		fprintf(stderr, "Can't register a histogram named %s because there is a metric with that name already!\n", name);
		return FALSE;
	}

	histogram = malloc(sizeof(Metric));
	(*histogram).help = help;
	(*histogram).labeled_metric = g_hash_table_new(&g_string_hash, &g_string_equal);
	Buckets* buckets = malloc(sizeof(Buckets));
	(*buckets).number = nbuckets;
	(*buckets).internal_buckets = malloc(sizeof(Bucket*) * nlabels);
	for(i=0; i<nbuckets; i++) {
		fprintf(stderr, "Pointer: %d\n", &((*buckets).internal_buckets[i]));
		(*buckets).internal_buckets[i] = malloc(sizeof(Bucket));
		(*((*buckets).internal_buckets[i])).margin = bucket_margins[i];
	}
	GString *key_string = g_string_new("");
	for(i=0; i<nlabels; i++) {
		key_string = g_string_append(key_string, labels[i]);
	}
	if(g_hash_table_insert((*histogram).labeled_metric, key_string, buckets) == FALSE) {
		return FALSE;
	}
	if(g_hash_table_insert(metrics_storage, name, histogram) == FALSE) {
		return FALSE;
	}
	fprintf(stderr, "new_histogram_vec succeeded\n");
}

int observe_histogram(char* name, char** labels, int nlabels, double value) {
	int i;
	Metric *histogram = g_hash_table_lookup(metrics_storage, name);
	if(histogram == NULL) {
		fprintf(stderr, "Can't find a histogram named %s\n", name);
		return FALSE;
	}
	GString *key_string = g_string_new("");
	for(i=0; i<nlabels; i++) {
		key_string = g_string_append(key_string, labels[i]);
	}
	Buckets* buckets = g_hash_table_lookup((*histogram).labeled_metric, key_string);
	if(buckets == NULL) {
		fprintf(stderr, "Failed to find buckets, they are NULL\n");
		return FALSE;
	}
	fprintf(stderr, "Number: %d\n", (*buckets).number);
	for(i=0; i<(*buckets).number; i++) {
		fprintf(stderr, "1\n");
		Bucket* bucket = (*buckets).internal_buckets[i];
		fprintf(stderr, "2\n");
		fprintf(stderr, "Margin: %f\n", (*bucket).margin);
	}
	return FALSE;
}

int print_metrics() {
	return TRUE;
}
