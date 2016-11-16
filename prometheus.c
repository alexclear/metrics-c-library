#include "prometheus.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#define METRIC_TYPE_COUNTER "counter"
#define METRIC_TYPE_HISTOGRAM "histogram"

GHashTable *metrics_storage = NULL;

typedef struct {
	char **labels;
	void *value;
} ConcreteValue;

typedef struct {
	unsigned int number_labels;
	char *name;
	char **label_names;
	char *help;
	char *type;
	GHashTable *labeled_metric;
} Metric;

typedef struct {
	Metric metric;
	unsigned int number_buckets;
	double *bucket_margins;
} Histogram;

typedef struct {
	double margin;
	unsigned int count;
} Bucket;

typedef struct {
	unsigned int number_buckets;
	unsigned int total_count;
	double total_sum;
	Bucket** internal_buckets;
} Buckets;

int new_counter_vec(char* name, char* help, char** label_names, int nlabels) {
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
	(*counter).name = name;
	(*counter).type = METRIC_TYPE_COUNTER;
	(*counter).number_labels = nlabels;
	(*counter).labeled_metric = g_hash_table_new(&g_string_hash, &g_string_equal);
	(*counter).label_names = malloc(sizeof(char*) * nlabels);
	for(i=0; i<nlabels; i++) {
		(*counter).label_names[i] = malloc(strlen(label_names[i]) + 1);
		strncpy((*counter).label_names[i], label_names[i], strlen(label_names[i]) + 1);
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
		ConcreteValue* concrete_val = g_hash_table_lookup((*counter).labeled_metric, key_string);
		if(concrete_val != NULL) {
			fprintf(stderr, "Old counter value: %i\n", *((int*) concrete_val->value));
		} else {
			int* val = (int *) malloc(sizeof(int));
			(*val) = 0;
			concrete_val = (ConcreteValue*) malloc(sizeof(ConcreteValue));
			(*concrete_val).labels = malloc(sizeof(char*) * nlabels);
			concrete_val->value = val;
			GString *key_string = g_string_new("");
			for(i=0; i<nlabels; i++) {
				key_string = g_string_append(key_string, labels[i]);
				(*concrete_val).labels[i] = malloc(strlen(labels[i]) + 1);
				strncpy((*concrete_val).labels[i], labels[i], strlen(labels[i]) + 1);
			}
			if(g_hash_table_insert((*counter).labeled_metric, key_string, concrete_val) == FALSE) {
				return FALSE;
			}
		}
		(*((int*) concrete_val->value))++;
	} else {
		fprintf(stderr, "Failed to lookup a counter, it is NULL\n");
		return FALSE;
	}
	return TRUE;
}

ConcreteValue* initialize_buckets(Histogram *histogram, char** labels) {
	int i;
	double previous_margin = 0;
	fprintf(stderr, "Number of buckets: %d\n", histogram->number_buckets);
	Buckets* buckets = malloc(sizeof(Buckets));
	(*buckets).number_buckets = histogram->number_buckets;
	(*buckets).total_count = 0;
	(*buckets).internal_buckets = malloc(sizeof(Bucket*) * histogram->number_buckets);
	for(i=0; i < histogram->number_buckets; i++) {
		int j;
		//fprintf(stderr, "Pointer: %d, margin: %f\n", &((*buckets).internal_buckets[i]), bucket_margins[i]);
		//fprintf(stderr, "1==========\n");
		//for(j=0; j<i; j++) {
		//	Bucket* bucket1 = (*buckets).internal_buckets[j];
		//	fprintf(stderr, "Pointer: %d, Margin: %f\n", &((*bucket1).margin), (*bucket1).margin);
		//}
		//fprintf(stderr, "!=========!\n");
		(*buckets).internal_buckets[i] = malloc(sizeof(Bucket));
		//fprintf(stderr, "2==========\n");
		//for(j=0; j<i; j++) {
		//	Bucket* bucket1 = (*buckets).internal_buckets[j];
		//	fprintf(stderr, "Pointer: %d, Margin: %f\n", &((*bucket1).margin), (*bucket1).margin);
		//}
		//fprintf(stderr, "!=========!\n");
		(*((*buckets).internal_buckets[i])).margin = histogram->bucket_margins[i];
		(*((*buckets).internal_buckets[i])).count = 0;
		//fprintf(stderr, "3==========\n");
		//for(j=0; j<i; j++) {
		//	Bucket* bucket1 = (*buckets).internal_buckets[j];
		//	fprintf(stderr, "Pointer: %d, Margin: %f\n", &((*bucket1).margin), (*bucket1).margin);
		//}
		//fprintf(stderr, "!=========!\n");
		fprintf(stderr, "Pointer: %d, margin: %f, margin': %f\n", &((*((*buckets).internal_buckets[i])).margin), histogram->bucket_margins[i], (*((*buckets).internal_buckets[i])).margin);
		if(histogram->bucket_margins[i] < previous_margin) {
			fprintf(stderr, "Margins are not properly sorted!\n");
			exit(-1);
		}
		previous_margin = histogram->bucket_margins[i];
		//fprintf(stderr, "4==========\n");
		//for(j=0; j<i; j++) {
		//	Bucket* bucket1 = (*buckets).internal_buckets[j];
		//	fprintf(stderr, "Pointer: %d, Margin: %f\n", &((*bucket1).margin), (*bucket1).margin);
		//}
		//fprintf(stderr, "!=========!\n");
	}
	ConcreteValue* concrete_val = (ConcreteValue*) malloc(sizeof(ConcreteValue));
	(*concrete_val).labels = malloc(sizeof(char*) * histogram->metric.number_labels);

	concrete_val->value = buckets;
	GString *key_string = g_string_new("");
	for(i=0; i < histogram->metric.number_labels; i++) {
		key_string = g_string_append(key_string, labels[i]);
		(*concrete_val).labels[i] = malloc(strlen(labels[i]) + 1);
		strncpy((*concrete_val).labels[i], labels[i], strlen(labels[i]) + 1);
	}
	if(g_hash_table_insert((*histogram).metric.labeled_metric, key_string, concrete_val) == FALSE) {
		return NULL;
	}
	return concrete_val;
}

int new_histogram_vec(char* name, char* help, char** label_names, int nlabels, double* bucket_margins, int nbuckets) {
	double previous_margin = 0;
	int i=0;
	// Initialize a hash table
	// TODO: do it only once (set a lock)
        //     - not applicable in nginx environment
	if(metrics_storage == NULL) {
		metrics_storage = g_hash_table_new(g_str_hash, g_str_equal);
	}

	// TODO: make this thread-safe
	Histogram *histogram = g_hash_table_lookup(metrics_storage, name);
	if(histogram != NULL) {
		fprintf(stderr, "Can't register a histogram named %s because there is a metric with that name already!\n", name);
		return FALSE;
	}

	histogram = malloc(sizeof(Histogram));
	(*histogram).metric.help = help;
	(*histogram).metric.name = name;
	(*histogram).metric.type = METRIC_TYPE_HISTOGRAM;
	(*histogram).metric.number_labels = nlabels;
	(*histogram).number_buckets = nbuckets;
	(*histogram).bucket_margins = bucket_margins;
	(*histogram).metric.label_names = malloc(sizeof(char*) * nlabels);
	for(i=0; i<nlabels; i++) {
		(*histogram).metric.label_names[i] = malloc(strlen(label_names[i]) + 1);
		strncpy((*histogram).metric.label_names[i], label_names[i], strlen(label_names[i]) + 1);
	}
	(*histogram).metric.labeled_metric = g_hash_table_new(&g_string_hash, &g_string_equal);
	if(g_hash_table_insert(metrics_storage, name, histogram) == FALSE) {
		return FALSE;
	}
	//for(i=0; i<nbuckets; i++) {
	//	Bucket* bucket = (*buckets).internal_buckets[i];
	//	fprintf(stderr, "Pointer: %d, Margin: %f\n", &((*bucket).margin), (*bucket).margin);
	//}
	fprintf(stderr, "new_histogram_vec succeeded\n");
}

int observe_histogram(char* name, char** labels, int nlabels, double value) {
	int i, val_index;
	fprintf(stderr, "Observe_histogram called\n");
	// TODO: check if number of labels is valid
	Histogram *histogram = g_hash_table_lookup(metrics_storage, name);
	if(histogram == NULL) {
		fprintf(stderr, "Can't find a histogram named %s\n", name);
		return FALSE;
	}
	GString *key_string = g_string_new("");
	for(i=0; i<nlabels; i++) {
		key_string = g_string_append(key_string, labels[i]);
	}
	ConcreteValue* concrete_val = g_hash_table_lookup((*histogram).metric.labeled_metric, key_string);
	if(concrete_val == NULL) {
		fprintf(stderr, "Calling initialize_buckets\n");
		concrete_val = initialize_buckets(histogram, labels);
		fprintf(stderr, "initialize_buckets finished\n");
	}
	fprintf(stderr, "Number: %d\n", (*((Buckets*) concrete_val->value)).number_buckets);
	for(i=0; i<(*((Buckets*) concrete_val->value)).number_buckets; i++) {
		//fprintf(stderr, "1\n");
		Bucket* bucket = (*((Buckets*) concrete_val->value)).internal_buckets[i];
		//fprintf(stderr, "2\n");
		fprintf(stderr, "Pointer: %d, Margin: %f\n", &((*bucket).margin), (*bucket).margin);
		if((*bucket).margin > value) {
			break;
		}
	}
	val_index = i;
	(*((*((Buckets*) concrete_val->value)).internal_buckets[val_index])).count++;
	(*((Buckets*) concrete_val->value)).total_count++;
	(*((Buckets*) concrete_val->value)).total_sum += value;
	fprintf(stderr, "Observe_histogram finished\n");
	return TRUE;
}

void print_labeled_metric(gpointer label_name, gpointer gpmetric) {
	int i, j;
	Metric *pmetric = (Metric*) gpmetric;
	//fprintf(stderr, "Key: %s\n", ((GString*) label_name)->str);
	ConcreteValue *val = g_hash_table_lookup((*pmetric).labeled_metric, label_name);
	if(val != NULL) {
//		fprintf(stderr, "Label name: %s\n", val->labels[i]);
		if(strcmp((*pmetric).type, METRIC_TYPE_COUNTER) == 0) {
			fprintf(stderr, "%s", (*pmetric).name);
			if(pmetric->number_labels > 0) {
				fprintf(stderr, "{");
				for(i=0; i < (*pmetric).number_labels; i++) {
					fprintf(stderr, "%s=\"%s\",", pmetric->label_names[i], val->labels[i]);
				}
				fprintf(stderr, "} ");
			}
			fprintf(stderr, "%f\n", (float) *((int*) val->value));
		}
		if(strcmp((*pmetric).type, METRIC_TYPE_HISTOGRAM) == 0) {
			int cumulative_count = 0;
			Buckets* buckets = (Buckets*) val->value;
			for(j=0; j < (buckets->number_buckets); j++) {
				fprintf(stderr, "%s_bucket", (*pmetric).name);
				if(pmetric->number_labels > 0) {
					fprintf(stderr, "{");
					for(i=0; i < (*pmetric).number_labels; i++) {
						fprintf(stderr, "%s=\"%s\",", pmetric->label_names[i], val->labels[i]);
					}
					fprintf(stderr, "le=\"%f\",", buckets->internal_buckets[j]->margin);
					fprintf(stderr, "}");
				}
				cumulative_count += buckets->internal_buckets[j]->count;
				fprintf(stderr, " %f\n", (float) cumulative_count);
			}


			fprintf(stderr, "%s_bucket", (*pmetric).name);
			if(pmetric->number_labels > 0) {
				fprintf(stderr, "{");
				for(i=0; i < (*pmetric).number_labels; i++) {
					fprintf(stderr, "%s=\"%s\",", pmetric->label_names[i], val->labels[i]);
				}
				fprintf(stderr, "le=\"+Inf\",");
				fprintf(stderr, "}");
			}
			fprintf(stderr, " %f\n", (float) buckets->total_count);
			fprintf(stderr, "%s_count", (*pmetric).name);
			if(pmetric->number_labels > 0) {
				fprintf(stderr, "{");
				for(i=0; i < (*pmetric).number_labels; i++) {
					fprintf(stderr, "%s=\"%s\",", pmetric->label_names[i], val->labels[i]);
				}
				fprintf(stderr, "}");
			}
			fprintf(stderr, " %f\n", (float) buckets->total_count);
			fprintf(stderr, "%s_sum", (*pmetric).name);
			if(pmetric->number_labels > 0) {
				fprintf(stderr, "{");
				for(i=0; i < (*pmetric).number_labels; i++) {
					fprintf(stderr, "%s=\"%s\",", pmetric->label_names[i], val->labels[i]);
				}
				fprintf(stderr, "}");
			}
			fprintf(stderr, " %f\n", buckets->total_sum);
		}
	} else {
		fprintf(stderr, "Failed to lookup a labeled metric value, it is NULL\n");
		exit(-1);
	}
}

void print_metric(gpointer key, gpointer user_data) {
	Metric *metric = g_hash_table_lookup(metrics_storage, key);
	if(metric == NULL) {
		fprintf(stderr, "Can't find a metric named %s\n", (char*) key);
		exit(-1);
	}
	fprintf(stderr, "# HELP %s %s\n", (char*) key, metric->help);
	fprintf(stderr, "# TYPE %s %s\n", (char*) key, metric->type);
	g_list_foreach(g_hash_table_get_keys(metric->labeled_metric), print_labeled_metric, metric);
}

int print_metrics() {
	g_list_foreach(g_hash_table_get_keys(metrics_storage), print_metric, NULL);
	return TRUE;
}
