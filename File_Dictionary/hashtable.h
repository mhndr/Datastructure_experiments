#include <stdbool.h>


typedef struct {
	void *key;
	void *value;
}Item;

typedef struct {
	size_t nItems;
	Item **items;
	//pthread_mutex_t lock; 
}Entry;

typedef struct {
	size_t nentries;
	Entry* entries;
	size_t size;
}Hashtable;

int ht_put(Hashtable *ht, char *key, void *value);
void* ht_get(Hashtable *ht, char *key);
Hashtable* create_hashtable(size_t size);
bool ht_find_key(Hashtable* ht,char *key);
char** ht_get_keys(Hashtable* ht,int* nKeys);
