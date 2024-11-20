#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "hashtable.h"

#define SIZE 20
#define ALIVE 0xfedbeef
#define DEAD 0xdeadbeef
#define MAX_INPUT_SIZE 1024

const char* db_name = "./test_c.db";
Hashtable *ht;
const char* commands[] = { "GET", "PUT", "DEL"};

typedef struct {
	size_t total_len;
	uint64_t CRC;
	uint64_t DoA;
	uint64_t timestamp;
	size_t key_len;
	size_t val_len;
} Header;

typedef struct{
	Header header;
	char *data[];
} Record;

typedef struct{
	size_t offset;
	size_t size;
} Value;

//static void clean_file();
//void patch(char* patch_file_name);


void* get(char* key) {
	FILE *db_fd = fopen(db_name,"r");
	Value *value = (Value*) ht_get(ht,key);
	if(!value) {
		fclose(db_fd);
		return NULL;
	}
	//printf("\noffset = %zu,size = %zu",value->offset,value->size);
	fseek(db_fd,value->offset,0);
	char *data = malloc(sizeof(value->size));
	int ret = fread(data,value->size,1,db_fd);
	if(ret == value->size) {
		fclose(db_fd);
		return data;
	}
	
	fclose(db_fd);
	return data;
}


void print_index() {
	if(ht == NULL)
		return;
	
	int nKeys;
	char **keys = ht_get_keys(ht,&nKeys);
	printf("\nPRINT INDEX\n");
	printf("no of keys = %d\n",nKeys); 
	for(int i=0;i<nKeys;i++) {
		printf(" %s ",keys[i]);
		//FIXME
		//char *val = get(keys[i]);
		//printf("{%s,%s}\n",keys[i],val);
	}
	return;
}



static void load_index() {
	FILE *db_fd = fopen(db_name,"r");
	Header *header;
	char *data;
	int ret;
	if(!db_fd)
		return;
	while(1) {
		header =  malloc(sizeof(Header));
		ret = fread((char*)header,sizeof(Header),1,db_fd);
		if(ret != 1){
			free(header);
			goto exit; 
		}
		/*
		printf("\nDoA = %04llx\n",header->DoA);
		printf("timestamp  = %04llx\n",header->timestamp);
		printf("key length = %zu\n",header->key_len);
		printf("val length = %zu\n",header->val_len);
		printf("total length  = %zu\n",header->total_len);
		*/
		if(header->DoA != ALIVE) {
			fseek(db_fd,header->key_len+header->val_len,ftell(db_fd));
			free(header);
			continue; //don't load deleted records
		}
		char *word = malloc(sizeof(header->key_len));
		fread(word,header->key_len,1,db_fd);
		printf("word = %s , ",word);

		Value *value  = malloc(sizeof(Value));
		value->offset = ftell(db_fd);
		value->size   = header->val_len;
    	ht_put(ht,word,value);

		char *meaning = malloc(sizeof(header->val_len)); 
		fread(meaning,header->val_len,1,db_fd);
		printf("meaning = %s , ",meaning);
		header->DoA==ALIVE ? printf("ALIVE\n"):printf("DEAD\n");
		//printf("offset = %zu,size = %zu\n",value->offset,value->size);

		free(header);
	}
exit:
	fclose(db_fd);
} 

void del(char* key) {
	FILE *db_fd = fopen(db_name,"r+");	
	size_t key_len = strlen(key);
	Value *value = ht_get(ht,key);
	
	Header *header = malloc(sizeof(Header));
	//seek to the start of the record for given key
	rewind(db_fd);
	fseek(db_fd,value->offset-(key_len+sizeof(Header)),0);
	int ret = fread((char*)header,sizeof(Header),1,db_fd);
	if(ret != 1){
		free(header);
		return; 
	}
	
	printf("\nDoA = %04llx\n",header->DoA);
	printf("val length = %zu\n",header->val_len);
	printf("val offset = %zu\n",value->offset);
	
	header->DoA = DEAD;
	//FIXME
	//rewind(db_fd);
	//fseek(db_fd,value->offset-(key_len+sizeof(Header)),0);
	fwrite((char*)header,sizeof(Header),1,db_fd);
	fclose(db_fd);

}

void put(char* key,void* value,uint64_t val_size) {
	if(ht_find_key(ht,key)) {
		printf("Found key %s in index, updating file\n",key);
		del(key);
	}
	FILE *db_fd = fopen(db_name,"a");	
	size_t key_len = strlen(key);
	size_t record_size = sizeof(Header)+key_len+val_size;
	Record *record = malloc(record_size);
	
	record->header.DoA       = ALIVE;
	record->header.timestamp = 0xcafe007;
	record->header.key_len   = key_len;
	record->header.val_len   = val_size;
	record->header.total_len = sizeof(Header)+key_len+val_size;

	void *data = (void*)(record->data);

	memcpy(data,key,key_len);
	data =  (char*)data + key_len;
	memcpy(data,value,val_size);
	
	fwrite((char*)record,record->header.total_len,1,db_fd);
	fclose(db_fd);
	
	load_index();
}

int get_cmd_idx(char *cmd) {
	int numCommands = sizeof(commands) / sizeof(commands[0]);
	for (int i = 0; i < numCommands; i++) {
        if (strcmp(cmd, commands[i]) == 0) {
            return i;
        }
    }
	return -1;
}

char* eval(char *cmd, char *key, char*val) {
	int cmd_idx = get_cmd_idx(cmd);
	switch(cmd_idx) {
		case 0:
			return get(key);
		case 1:
			if(!val) 
				return "Insufficient Data";
			put(key,val,strlen(val));
			return "DONE";
		case 2:
			del(key);
			return "DONE";
		default:
			return "UNKNOWN COMMAND";	
	}
	return "UNKNOWN COMMAND";	
}

int main(int argc, char ** argv) {
	char *key;
	char *val;
	char *cmd;
	char *token,*sep=" ",*brkt;
	char input[MAX_INPUT_SIZE];

	ht = create_hashtable(SIZE);
	load_index();
	print_index();

	while (1) {
        printf("\n> "); // Prompt
        if(fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            printf("\nGoodbye!\n");
            break;
        }

        // Remove the trailing newline character from fgets
        input[strcspn(input, "\n")] = '\0';

        if(strcmp(input, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }

		int i = 0;
		for(token = strtok_r(input, sep, &brkt);
          token;
          token = strtok_r(NULL, sep, &brkt),i++) {
			switch(i) {
				case 0:
					cmd = strdup(token);
					break;
				case 1:
					key = strdup(token);
					break;
				case 2:
					val = malloc(sizeof(char*));
				default:
					// possible mem leaks :)
					val = strcat(val,strdup(token));
					val = strcat(val," ");
			}
		}

        // Evaluate and print tddhe result
        char *result = eval(cmd,key,val);
        printf("Result: %s\n", result);
    }	
	return 0;
}
