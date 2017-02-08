#include        <time.h>
#include        <sys/time.h>
#include        <sys/types.h>
#include        <sys/socket.h>
#include        <netinet/in.h>
#include        <errno.h>
#include        <netdb.h>
#include        <pthread.h>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdio.h>
#include        <string.h>
#include        <semaphore.h>
#include        <unistd.h>
#include        <limits.h>
#include        <pthread.h>

#include        "socketutil.h"

   
#define PORT            "59393"

pthread_mutex_t recvmute;
pthread_cond_t recvdone;




int fillbuffer(FILE* fp, int sequence,int col,filebuffer* filebuf){
 
  
  char* prompt;
  int i;
  int endseq = sequence+10;
  packetstruct* built;
  int thiscol = col; 
  for(i=sequence;i<endseq;i++){
    
    if(i == INT_MAX){
      i = 0;
      if(thiscol == 1){
	thiscol = 0;
      }
      else{
	thiscol = 1;
      }
    }
    if(findpacket(i,filebuf)!=NULL){
      continue;
    }
    if(!feof(fp)){
      built = (packetstruct*) buildpacketfromfile(fp,i,thiscol,0);
      if(addtobuff(filebuf,built)==0){
	printf("duplicate or filled for seq %d\n",i);
      }
      built = NULL;

    }
    else{
      prompt = "\0";

      built = (packetstruct*) buildpacketfromtext(prompt,i,col,4);

      if(addtobuff(filebuf,built)==0){
        printf("duplicate or filled for seq %d\n",i);
      }
      built = NULL;

      break;
    }
  }
  return i+1;
      
}

int throwpackets(int currseq,filebuffer* filebuf,int sock,struct sockaddr_in* sockaddr_ptr){

  packetstruct* packptr;
  char* packet;
  int i;

  for(i=currseq;i<currseq+10;i++){
    packptr = findpacket(i,filebuf);
    
    if(packptr==NULL){
      return 1;
    }
    packet = (char*) packptr;
    
    if ( sendto( sock, packet, sizeof(packetstruct)+packptr->size, 0, (struct sockaddr *)sockaddr_ptr, sizeof(*sockaddr_ptr )) < 0 ){

      printf( "\x1b[2;31msendto() failed file %s line %d errno %d %s\x1b[0m\n", __FILE__,__LINE__ , errno, strerror(errno) );
      
      return -1;
    }
    

  }
  return 0;
}

 

void* readfrom(void* sock_desc){
  int sock = *(int*)sock_desc;
  char recvmess[2000];
  int read_size;
  packetstruct* ptr;
  socklen_t sz;
  struct sockaddr_in addr;
  char* buffer2;
  char origin[40];
  if ( (read_size = recvfrom( sock, recvmess, sizeof(recvmess), 0, (struct sockaddr *)&addr, &sz )) > 0 )
    {
      
      buffer2 = (char*)malloc(read_size+1);

      memmove(buffer2,recvmess,read_size+1);
      bzero(recvmess,sizeof(recvmess));
      ptr = (packetstruct*) buffer2;
      ptr->data =  (ptr+1);
      
      if(ptr->flag == 2){
	printf( "\nreceived nack for packet: %d from %s port %d:\n",ptr->sequence,get_istring( ntohl( addr.sin_addr.s_addr ), origin, sizeof(origin) ), ntohs( addr.sin_port ));      
	
	
      }
      if(ptr->flag == 5){
	printf( "\nThis reciever has a complete file:  %d from %s port %d:\n",ptr->sequence,get_istring( ntohl( addr.sin_addr.s_addr ), origin, sizeof(origin) ), ntohs( addr.sin_port ));

      }
     
      bzero(recvmess,sizeof(recvmess));
      
      
    }
  pthread_cond_signal(&recvdone);
  return buffer2;
}

packetstruct* recvtimer(struct timespec *max_wait,void* sock_desc)
{
  struct timespec abs_time;
  pthread_t tid;
  int err;
  packetstruct* nackptr;
  pthread_mutex_lock(&recvmute);

  nackptr = NULL;
  /* pthread cond_timedwait expects an absolute time to wait until */
  clock_gettime(CLOCK_REALTIME, &abs_time);
  abs_time.tv_sec += max_wait->tv_sec;
  abs_time.tv_nsec += max_wait->tv_nsec;

  pthread_create(&tid, NULL, readfrom,sock_desc );

  /* pthread_cond_timedwait can return spuriously: this should
   * be in a loop for production code
   */
  err = pthread_cond_timedwait(&recvdone, &recvmute, &abs_time);

  if (err == ETIMEDOUT){
    fprintf(stderr, "%s: calculation timed out\n", __func__);
    pthread_mutex_unlock(&recvmute);
    return NULL;
  }
  
  if (!err){
    pthread_mutex_unlock(&recvmute);
    pthread_join(tid,(void**)&nackptr);
  }
  return nackptr;
}





int main( int argc, char ** argv )
{
  
  packetstruct* packetptr;
  packetstruct* nacketptr;
  struct addrinfo addrinfo;
  struct addrinfo *result;
  int on = 1;
  int sd;
  struct sockaddr_in *sockaddr_ptr;
  int len;
  char buffer[1024];
  char*  prompt = "Enter a filename>>";
  const char *func = "main";
  FILE* fp;
  char* packet;
  int sequence = 0;
  int sendseq = 0;
  int nextsequence;
  int col = 0;
  
  filebuffer* filebuf = NULL;

  nacketptr = NULL;
  pthread_mutex_init(&recvmute,NULL);
  pthread_cond_init(&recvdone,NULL);
  filebuf = initializefile();

  addrinfo.ai_flags = 0;
  addrinfo.ai_family = AF_INET;
  addrinfo.ai_socktype = SOCK_DGRAM;// I want connectionless datagrams
  addrinfo.ai_protocol = 0;
  addrinfo.ai_addrlen = 0;
  addrinfo.ai_addr = NULL;
  addrinfo.ai_canonname = NULL;
  addrinfo.ai_next = NULL;

  //if ( getaddrinfo( "128.6.5.127", PORT, &addrinfo, &result ) != 0 )// broadcast address
  if ( getaddrinfo( "255.255.255.255", PORT, &addrinfo, &result ) != 0 )// broadcast address
    {
      printf( "getaddrinfo() failed in %s()\n", func );
      return -1;
    }
  else if ( (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
    {
      freeaddrinfo( result );
      printf( "socket() failed in %s()\n", func );
      return -1;
    }
  else if ( setsockopt( sd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on) ) == -1 )
    {
      freeaddrinfo( result );
      printf( "setsockopt() failed in %s()\n", func );
      return -1;
    }
  else
    {
      sockaddr_ptr = (struct sockaddr_in *)(result->ai_addr);
      printf( "datagram target is %s port %d\n", get_istring( ntohl( sockaddr_ptr->sin_addr.s_addr ), buffer, sizeof(buffer) ), ntohs( sockaddr_ptr->sin_port ) );
      
        

      if ( write( 1, prompt, strlen(prompt)+1), (len = read( 0, buffer, sizeof(buffer) )) > 0 ){
	
	buffer[ len-1 ] = '\0';
	printf( "%s now opening file %s\n", argv[0],buffer );
	fp = fopen(buffer, "r");
	if(fp == NULL){
	  perror(" no such file");
	  freeaddrinfo(result);
	  return -1;
	}
      }
      packet =  buildpacketfromtext(buffer,sequence,col,3);
      bzero(buffer,sizeof(buffer));
      packetptr = (packetstruct*) packet;
      
      addtobuff(filebuf,packetptr);
      
      struct timespec max_wait;

      memset(&max_wait, 0, sizeof(max_wait));

      /* wait at most 5 seconds */
      max_wait.tv_sec = 10;

      
      sequence++;
      
      nextsequence = fillbuffer(fp,sequence,col,filebuf);
      sequence = nextsequence;
      
      while(1){
	printf("throwing packs %d to %d\n",sendseq,sendseq+10);
	throwpackets(sendseq,filebuf,sd,sockaddr_ptr);
	  
	nacketptr = recvtimer(&max_wait,&sd);
	if( nacketptr ==NULL){
	  continue;  
	}
	else{
	  nextsequence = fillbuffer(fp,nacketptr->sequence,col,filebuf);
	  sendseq = nacketptr->sequence;
	  free(nacketptr);
	} 
	
      }
      
     
      
      
     
     
     freeaddrinfo( result );
     
     
     printf( "Normal end of %s\n", argv[0] );
  
  
     return 0;
    }
  
}
