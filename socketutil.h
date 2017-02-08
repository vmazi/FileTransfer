#ifndef SOCKETUTIL_H_
#define SOCKETUTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include "hashtable.h"

#define qualitycode (2492)


typedef struct filebuffer{

  hashtable* buffer;
  pthread_mutex_t buffmute;
  int currentseq;

}filebuffer;

typedef struct recvlistenarg{
  filebuffer* filebuf;
  int sd;
}recvlistenarg;

typedef struct packetstruct{
  int qualcheck;
  int sequence;
  int col;
  int flag;
  int size;
  void* data;
}packetstruct;

typedef struct assemblestruct{
  FILE* fp;
  packetstruct* nacketpacket;

}assemblestruct;

filebuffer* initializefile();
char* keygen(int seq);
int addtobuff(filebuffer* buffer,packetstruct* packetptr);
int deletefrombuff(int key, filebuffer* filebuf);
packetstruct* findpacket(int seq, filebuffer* filebuf);

char* buildpacketfromfile(FILE* fp, int sequence, int col,int flag);
char* buildpacketfromtext(char* data, int sequence, int col, int flag);
long get_iaddr(const struct sockaddr_in sockaddr );
int get_port(const struct sockaddr_in sockaddr);
void set_iaddr(struct sockaddr_in sockaddr, long x, unsigned int port);
void set_iaddr_str(struct sockaddr_in sockaddr, char * x, unsigned int port );
long get_host_addr( char * name);
char *get_istring( unsigned long x, char * s, unsigned int len );
long get_iaddr_string( char * string);

#endif
