#ifndef HASHTABLE_H_
#define HASHTABLE_H_


typedef struct hashnode
{
  
  char* key;
  void* value;
  struct hashnode* next;
}hashnode;
typedef struct hashtable
{
  int size;
  int items;
  hashnode ** table;
}hashtable;



void destroy(hashtable* root);//frees hashtable after use
hashtable* constructor(int size);//creates hashtable with (size) buckets
int rol(unsigned value, int places);//rotates bits from left to right
int hashl(char const *input,int size);//calls ROL on key to generate hashtable index
int set(char* key, void* value,hashtable* root);//insert value into hashtable, return 1 if succes, 0 if not;
void* get(char*key,hashtable* root);//retrieves value with key (IMPORTANT, it returns void* value, NOT a hashnode*)
void* delete (char* key,hashtable* root);//delets value with key returning unsigned reference to value or null if not found
int load();//returns number of items/max size, i.e. load factor

#endif // HASHTABLE_H_

