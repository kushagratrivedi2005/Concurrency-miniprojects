#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#define MAX_FILENAME 9 // 8 + NULL (ideally keep an odd number here)
#define NUM_THREADS 4
#define count_BASE (28 * 28)  // For processing 2 characters at once (26 letters + dot + space)
#define THRESHOLD 42

typedef struct {
    char name[MAX_FILENAME];
    int id;
    char timestamp[20];
    unsigned long long name_hash;  // Changed to unsigned for count sort
    long long timestamp_val;
} FileRecord;

typedef struct {
    FileRecord* records;
    FileRecord* output;
    int start;
    int end;
    int exp;
    char sort_by;
} ThreadArgs;

typedef struct {
    FileRecord* records;
    int start;
    int end;
    char sort_by;
} MergeSortArgs;

// Modified hash calculation to process 2 characters at once
unsigned long long calculate_name_hash(const char* name) {
    unsigned long long hash = 0;
    int len = strlen(name);
    
    // Process pairs of characters
    for (int i = 0; i < len; i += 2) {
        unsigned long long pair_value = 0;
        
        // First character of the pair
        char c1 = tolower(name[i]);
        if (c1 == ' ') {
            pair_value = 0;
        } else {
            int val1 = (c1 == '.') ? 1 : (c1 - 'a' + 2);
            pair_value = val1;
        }
        
        // Second character of the pair (if exists)
        if (i + 1 < len) {
            char c2 = tolower(name[i + 1]);
            if (c2 == ' ') {
                pair_value = pair_value * 27;
            } else {
                int val2 = (c2 == '.') ? 0 : (c2 - 'a' + 1);
                pair_value = pair_value * 27 + val2;
            }
        }
        
        hash = hash * count_BASE + pair_value;
    }
    
    return hash;
}

// Convert ISO timestamp to Unix timestamp (unchanged)
long long convert_timestamp(const char* timestamp) {
    struct tm tm = {0};
    sscanf(timestamp, "%4d-%2d-%2dT%2d:%2d:%2d",
           &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
           &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    return (long long)mktime(&tm);
}

// Get value for count sort based on sort column
unsigned long long get_sort_value(FileRecord* record, char sort_by) {
    switch (sort_by) {
        case 'I': return (unsigned long long)record->id;
        case 'T': return (unsigned long long)record->timestamp_val;
        case 'N': return record->name_hash;
        default: return 0;
    }
}

// Thread function for counting elements in count sort
void* count_count(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int count[count_BASE] = {0};
    
    // Count occurrences of each digit value
    for (int i = args->start; i < args->end; i++) {
        unsigned long long value = get_sort_value(&args->records[i], args->sort_by);
        unsigned long long digit = (value / args->exp) % count_BASE;
        count[digit]++;
    }
    
    // Calculate positions
    int pos[count_BASE];
    pos[0] = args->start;
    for (int i = 1; i < count_BASE; i++) {
        pos[i] = pos[i-1] + count[i-1];
    }
    
    // Copy records to output array
    for (int i = args->start; i < args->end; i++) {
        unsigned long long value = get_sort_value(&args->records[i], args->sort_by);
        unsigned long long digit = (value / args->exp) % count_BASE;
        args->output[pos[digit]++] = args->records[i];
    }
    
    return NULL;
}

// Single-threaded count sort implementation
void count_sort(FileRecord* records, int n, char sort_by) {
    FileRecord* output = (FileRecord*)malloc(n * sizeof(FileRecord));
    if (output == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    // Find maximum value to determine number of digits
    unsigned long long max_val = 0;
    for (int i = 0; i < n; i++) {
        unsigned long long value = get_sort_value(&records[i], sort_by);
        if (value > max_val) max_val = value;
    }
    
    // Do counting sort for every digit
    for (unsigned long long exp = 1; max_val/exp > 0; exp *= count_BASE) {
        int count[count_BASE] = {0};
        
        // Count occurrences of each digit value
        for (int i = 0; i < n; i++) {
            unsigned long long value = get_sort_value(&records[i], sort_by);
            unsigned long long digit = (value / exp) % count_BASE;
            count[digit]++;
        }
        
        // Calculate positions
        int pos[count_BASE];
        pos[0] = 0;
        for (int i = 1; i < count_BASE; i++) {
            pos[i] = pos[i-1] + count[i-1];
        }
        
        // Copy records to output array
        for (int i = 0; i < n; i++) {
            unsigned long long value = get_sort_value(&records[i], sort_by);
            unsigned long long digit = (value / exp) % count_BASE;
            output[pos[digit]++] = records[i];
        }
        
        // Copy output back to records
        memcpy(records, output, n * sizeof(FileRecord));
    }
    
    free(output);
}

// Parallel count sort implementation
void parallel_count_sort(FileRecord* records, int n, char sort_by) {
    FileRecord* output = (FileRecord*)malloc(n * sizeof(FileRecord));
    if (output == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    // Find maximum value to determine number of digits
    unsigned long long max_val = 0;
    for (int i = 0; i < n; i++) {
        unsigned long long value = get_sort_value(&records[i], sort_by);
        if (value > max_val) max_val = value;
    }
    
    // Do counting sort for every digit
    for (unsigned long long exp = 1; max_val/exp > 0; exp *= count_BASE) {
        pthread_t threads[NUM_THREADS];
        ThreadArgs thread_args[NUM_THREADS];
        int chunk_size = n / NUM_THREADS;
        
        // Create threads for parallel counting
        for (int i = 0; i < NUM_THREADS; i++) {
            thread_args[i].records = records;
            thread_args[i].output = output;
            thread_args[i].start = i * chunk_size;
            thread_args[i].end = (i == NUM_THREADS - 1) ? n : (i + 1) * chunk_size;
            thread_args[i].exp = exp;
            thread_args[i].sort_by = sort_by;
            
            if (pthread_create(&threads[i], NULL, count_count, &thread_args[i]) != 0) {
                fprintf(stderr, "Error creating thread\n");
                free(output);
                exit(1);
            }
        }
        
        // Wait for all threads to complete
        for (int i = 0; i < NUM_THREADS; i++) {
            if (pthread_join(threads[i], NULL) != 0) {
                fprintf(stderr, "Error joining thread\n");
                free(output);
                exit(1);
            }
        }
        
        // Copy output back to records
        memcpy(records, output, n * sizeof(FileRecord));
    }
    
    free(output);
}

// Compare function for merge sort
int compare_records(const FileRecord* a, const FileRecord* b, char sort_by) {
    long long val_a, val_b;
    
    switch (sort_by) {
        case 'I':
            val_a = a->id;
            val_b = b->id;
            break;
        case 'T':
            val_a = a->timestamp_val;
            val_b = b->timestamp_val;
            break;
        case 'N':
            val_a = a->name_hash;
            val_b = b->name_hash;
            break;
        default:
            return 0;
    }
    
    if (val_a < val_b) return -1;
    if (val_a > val_b) return 1;
    return 0;
}

// Merge function for merge sort
void merge(FileRecord* records, int left, int mid, int right, char sort_by) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;
    
    FileRecord* L = (FileRecord*)malloc(n1 * sizeof(FileRecord));
    FileRecord* R = (FileRecord*)malloc(n2 * sizeof(FileRecord));
    
    for (i = 0; i < n1; i++)
        L[i] = records[left + i];
    for (j = 0; j < n2; j++)
        R[j] = records[mid + 1 + j];
    
    i = 0;
    j = 0;
    k = left;
    
    while (i < n1 && j < n2) {
        if (compare_records(&L[i], &R[j], sort_by) <= 0)
            records[k++] = L[i++];
        else
            records[k++] = R[j++];
    }
    
    while (i < n1)
        records[k++] = L[i++];
    while (j < n2)
        records[k++] = R[j++];
    
    free(L);
    free(R);
}

// Sequential merge sort
void merge_sort_sequential(FileRecord* records, int left, int right, char sort_by) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        merge_sort_sequential(records, left, mid, sort_by);
        merge_sort_sequential(records, mid + 1, right, sort_by);
        merge(records, left, mid, right, sort_by);
    }
}

// Thread function for merge sort
void* merge_sort_thread(void* arg) {
    MergeSortArgs* args = (MergeSortArgs*)arg;
    merge_sort_sequential(args->records, args->start, args->end, args->sort_by);
    return NULL;
}

// Parallel merge sort implementation
void parallel_merge_sort(FileRecord* records, int n, char sort_by) {
    pthread_t threads[NUM_THREADS];
    MergeSortArgs thread_args[NUM_THREADS];
    int chunk_size = n / NUM_THREADS;
    
    // Create threads for initial parallel sorting
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].records = records;
        thread_args[i].start = i * chunk_size;
        thread_args[i].end = (i == NUM_THREADS - 1) ? n - 1 : (i + 1) * chunk_size - 1;
        thread_args[i].sort_by = sort_by;
        
        pthread_create(&threads[i], NULL, merge_sort_thread, &thread_args[i]);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Merge the sorted chunks
    for (int size = chunk_size; size < n; size = size * 2) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = (left + 2 * size - 1 < n - 1) ? left + 2 * size - 1 : n - 1;
            if (mid < right) {
                merge(records, left, mid, right, sort_by);
            }
        }
    }
}

int main() {
    int n;
    if (scanf("%d", &n) != 1) {
        fprintf(stderr, "Invalid input for number of records\n");
        return 1;
    }
    
    FileRecord* records = (FileRecord*)malloc(n * sizeof(FileRecord));
    if (records == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    // Read input
    for (int i = 0; i < n; i++) {
        if (scanf("%s %d %s", records[i].name, &records[i].id, records[i].timestamp) != 3) {
            fprintf(stderr, "Invalid input for record %d\n", i);
            free(records);
            return 1;
        }
        if (strlen(records[i].name)!=MAX_FILENAME-1){
            int len= strlen(records[i].name);
            for (int j=len;j<MAX_FILENAME;j++){
                records[i].name[j]=' ';
            }
            records[i].name[MAX_FILENAME]='\0';
        }
        records[i].name_hash = calculate_name_hash(records[i].name);
        records[i].timestamp_val = convert_timestamp(records[i].timestamp);
    }
    
    char sort_column[10];
    if (scanf("%s", sort_column) != 1) {
        fprintf(stderr, "Invalid input for sort column\n");
        free(records);
        return 1;
    }
    
    // Determine sort type
    char sort_by;
    if (strcmp(sort_column, "ID") == 0) sort_by = 'I';
    else if (strcmp(sort_column, "Timestamp") == 0) sort_by = 'T';
    else sort_by = 'N';
    
    // Choose sorting algorithm based on number of records
    if (n < THRESHOLD) {
        parallel_count_sort(records, n, sort_by);
    } else {
        parallel_merge_sort(records, n, sort_by);
    }
    
    // Print sorted records
    printf("%s\n", sort_column);
    for (int i = 0; i < n; i++) {
        printf("%s \t%d\t\t%s\n", records[i].name, records[i].id, records[i].timestamp);
    }
    
    free(records);
    return 0;
}