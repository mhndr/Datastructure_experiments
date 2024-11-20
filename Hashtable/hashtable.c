#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include <string.h> 


#define HASHTABLE_MIN_SIZE 16
#define HASHTABLE_HIGH 0.50
#define HASHTABLE_LOW 0.10
#define HASHTABLE_REHASH_FACTOR 2.0 / (HASHTABLE_LOW + HASHTABLE_HIGH)

#define THREAD_COUNT 5
#define SIZE 100
#define hash2(i) i*2654435761 % (2^32)
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))

unsigned long hash(unsigned long x) {
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

pthread_t tid[5]; 

//TODO: Implement locks to control concurrent access

/*typedef struct {
	int count;
	int sum;
	float min;
	float max;
	float mean;
}Record;*/

typedef struct {
	size_t size;
	int *array;
	//Record *array; 
	pthread_mutex_t lock; 
}Entry;

typedef struct {
	size_t nentries;
	Entry* entries;
}Hashtable;


int insert(char* caller,Hashtable *ht, size_t key, int value) {
	size_t key_hash = hash(key);
	size_t idx = key_hash % SIZE;
//	printf("\n%s inside insert",caller);
	Entry* entry = &ht->entries[idx];
	pthread_mutex_lock(&entry->lock); 
	printf("\nlock acquired on entry %zu by %s",idx,caller);
	if(entry->size == 0) {
		entry->size = 1;
		entry->array = (int*)malloc(sizeof(int));
		entry->array[0] = value;
		ht->nentries++;
	}else {
		entry->size++;
		entry->array = (int*) realloc(entry->array,sizeof(int)*(entry->size));
		entry->array[entry->size-1] = value;
	}	
	pthread_mutex_unlock(&entry->lock); 
	printf("\nlock released on entry %zu by %s",idx,caller);
	return 0;
}

int get(Hashtable *ht, size_t key) {
	size_t key_hash = hash(key);
	size_t idx = key_hash % SIZE;
	return 0;	
}

Hashtable* create_hashtable() {
	Hashtable *ht = (Hashtable*)malloc(sizeof(Hashtable));
	ht->nentries = 0;
	ht->entries = (Entry*) malloc(sizeof(Entry)*SIZE);
	
	for(int i=0;i<SIZE;i++) {
		ht->entries[i].size = 0;
		ht->entries[i].array = NULL;
		if (pthread_mutex_init(&ht->entries[i].lock, NULL) != 0) { 
        	printf("\n mutex init has failed for entry no: %d",i); 
        	return NULL; 
    	} 
	}
	return ht;
}

int print_ht(Hashtable* ht) {
	if(ht == NULL)
		return -1;
	
	int size;
	for(int i=0;i<SIZE;i++) {
		if(ht->entries[i].size == 0) {
			printf("\nNULL");
		}
		else{
			size = ht->entries[i].size;
			printf("\n");
			for(int j=0;j<size;j++) {
				printf("%d ",ht->entries[i].array[j]);
			}
		}
	}
	return 0;	
}

typedef struct {
	char caller[25];
	Hashtable *ht;
}Thread_arg;

void* trythis(void *arg) 
{ 
	printf("\nInside thread");
	Hashtable *ht = ((Thread_arg*)arg)->ht;
	char *caller =  ((Thread_arg*)arg)->caller;
	int random_number;
	for(int i=0;i<SIZE;i++) {
		random_number = rand();
		insert(caller,ht,random_number,i);
	}
	return NULL;
}

int main() {
	Hashtable *ht = create_hashtable();
	int i=0;
	int error; 
	Thread_arg *arg_main,arg_thr[THREAD_COUNT];

	arg_main = malloc(sizeof(Thread_arg));
	arg_main->ht = ht;
	strcpy(arg_main->caller,"main");

	srand(time(NULL));

	while (i < THREAD_COUNT) { 
		arg_thr[i].ht = ht;
		sprintf(arg_thr[i].caller,"Thread %d",i);

		error = pthread_create(&(tid[i]), NULL, &trythis, &arg_thr[i]); 
        if (error != 0) 
            printf("\nThread can't be created : [%s]", strerror(error)); 
		printf("\nThread %d created",i);
        i++; 
    } 
  	trythis(arg_main);
	
	for (int i=0;i<THREAD_COUNT;i++) {
    	pthread_join(tid[i], NULL); 
    }


	print_ht(ht);
	return 0;
}

