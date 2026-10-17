#define _POSIX_C_SOURCE 200809L
#include "dsr_activity.h"
#include "dsr_activity_internal.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);abort();}}while(0)
static pthread_barrier_t barrier;
static bool won[2],both_restart;
static void rendezvous(void){int r=pthread_barrier_wait(&barrier);if(r!=0&&r!=PTHREAD_BARRIER_SERIAL_THREAD)abort();}
static void *contender(void *arg)
{
 unsigned id=*(unsigned *)arg;
 for(unsigned iteration=0;iteration<2000;iteration++){
  rendezvous();won[id]=(id||both_restart)?dsr_activity_restart_reserve():dsr_activity_begin();
  rendezvous();rendezvous();
  if(won[id]){if(id||both_restart)dsr_activity_restart_cancel();else dsr_activity_end();}
  rendezvous();
 }
 return NULL;
}
static void race(bool restart_only)
{
 both_restart=restart_only;CHECK(!pthread_barrier_init(&barrier,NULL,3));pthread_t threads[2];unsigned ids[]={0,1};
 for(unsigned i=0;i<2;i++)CHECK(!pthread_create(&threads[i],NULL,contender,&ids[i]));
 for(unsigned iteration=0;iteration<2000;iteration++){
  rendezvous();rendezvous();CHECK(won[0]!=won[1]);
  CHECK(dsr_activity_restart_pending()==(won[1]||both_restart));
  if(dsr_activity_restart_pending())CHECK(!dsr_activity_begin());
  else CHECK(!dsr_activity_restart_reserve());
  rendezvous();rendezvous();CHECK(!dsr_activity_restart_pending());
 }
 for(unsigned i=0;i<2;i++)CHECK(!pthread_join(threads[i],NULL));
 CHECK(!pthread_barrier_destroy(&barrier));
 CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();
}
static void *writer(void *arg)
{(void)arg;for(unsigned i=0;i<20000;i++){if(!dsr_activity_begin())abort();if(!dsr_activity_begin())abort();dsr_activity_end();dsr_activity_end();}return NULL;}
int main(int argc,char **argv)
{
 CHECK(argc==2);
 if(!strcmp(argv[1],"basic")){
  CHECK(!dsr_activity_restart_pending()&&!dsr_activity_restart_dispatch());
  CHECK(dsr_activity_begin()&&dsr_activity_begin()&&!dsr_activity_restart_reserve());
  dsr_activity_restart_cancel();CHECK(!dsr_activity_restart_pending());
  dsr_activity_end();CHECK(!dsr_activity_restart_reserve());dsr_activity_end();
  CHECK(dsr_activity_restart_reserve()&&dsr_activity_restart_pending()&&!dsr_activity_begin());
  CHECK(!dsr_activity_restart_reserve());dsr_activity_restart_cancel();
  CHECK(dsr_activity_begin());dsr_activity_end();CHECK(dsr_activity_restart_reserve()&&dsr_activity_restart_dispatch());
  CHECK(dsr_activity_restart_pending()&&!dsr_activity_begin()&&!dsr_activity_restart_dispatch()&&!dsr_activity_restart_reserve());
  dsr_activity_restart_cancel();CHECK(!dsr_activity_begin()&&dsr_activity_restart_pending());
 }else if(!strcmp(argv[1],"fault")){
  dsr_activity_end();CHECK(dsr_activity_restart_pending()&&!dsr_activity_begin()&&!dsr_activity_restart_reserve()&&!dsr_activity_restart_dispatch());
  dsr_activity_restart_cancel();CHECK(dsr_activity_restart_pending()&&!dsr_activity_begin());
 }else if(!strcmp(argv[1],"pending-fault")){
  CHECK(dsr_activity_restart_reserve());dsr_activity_end();dsr_activity_restart_cancel();
  CHECK(dsr_activity_restart_pending()&&!dsr_activity_begin()&&!dsr_activity_restart_dispatch());
 }else if(!strcmp(argv[1],"writer-race"))race(false);
 else if(!strcmp(argv[1],"restart-race"))race(true);
 else if(!strcmp(argv[1],"writers")){
  pthread_t threads[8];for(unsigned i=0;i<8;i++)CHECK(!pthread_create(&threads[i],NULL,writer,NULL));
  for(unsigned i=0;i<8;i++)CHECK(!pthread_join(threads[i],NULL));
  CHECK(dsr_activity_restart_reserve());dsr_activity_restart_cancel();CHECK(!dsr_activity_restart_pending());
 }else CHECK(false);
 printf("PASS: %u genuine activity atomic checks (%s); native pthread scheduling, no ESP proof\n",checks,argv[1]);return 0;
}
