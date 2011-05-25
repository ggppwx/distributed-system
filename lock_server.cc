// the lock server implementation
// the lock_server.cc is linked with lock_smain.cc then transfered to lock_server!!!!!


#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <pthread.h>



lock_server::lock_server():
  nacquire (0)  
{
	assert(0==pthread_mutex_init(&count_mutex, NULL));
	assert(0==pthread_cond_init (&count_threshold_cv, NULL));
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::handleAcquire(int clt, lock_protocol::lockid_t lid, int &r)
{
	//check lock_status
	// if the lock is locked, and if so, the handler blocks until 
	//the lock is free. When the lock is free, acquire changes its
	//state to locked, then returns to the client, which indicates
	//that the client now has the lock. The value r returned by acquire doesn't matter.
	//printf("receive request in handleAcquire of lock_server\n ");
	lock_protocol::status ret=lock_protocol::OK;
	
	//printf("acquire request from clt %d\n", clt);
	//lab1
	int rc;
	
	assert(0==pthread_mutex_lock(&count_mutex));  //now the mutex works!!!!!!!
	while (lock_status[lid]==LOCKED) {
		//block
		//r=-1;
		assert(0==pthread_cond_wait(&count_threshold_cv, &count_mutex));
		printf("thread wait\n");
		
		
	}
	if (lock_status[lid]==FREE) {
		//change status
		printf(".....lock %lld requirement from clt %d is admitted....\n",lid,clt);
		lock_status[lid]=LOCKED;
		
	}     
	
	assert(0==pthread_mutex_unlock(&count_mutex));
	return ret;
}

lock_protocol::status
lock_server::handleRelease(int clt, lock_protocol::lockid_t lid, int &)
{
	//check lock_status
	//The handler for release changes the lock state to free, and 
	//notifies any threads that are waiting for the lock.
	lock_protocol::status ret=lock_protocol::OK;
	
	printf("...release lock id : %lld request from clt %d \n",lid ,clt);
	//lab1
	
	assert(0==pthread_mutex_lock(&count_mutex));
	lock_status[lid]=FREE;   //set status FREE before signal the waiting thread
	assert(0==pthread_cond_signal(&count_threshold_cv));
	assert(0==pthread_mutex_unlock(&count_mutex));
	
	return ret;


}
