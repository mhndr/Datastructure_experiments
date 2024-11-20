#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h> 
#include"hashtable.h"
#include<stdbool.h>

#define HASHTABLE_MIN_SIZE 16
#define HASHTABLE_HIGH 0.50
#define HASHTABLE_LOW 0.10
#define HASHTABLE_REHASH_FACTOR 2.0 / (HASHTABLE_LOW + HASHTABLE_HIGH)

#define THREAD_COUNT 5
#define SIZE 10
#define hash2(i) i*2654435761 % (2^32)
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))

static unsigned long hash(unsigned long x) {
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

static unsigned long  DJB2_hash(char *str)
{
    uint32_t hash = 5381;
    uint8_t c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

char** ht_get_keys(Hashtable *ht , int *nKeys) {
	if(!ht || !nKeys)
		return NULL;

	char **keys = NULL;
	*nKeys=0; 	
	printf("\nGET KEYS");
	for(int i=0;i<ht->size;i++) {
		for(int j=0;j<ht->entries[i].nItems;j++) {
			keys = (char**)realloc(keys,(j+1)*sizeof(char*));
			keys[*nKeys] = ((Item*)ht->entries[i].items[j])->key;
			printf(" %s ",keys[*nKeys]);
			(*nKeys)++;
		}
	}
	return keys;
}

int ht_put(Hashtable *ht, char* key, void* value) {
	size_t key_hash = DJB2_hash(key);
	size_t idx = key_hash % ht->size;
	Entry* entry = &ht->entries[idx];
	//pthread_mutex_lock(&entry->lock); 
	if(entry->nItems != 0) {
		//key exists, modify the value
		for(int i=0;i<entry->nItems;i++) {
			if(strcmp(key,entry->items[i]->key) == 0){
				//printf("\nkey  %s exists,updating",key);
				free(entry->items[i]->value); //free old value
				entry->items[i]->value = value; //update
				goto exit;
			}
		}
	}	
	Item *item = malloc(sizeof(Item));
	item->key = key;
	item->value = value;
	entry->nItems++;
	entry->items = (Item**) realloc(entry->items,(sizeof(void*))*(entry->nItems));
	entry->items[entry->nItems-1] = item;
	ht->nentries++;

exit:
	//pthread_mutex_unlock(&entry->lock); 
	return 0;
}


//TODO
//ht_delete()
//ht_resize()
bool ht_find_key(Hashtable *ht, char *key){
	if(!ht || !key)
		return false;
			
	for(int i=0;i<ht->size;i++) {
		for(int j=0;j<ht->entries[i].nItems;j++) {
			if(0 == strcmp(key, ((Item*)ht->entries[i].items[j])->key))
				return true;
		}	
	}
	return false;
}

void*  ht_get(Hashtable *ht, char * key) {
	size_t key_hash = DJB2_hash(key);
	size_t idx = key_hash % ht->size;
	Entry* entry = &ht->entries[idx];

	if(entry->nItems == 0) {
		return NULL;
	}
	//pthread_mutex_lock(&entry->lock); 
	for(int i=0;i<entry->nItems;i++) {
		if(strcmp(key,entry->items[i]->key) == 0){
			//pthread_mutex_unlock(&entry->lock);
			return  entry->items[i]->value;
		}
	}
	return NULL;	
}

Hashtable* create_hashtable(size_t size) {
	Hashtable *ht = (Hashtable*)malloc(sizeof(Hashtable));
	ht->nentries = 0;
	ht->entries = (Entry*) malloc(sizeof(Entry)*SIZE);
	ht->size = size;
	for(int i=0;i<size;i++) {
		ht->entries[i].nItems = 0;
		ht->entries[i].items= NULL;
		/*if (pthread_mutex_init(&ht->entries[i].lock, NULL) != 0) { 
        	printf("\n mutex init has failed for entry no: %d",i); 
        	return NULL; 
    	} */
	}
	return ht;
}

