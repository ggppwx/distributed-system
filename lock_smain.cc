#include "rpc.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include "lock_server_cache.h"
#include "paxos.h"
#include "rsm.h"

#include "jsl_log.h"



int
main(int argc, char *argv[])
{

	
//	assert(0==pthread_mutex_init(&count_mutex, NULL));
//	assert(0==pthread_cond_init (&count_threshold_cv, NULL));
	
  int count = 0;

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  srandom(getpid());

  if(argc != 3){
    fprintf(stderr, "Usage: %s [master:]port [me:]port\n", argv[0]);
    exit(1);
  }
	

  char *count_env = getenv("RPC_COUNT");
  if(count_env != NULL){
    count = atoi(count_env);
  }

  //jsl_set_debug(2);
  // Comment out the next line to switch between the ordinary lock
  // server and the RSM.  In Lab 7, we disable the lock server and
  // implement Paxos.  In Lab 8, we will make the lock server use your
  // RSM layer.
#define	RSM
//#ifdef RSM
  rsm rsm(argv[1], argv[2]);
//#endif

//#ifndef RSM
  // lock_server ls;
/*	lock_server_cache lsc;   */
	lock_server_cache lsc(&rsm);  //lab 8
	
	
 /* rpcs server(atoi(argv[1]), count); //before lab 8  */
	
  // server.reg(lock_protocol::stat, &lsc, &lock_server_cache::stat);
	
	
/*  before lab 8
	server.reg(lock_protocol::subscribe, &lsc,&lock_server_cache::handleSubscribe);
	server.reg(lock_protocol::acquire, &lsc, &lock_server_cache::handleAcquire );
	server.reg(lock_protocol::release, &lsc, &lock_server_cache::handleRelease );
	server.reg(lock_protocol::unsubscribe, &lsc,&lock_server_cache::handleUnsubscribe);
*/	
	rsm.reg(lock_protocol::subscribe, &lsc,&lock_server_cache::handleSubscribe);
	rsm.reg(lock_protocol::acquire, &lsc, &lock_server_cache::handleAcquire );
	rsm.reg(lock_protocol::release, &lsc, &lock_server_cache::handleRelease );
	rsm.reg(lock_protocol::unsubscribe, &lsc,&lock_server_cache::handleUnsubscribe);
	printf("successfully register\n");
//#endif


  while(1)
    sleep(1000);
}
