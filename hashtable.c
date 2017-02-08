#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "hashtable.h"


void destroy(hashtable* root)
{
  int i;
  int items;  
  hashnode* tmp;
  hashnode* trash;
  hashnode** tmp1;
  
  if (root == NULL||root->table == NULL)
    {
      return;
    }
  else
    {
       items = root->items;
       tmp1= root->table;
       for(i=0;i<items;i++)
	 {
	   tmp = *tmp1;
	   while (tmp!=NULL)
	     {
	       if(tmp->next!=NULL)
		 {
		   trash = tmp->next;
		   free(tmp->key);
		   
		   free(tmp);
		   tmp = trash;
		   i++;
		   continue;
		 }
	       else
		 {
		   free(tmp->key);
		   
		   free(tmp);
		   tmp = NULL;
		    
		 }
	     }
	   tmp1++;
	 }
      
       free(root->table);       
       free(root);
       root = NULL;
    }
}
hashtable* constructor(int size)
{
  int i;
  struct hashnode** ptr = NULL;
  struct hashtable* new = NULL;
  new = (struct hashtable*)malloc(1*sizeof(struct hashtable));
  new->table = (struct hashnode**)malloc(size*sizeof(struct hashnode*));
  ptr = new->table;
  for(i=0;i<size;i++)
    {
      ptr[i] = NULL;
    }
  new->size = size;
  new->items = 0;
  return new;

}

  
int rol(unsigned value, int places) 
{  
  int WORD_LENGTH = (8*sizeof(value));
  return((value<<places)|(value>>(WORD_LENGTH-places))); 
  		    
}
			 
int hashl(char const *input,int size) 
{ 
  int result = 0x55555555;
  
  while (*input) 
    { 
      result ^= *input++;
      result = rol(result, 5);
    }
  return (abs(result)%size);
}


int set(char* key, void* value,hashtable* root)
{
  int hashindex;
  hashindex = hashl(key, root->size);
  struct hashnode* new = (struct hashnode*) malloc(1*sizeof(struct hashnode));
  new->key = (char*) malloc(strlen(key)+1);
  strcpy(new->key,key);
  new->value = value;
  new->next = NULL;




  if(root->table[hashindex]==NULL)
    {
      root->items++;
      root->table[hashindex] = new;
      return 1;
    }
  else 
    {
      struct hashnode* pointer = root->table[hashindex];
      while(pointer->next!=NULL)
	{
	  if ((strcmp(pointer->key,new->key)==0))
	    {
	      printf("duplicate key\n");
	      free(new->key);
	      free(new);
	      new = NULL;
	      return 0;
	    }
	  else
	    {
	      pointer = pointer->next;
	    }
	  
	}
      root->items++;
     
      if ((strcmp(pointer->key,new->key)==0))
	{
	  root->items --;
	  printf("duplicate key\n");
	  free(new->key);
	  free(new);
	  new = NULL;
	  return 0;
	}
      else
	{
	  pointer->next = new;
	  return 1;
	}
    }

}
void* get(char*key,hashtable* root)
{
  int hashindex = hashl(key,root->size);
  hashnode* pointer = root->table[hashindex];
  if(pointer == NULL)
    {
      return NULL;
    }
  else if(strcmp(pointer->key,key)==0)
    {
      return pointer->value;
    }
  else
    {
      while(pointer!= NULL)
	{
	 if (strcmp(pointer->key,key)==0)
	   {
	     return pointer->value;
	   }
	 else
	   {
	     pointer = pointer->next;
	   }
	}
      return NULL;
    }


}
void* delete (char* key,hashtable* root)
{
  void* ret;
  char* tmp = key;
  hashnode** tmptble = root->table;
  int hashindex = hashl(tmp,root->size);
  hashnode* tmpnode;
  hashnode* trash;
  if (tmptble[hashindex] == NULL)
    {
      return NULL;
    }
  tmpnode = tmptble[hashindex];
  if (strcmp(tmpnode->key,tmp)==0)
    {
      
      
      if (tmpnode->next == NULL)
	{
	  ret = tmpnode->value;
	  free(tmpnode->key);
	  
	  free(tmpnode);
	  tmpnode = NULL;
	  tmptble[hashindex] = NULL;
	  root->items--;
	  return ret;
	}
      else 
	{
	  tmptble[hashindex] = tmpnode->next;
	  ret = tmpnode->value;
	  free(tmpnode->key);
	  
	  free(tmpnode);
	  tmpnode = NULL;
	  root->items--;
	  return ret;
	}
    }
  else
    {
      
      while(tmpnode->next != NULL)
	{
	  if (strcmp(tmpnode->next->key,tmp)==0)
	    {
	      ret = tmpnode->next->value;
	      trash = tmpnode->next;
	      if(tmpnode->next->next)
		{
		  tmpnode->next = tmpnode->next->next;
		  free(trash->key);
		  
		  free(trash);
		  trash = NULL;
		  root->items--;
		  return ret;
		}
	      else
		{
		  tmpnode->next = NULL;
		  free(trash->key);
		 
		  free(trash);
		  trash = NULL;
		  root->items--;
		  return ret;
		}
	    }
	  else
	    {
	      tmpnode = tmpnode->next;
	    }	  
	}
      if (strcmp(tmpnode->key,tmp)==0)
	{
	  

	  
	   
	  ret = tmpnode->value;
	  free(tmpnode->key);
	  
	  free(tmpnode);
	  tmpnode = NULL;
	  
	  root->items--;
	  return ret;
	}

      
      tmpnode = NULL;
      return NULL;
	
    }

} 

int load(hashtable* map){

  if (map->size == map->items){
    return 1;
  }
  else{
    return 0;
  }

}

