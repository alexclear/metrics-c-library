#include "prometheus.h"
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	return TRUE;
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
	unsigned int i;
	double previous_margin = 0;
	fprintf(stderr, "Number of buckets: %d\n", histogram->number_buckets);
	Buckets* buckets = malloc(sizeof(Buckets));
	(*buckets).number_buckets = histogram->number_buckets;
	(*buckets).total_count = 0;
	(*buckets).internal_buckets = malloc(sizeof(Bucket*) * histogram->number_buckets);
	for(i=0; i < histogram->number_buckets; i++) {
		(*buckets).internal_buckets[i] = malloc(sizeof(Bucket));
		(*((*buckets).internal_buckets[i])).margin = histogram->bucket_margins[i];
		(*((*buckets).internal_buckets[i])).count = 0;
		if(histogram->bucket_margins[i] < previous_margin) {
			fprintf(stderr, "Margins are not properly sorted!\n");
			exit(-1);
		}
		previous_margin = histogram->bucket_margins[i];
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
	fprintf(stderr, "new_histogram_vec succeeded\n");
	return TRUE;
}

int observe_histogram(char* name, char** labels, unsigned int nlabels, double value) {
	unsigned int i, val_index;
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
	for(i=0; i<(*((Buckets*) concrete_val->value)).number_buckets; i++) {
		Bucket* bucket = (*((Buckets*) concrete_val->value)).internal_buckets[i];
		if((*bucket).margin > value) {
			break;
		}
	}
	val_index = i;
	(*((*((Buckets*) concrete_val->value)).internal_buckets[val_index])).count++;
	(*((Buckets*) concrete_val->value)).total_count++;
	(*((Buckets*) concrete_val->value)).total_sum += value;
	return TRUE;
}

typedef void (* printf_callback) (void* context, const char *format, ...);

typedef struct {
	int max_size;
	char *buffer;
	int result;
	Metric *current_metric;
	printf_callback printf_callback;
} ExportContext;

void do_fprintf(ExportContext* context, const char *format, ...) {
	va_list arglist;
	va_start(arglist,format);
	vfprintf(stderr, format, arglist);
	va_end(arglist);
}

void shift(ExportContext* context, int result) {
	context->result += result;
	context->max_size -= result;
	if(context->max_size <= 0) {
		fprintf(stderr, "Buffer exhausted\n");
		exit(-1);
	}
}

void msnprintf(ExportContext* context, const char *format, ...) {
	va_list arglist;
	va_start(arglist,format);
	int result = vsnprintf(context->buffer + context->result, context->max_size, format, arglist);
	va_end(arglist);
	shift(context, result);
}

typedef void (* labels_callback) ();

void print_labels(Metric *pmetric, ExportContext* context, ConcreteValue *val, labels_callback callback) {
	unsigned int i;
	if(pmetric->number_labels > 0) {
		(*context->printf_callback)(context, "{");
		for(i=0; i < (*pmetric).number_labels; i++) {
			(*context->printf_callback)(context, "%s=\"%s\",", pmetric->label_names[i], val->labels[i]);
		}
		if(callback != NULL) {
			(*callback)();
		}
		(*context->printf_callback)(context, "}");
	}
}

void export_labeled_metric(gpointer label_name, gpointer gcontext) {
	unsigned int i, j;
	ExportContext* context = (ExportContext*) gcontext;
	Metric *pmetric = context->current_metric;
	ConcreteValue *val = g_hash_table_lookup((*pmetric).labeled_metric, label_name);
	if(val != NULL) {
		if(strcmp((*pmetric).type, METRIC_TYPE_COUNTER) == 0) {
			(*context->printf_callback)(context, "%s", (*pmetric).name);
			if(pmetric->number_labels > 0) {
				(*context->printf_callback)(context, "{");
				for(i=0; i < (*pmetric).number_labels; i++) {
					(*context->printf_callback)(context, "%s=\"%s\",", pmetric->label_names[i], val->labels[i]);
				}
				(*context->printf_callback)(context, "} ");
			}
			(*context->printf_callback)(context, "%f\n", (float) *((int*) val->value));
		}
		if(strcmp((*pmetric).type, METRIC_TYPE_HISTOGRAM) == 0) {
			int cumulative_count = 0;
			Buckets* buckets = (Buckets*) val->value;
			for(j=0; j < (buckets->number_buckets); j++) {
				(*context->printf_callback)(context, "%s_bucket", (*pmetric).name);
				print_labels(pmetric, context, val, ( { void label_callback() {
						(*context->printf_callback)(context, "le=\"%f\",", buckets->internal_buckets[j]->margin);
					} label_callback; } ));
				cumulative_count += buckets->internal_buckets[j]->count;
				(*context->printf_callback)(context, " %f\n", (float) cumulative_count);
			}


			(*context->printf_callback)(context, "%s_bucket", (*pmetric).name);
			print_labels(pmetric, context, val, ( { void label_callback() {
				(*context->printf_callback)(context, "le=\"+Inf\",");
			} label_callback; } ));
			(*context->printf_callback)(context, " %f\n", (float) buckets->total_count);
			(*context->printf_callback)(context, "%s_count", (*pmetric).name);
			print_labels(pmetric, context, val, NULL);
			(*context->printf_callback)(context, " %f\n", (float) buckets->total_count);
			(*context->printf_callback)(context, "%s_sum", (*pmetric).name);
			print_labels(pmetric, context, val, NULL);
			(*context->printf_callback)(context, " %f\n", buckets->total_sum);
		}
	} else {
		fprintf(stderr, "Failed to lookup a labeled metric value, it is NULL\n");
		exit(-1);
	}
}

void export_metric(gpointer key, gpointer user_data) {
        ExportContext* context = (ExportContext*) user_data;
	Metric *metric = g_hash_table_lookup(metrics_storage, key);
	if(metric == NULL) {
		fprintf(stderr, "Can't find a metric named %s\n", (char*) key);
		exit(-1);
	}
	(*context->printf_callback)(context, "# HELP %s %s\n", (char*) key, metric->help);
	(*context->printf_callback)(context, "# TYPE %s %s\n", (char*) key, metric->type);
	context->current_metric = metric;
	g_list_foreach(g_hash_table_get_keys(metric->labeled_metric), export_labeled_metric, context);
}

int internal_export_metrics(char* buffer, int max_size, printf_callback callback) {
	ExportContext context;
	context.max_size = max_size;
	context.result = 0;
	context.buffer = buffer;
	context.printf_callback = callback;
	g_list_foreach(g_hash_table_get_keys(metrics_storage), export_metric, &context);
	return context.result;
}

int export_metrics(char* buffer, int max_size) {
	return internal_export_metrics(buffer, max_size, (printf_callback) msnprintf);
}

int print_metrics() {
	return internal_export_metrics(NULL, 0, (printf_callback) do_fprintf);
}
