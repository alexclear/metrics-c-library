#include "prometheus.h"
#include <glib.h>
#include <stdio.h>

GHashTable *metrics_storage = NULL;

typedef struct {
	char *help;
	GHashTable *labeled_metric;
} Counter;

int new_counter_vec(char* name, char* help, char** labels, int nlabels) {
	int i=0;
	// Initialize a hash table
	// TODO: do it only once (set a lock)
	if(metrics_storage == NULL) {
		metrics_storage = g_hash_table_new(g_str_hash, g_str_equal);
	}

	// TODO: check if we already have a storage with this name

	Counter *counter = malloc(sizeof(Counter));
	(*counter).help = help;
	(*counter).labeled_metric = g_hash_table_new(&g_string_hash, &g_string_equal);
	int* val = malloc(sizeof(int));
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
	Counter *counter = g_hash_table_lookup(metrics_storage, name);
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
