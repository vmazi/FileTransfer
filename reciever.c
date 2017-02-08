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
#include        "socketutil.h"

#define PORT "59393"
// UDP datagram receiver test program

pthread_mutex_t recvmute;
pthread_cond_t recvdone;

struct sockaddr_in* senderedaddr;

void* listenandstore(void* args){

  recvlistenarg* arguments;
  arguments = (recvlistenarg*) args;

  int sd = arguments->sd;//
  int lens;
  char buffer[1000];
  struct sockaddr_in addr;
  struct sockaddr_in* addri;
  socklen_t sz;
  int r = 0;
  filebuffer* filebuf = arguments->filebuf;//
  char* buffer2;

  packetstruct* ptr;

  sz = sizeof(addr);
  char origin[40];
  while( (lens = recvfrom( sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &sz )) > 0 )
    {
      buffer[lens] = '\0';

      printf( "received packet from %s port%d\n",get_istring( ntohl( addr.sin_addr.s_addr ), origin, sizeof(origin) ), ntohs( addr.sin_port ) );
      buffer2 = (char*)calloc(1,lens+1);

      memmove(buffer2,buffer,lens+1);
      bzero(buffer,sizeof(buffer));
      ptr = (packetstruct*) buffer2;
      ptr->data =  (void*)(ptr+1);

      addri = (struct sockaddr_in*) malloc(sizeof(addr));
      memmove(addri,&addr,sizeof(addr));

      senderedaddr = addri;

      r = rand()%10;
      if(r<3){



	/*if ( sendto( sd, packet, sizeof(packetstruct)+packetptr->size, 0, (struct sockaddr *)addri, sizeof(addr)) < 0 ){
        printf( "\x1b[2;31msendto() failed file %s line %d errno %d %s\x1b[0m\\n", __FILE__,__LINE__ , errno, strerror(errno) );
        freeaddrinfo( result );
        return -1;
        }*/

	printf("dropping pack: %d\n",ptr->sequence);


      }
      else{

	if(addtobuff(filebuf,ptr)==1){
  
	printf("storing packet: %d\n ",ptr->sequence);
	}
      }
      

    }
  return NULL;
}


struct sockaddr_in* recvtimer(struct timespec *max_wait,int sock_desc,filebuffer* filebuf)
{
  struct timespec abs_time;
  pthread_t tid;
  int err;
  struct sockaddr_in* senderaddr = NULL;
  pthread_mutex_lock(&recvmute);
  recvlistenarg* arguments;

  arguments = (recvlistenarg*) malloc(sizeof(recvlistenarg));
  arguments->sd = sock_desc;
  arguments->filebuf = filebuf;

 
  /* pthread cond_timedwait expects an absolute time to wait until */
  clock_gettime(CLOCK_REALTIME, &abs_time);
  abs_time.tv_sec += max_wait->tv_sec;
  abs_time.tv_nsec += max_wait->tv_nsec;


  pthread_create(&tid, NULL, listenandstore,(void*)arguments);

  /* pthread_cond_timedwait can return spuriously: this should
   * be in a loop for production code
   */
  err = pthread_cond_timedwait(&recvdone, &recvmute, &abs_time);

  if (err == ETIMEDOUT){
    fprintf(stderr, "%s done accepting packs\n", __func__);
    pthread_mutex_unlock(&recvmute);
    pthread_cancel(tid);
  }

  if (!err){
    pthread_mutex_unlock(&recvmute);
    pthread_join(tid,(void**)&senderaddr);
  }
  return NULL;
}

assemblestruct* assemblefile(FILE* fp1,filebuffer* filebuf,int currseq){

  assemblestruct* assemblee;
  int col = 0;
  int i;
  packetstruct* packptr;
  while(1){
    
    packptr = findpacket(currseq,filebuf);
    if(packptr==NULL){
      packptr = (packetstruct*) buildpacketfromtext("\0",currseq,col,2);
      
      assemblee = (assemblestruct*) malloc(sizeof(assemblestruct));
      assemblee->fp = fp1;
      assemblee->nacketpacket = packptr;
      return assemblee;
    }
    packptr->data =(char*)(packptr+1);
    col = packptr->col;
    switch(packptr->flag){

    case 0:
      if(fp1 == NULL){
        printf("there is no file open already\n");
        break;
      }
      printf("writing packet :%d\n",packptr->sequence);
      fwrite(packptr->data,sizeof(char),packptr->size,fp1);
      for(i=0;i<packptr->sequence;i++){
	deletefrombuff(i,filebuf);

      }
      break;
    case 3:
      if(fp1 != NULL){
        printf("there is a file open already\n");

      }
      fp1 =  fopen((char*)packptr->data,"a+");
      printf("creating file %s\n",(char*)packptr->data);
      break;
    case 4:
      if(fp1 == NULL){
        printf("there is no file open already\n");
        break;
      }
      
      fclose(fp1);
      puts("finished download");
      
      assemblee = (assemblestruct*) malloc(sizeof(assemblestruct));
      assemblee->fp = NULL;
      assemblee->nacketpacket = (packetstruct*) buildpacketfromtext("\0",packptr->sequence,col,5);
     
      return assemblee; 
  
    }

    packptr = NULL;
    currseq++;

  }




}



int main( int argc, char ** argv )
{
  struct addrinfo addrinfo;
  struct addrinfo *result;
  int on = 1;
  int sd;
  socklen_t sz;
  struct sockaddr_in addr;
  // struct sockaddr_in* addri;
  int currseq = 0;
  //char buffer[512];
  //char origin[40];
  const char *func = "main";
  //packetstruct* ptr;
  FILE* fp = NULL;
  //char* buffer2;
  int r;
  char* packet;
  packetstruct* nacketpack;
  filebuffer* filebuf = NULL;

  senderedaddr = NULL;

  filebuf = initializefile();

  addrinfo.ai_flags = AI_PASSIVE;
  addrinfo.ai_family = AF_INET;
  addrinfo.ai_socktype = SOCK_DGRAM;
  addrinfo.ai_protocol = 0;
  addrinfo.ai_addrlen = 0;
  addrinfo.ai_addr = NULL;
  addrinfo.ai_canonname = NULL;
  addrinfo.ai_next = NULL;
  if ( getaddrinfo( 0, PORT, &addrinfo, &result ) != 0 )
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
  else if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) == -1 )
    {
      freeaddrinfo( result );
      printf( "setsockopt() failed in %s()\n", func );
      return -1;
    }
  
  else if ( errno = 0, bind( sd, result->ai_addr, result->ai_addrlen ) == -1 )
    {
      freeaddrinfo( result );
      printf( "bind() failed in %s() line %d errno %d\n", func, __LINE__, errno );
      close( sd );
      return -1;
    }
  else
    {
      freeaddrinfo( result );
      struct sockaddr_in sin;

       socklen_t len = sizeof(sin);

      if (getsockname(sd, (struct sockaddr *)&sin, &len) == -1)
	perror("getsockname");
      else
	printf("port number %d\n", ntohs(sin.sin_port));

      sz = sizeof( addr );
      printf( "%s now at line %d ready to receive packets...\n", argv[0], __LINE__ );

      r = 0;
      struct timespec max_wait;

      memset(&max_wait, 0, sizeof(max_wait));

      /* wait at most 5 seconds */
      max_wait.tv_sec = 5;
      recvlistenarg* arguments;

      arguments = (recvlistenarg*) malloc(sizeof(recvlistenarg));
      arguments->sd = sd;
      arguments->filebuf = filebuf;
      
      assemblestruct* assembling;
      
      while(1){
	recvtimer(&max_wait,sd,filebuf);

	assembling = assemblefile(fp,filebuf,currseq);
	nacketpack = assembling->nacketpacket;
	fp = assembling->fp;

	packet = (char*)nacketpack;
	currseq = nacketpack->sequence;
	if (senderedaddr == NULL){
	  continue;
	}
	if(nacketpack == NULL){
	  continue;
	}
	if(nacketpack->flag == 5){
          sleep(2);

        }

	if ( sendto( sd, packet, sizeof(packetstruct)+nacketpack->size, 0, (struct sockaddr *)senderedaddr, sizeof(addr)) < 0 ){
	  printf( "\x1b[2;31msendto() failed file %s line %d errno %d %s\x1b[0m\\n", __FILE__,__LINE__ , errno, strerror(errno) );
	  freeaddrinfo( result );
	  return -1;
	}
	if(nacketpack->flag == 5){
	  return 0;

	}



      }      
      printf( "Execution end of %s len is %d\n", argv[0], len );
      return 0;
    }
}
