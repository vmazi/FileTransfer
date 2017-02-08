#include  <netinet/in.h>
#include  <sys/socket.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <netdb.h>
#include  <strings.h>
#include "socketutil.h"



filebuffer* initializefile(){
  filebuffer* this;
  this = (filebuffer*)malloc(sizeof(filebuffer));
  this->currentseq = 0;
  this->buffer = constructor(10000);
  pthread_mutex_init(&this->buffmute,NULL);
  return this;
}


char* keygen(int seq){
  char* result;
  result = (char*)malloc(16*sizeof(char));
  sprintf(result,"%d",seq);
  return result;
}
 
int addtobuff(filebuffer* buffer,packetstruct* packetptr){
  int result;
  char* key;
  key = keygen(packetptr->sequence);
  pthread_mutex_lock(&buffer->buffmute);
  
  result = set(key,packetptr,buffer->buffer);
 
  pthread_mutex_unlock(&buffer->buffmute);
  free(key);
  return result;
}
int deletefrombuff(int key, filebuffer* filebuf){

  char* keystr;
  char* packet;
  packetstruct* packetptr;
  
  keystr = keygen(key);
  pthread_mutex_lock(&filebuf->buffmute);

  packetptr = delete(keystr,filebuf->buffer);

  pthread_mutex_unlock(&filebuf->buffmute);

  if(packetptr == NULL){
    return 1;
  }
  else{
    packet = (char*) packetptr;
    free(packet);
    free(keystr);
    return 0;
  }

}

packetstruct* findpacket(int seq, filebuffer* filebuf){
  
  packetstruct* pack;
  char* keystr;
  keystr = keygen(seq);
  pthread_mutex_lock(&filebuf->buffmute);

  pack = get(keystr,filebuf->buffer);

  pthread_mutex_unlock(&filebuf->buffmute);

  if(pack == NULL){
    return NULL;
  }
  else{
    return pack;
  }

}



char* buildpacketfromfile(FILE* fp, int sequence, int col,int flag){
  char* packet;
  char buffer[5];
  int sizef;
  packetstruct* ptr;
  sizef = fread(buffer, sizeof(char),sizeof(buffer),fp);

  packet = (char*)calloc(1,sizef+sizeof(packetstruct));

  ptr = (packetstruct*)packet;
  ptr->qualcheck = qualitycode;
  ptr-> sequence = sequence;
  ptr->col = col;
  ptr->flag = flag;
  ptr->size = sizef;
  ptr->data = (void*) (ptr+1);

  memmove(ptr->data,buffer,sizef);
  return packet;

}

char* buildpacketfromtext(char* data, int sequence, int col, int flag){
  char* packet;

  int sizef;
  packetstruct* ptr;
  sizef = strlen(data)+1;
  packet = (char*)calloc(1,sizef+sizeof(packetstruct));

  ptr = (packetstruct*)packet;
  ptr->qualcheck = qualitycode;
  ptr-> sequence = sequence;
  ptr->col = col;
  ptr->flag = flag;
  ptr->size = sizef;
  ptr->data = (void*) (ptr+1);

  memmove(ptr->data,data,sizef);
  return packet;


}


long get_iaddr( const struct sockaddr_in sockaddr)
{
  return ntohl( sockaddr.sin_addr.s_addr );
}

int get_port(const struct sockaddr_in sockaddr )
{
  return ntohs( sockaddr.sin_port );
}

void set_iaddr(struct sockaddr_in  sockaddr, long x, unsigned int port )
{
  memset( &sockaddr, 0, sizeof(sockaddr) );
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons( port );
  sockaddr.sin_addr.s_addr = htonl( x );
}

void set_iaddr_str(struct sockaddr_in  sockaddr, char * x, unsigned int port )
{
  memset( &sockaddr, 0, sizeof(sockaddr) );
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons( port );
  //arati
  struct hostent * hostptr;
  if((hostptr = gethostbyname(x)) == NULL)
    {
      printf( "Error getting addr information\n" );
    }
  else
    {
      bcopy(hostptr->h_addr,(char *)&sockaddr.sin_addr,hostptr->h_length);
    }
}

long get_host_addr( char * name )
{
  struct sockaddr_in sockaddr;
  struct hostent *h;
  
  if ( (h = gethostbyname( name )) == NULL )
    {
      return 0;
    }
  else
    {
      bcopy(h->h_addr,(char *)&sockaddr.sin_addr,h->h_length);
      return get_iaddr( sockaddr );
    }
}

char * get_istring( unsigned long x, char * s, unsigned int len )
{
  int a,b,c,d;

  d = x & 0x000000ff;
  c = (x >> 8) & 0x000000ff;
  b = (x >> 16) & 0x000000ff;
  a = (x >> 24) & 0x000000ff;
  snprintf( s, len, "%d.%d.%d.%d", a,b,c,d );
  s[len-1] = '\0';
  return s;
}

long get_iaddr_string( char * string )
{
  int a,b,c,d;

  if ( string == 0 )
    {
      return 0;
    }
  else if ( sscanf( string, "%d.%d.%d.%d", &a, &b, &c, &d ) < 4 )
    {
      return 0;
    }
  else
    {
      return (a<<24) | (b<<16) | (c<<8) | d;
    }
}
