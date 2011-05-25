// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>

#include "rsm_client.h"


static void *
releasethread(void *x)
{
  lock_client_cache *cc = (lock_client_cache *) x;
  cc->releaser();
  return 0;
}

int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // assert(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  /* register RPC handlers with rlsrpc */
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::handleRetry); 
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::handleRevoke);
  seq_num=1; //initialize the seq num
  pthread_mutex_init(&client_mutex, NULL);
  pthread_cond_init(&client_cond, NULL);
  pthread_mutex_init(&release_queue_mutex, NULL);
  pthread_cond_init(&release_queue_cond, NULL);
	pthread_cond_init(&release_queue_cond11, NULL);
  pthread_t th;
  int r = pthread_create(&th, NULL, &releasethread, (void *) this);
  assert (r == 0);
	//lab 8 
	//create rsm_client object
	clm = new rsm_client(xdst);
	
	int rt;
	int ret = clm -> call(lock_protocol::subscribe, cl->id(), id, rt);   //cl should not change
	assert(ret == lock_protocol::OK);
}

lock_client_cache::~lock_client_cache()
{
	//destructor
	std::cout << "START THE DESTRUCtOr\n";
	pthread_mutex_lock(&client_mutex);
	std::map<lock_protocol::lockid_t,  client_lock * >::iterator it;
	for (it = client_locks.begin(); it != client_locks.end(); it++) {
		if (1) {
			int seq_id_tmp = (*it).second-> seq_id;
			lock_protocol::lockid_t lock_id_tmp = (*it).first;
			int r;
			
			pthread_mutex_unlock(&client_mutex);
			int ret = clm -> call(lock_protocol::release, cl->id(), id, seq_id_tmp, lock_id_tmp, r); //lab8 cl to clm
			pthread_mutex_lock(&client_mutex);
			(*it).second-> clstat == NONE;
		}
		
	}
	pthread_mutex_unlock(&client_mutex);
//	int rt;
//	int ret = cl -> call(lock_protocol::handleUnsubscribe, cl->id(), 0, rt);
}

void
lock_client_cache::releaser()
{

  // This method should be a continuous loop, waiting to be notified of
  // freed locks that have been revoked by the server, so that it can
  // send a release RPC.
	pthread_mutex_lock(&client_mutex);
	while(1){
	  
      printf("START releaser\n");
	start_rel:
		for(std::map<lock_protocol::lockid_t ,client_lock *>::iterator it = release_map.begin();it !=release_map.end();it++ ){

		  if((*it).second->clstat == RELEASING){
			  printf("[releaser ]: prepare for releasing\n");
			  pthread_cond_t tmpcv = (*it).second->cv;
			  (*it).second->clstat = NONE;
			  int seq_id_tmp = (*it).second-> seq_id;
			  lock_protocol::lockid_t lock_id_tmp = (*it).first;
			  int r;
//			  release_map.erase(lock_id_tmp);
			  pthread_mutex_unlock(&client_mutex);
			  
			  //before  call rpc, do flush
			  if(lu){
				  lu -> dorelease(lock_id_tmp);
			  }
			  
			  int ret = clm ->call(lock_protocol::release, cl->id(), id, seq_id_tmp, lock_id_tmp, r);  //no need get feedback
			  pthread_mutex_lock(&client_mutex);
			  if (ret == lock_protocol::OK) {
					release_map.erase(lock_id_tmp);
			  }
			  
				pthread_cond_signal(&tmpcv);
			  goto start_rel;

		  }

		}
		//check before sleep
//		for(std::map<lock_protocol::lockid_t ,client_lock *>::iterator it = release_map.begin();it !=release_map.end();it++ ){
//			if((*it).second->clstat == RELEASING){
//				printf("another loop releasing\n");
//				goto start_rel;
//			}
//			
//		}
		printf("releaser over to sleep\n");
		pthread_cond_wait(&release_queue_cond,&client_mutex);
   
	}
	pthread_mutex_unlock(&client_mutex);

}




lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  //remember to send id  to the server
  pthread_mutex_lock(&client_mutex);
  printf("[acquire lock: %lld in clt:%d thread %lld]\n",lid, cl->id(),pthread_self());
	
	// if no lock, create it
  if(client_locks.count(lid) == 0){
    client_lock *newcl = new client_lock();
    newcl->clstat = NONE;
    newcl->acquire_queue;
    newcl->releasing_flag=false;
    newcl->seq_id=0;
    newcl->lock_id=lid;
	  newcl->received_flag=false;
	  pthread_cond_init(&(newcl -> cv), NULL);
    client_locks[lid] = newcl;

  }
	
	client_lock *l= client_locks[lid];
	pthread_t tid = pthread_self();
	
starts:
	bool doRPC = false;
	switch (l -> clstat) {
		case ACQUIRING:
			//do nothing
//			struct timeval now;
//			struct timespec next_timeout;
//			gettimeofday(&now, NULL);
//			next_timeout.tv_sec = now.tv_sec + 10;
//			next_timeout.tv_nsec = 0;
//			
//			pthread_cond_timedwait(&l -> cv, &client_mutex, &next_timeout);
//			doRPC = true;
//			break;
		case LOCKED:
			while ( l -> clstat == LOCKED || l -> clstat == ACQUIRING ) {
				pthread_cond_wait(&l -> cv, &client_mutex);
			}
			goto starts;
		case FREE:
			l -> clstat = LOCKED;
			break;
		case NONE:
			doRPC = true;
			break;
		case RELEASING:
			while (l -> clstat == RELEASING) {
				pthread_cond_wait(&l -> cv, &client_mutex);
			}
			goto starts;
	}

  if (doRPC) {
	  printf("sending acquire rpc\n");
	  l -> clstat = ACQUIRING;
	  int r;
	  int seq_now = seq_num;
	  seq_num++;
//	  retry_map.erase(lid);  //we have retried
	  pthread_mutex_unlock(&client_mutex);
	  int ret = clm -> call(lock_protocol::acquire, cl->id(), id, seq_now, lid, r);
	  pthread_mutex_lock(&client_mutex);
	  
	  if (ret == lock_protocol::OK) {
		  printf("client %d  thread: %lld SUCCEED getting the lock from server\n",cl->id(),pthread_self());
		  l -> clstat = LOCKED;
		  l -> seq_id = seq_now;
		  pthread_mutex_unlock(&client_mutex);
		  return lock_protocol::OK;
		  
	  }else if (ret == lock_protocol::RETRY) {
		  printf("client %d  is UNABLE to get the lock from server\n",cl->id());
		  l -> clstat = ACQUIRING;  
		  			struct timeval now;
		  			struct timespec next_timeout;
		  			gettimeofday(&now, NULL);
		  			next_timeout.tv_sec = now.tv_sec + 1;
		  			next_timeout.tv_nsec = 0;
		  			
		  			pthread_cond_timedwait(&release_queue_cond11, &client_mutex, &next_timeout);
		  
					l -> clstat = NONE;
		  
//		  if (retry_map.find(lid) == retry_map.end()) { //all has been retried
//			  //waiting for result
//		  }else {
//			  //still need retry
//			  l -> clstat = NONE;
//		  }
		  goto starts;
	  }

  }
 
  pthread_mutex_unlock(&client_mutex);
  return lock_protocol::OK;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  pthread_mutex_lock(&client_mutex);
  printf("[release lock %lld in clt %d thread %lld] \n",lid, cl->id(), pthread_self());
  client_lock *l = client_locks[lid];
	
  //assert(l->clstat == LOCKED);
	if (release_map.find(lid) == release_map.end()) { //not in the release map
		printf("not in the revoke queue, just cache it\n");
		l -> clstat = FREE;
		pthread_cond_broadcast(&l -> cv);
	}else {
		l -> clstat = RELEASING;
		pthread_cond_signal(&release_queue_cond);
	}

	
  pthread_mutex_unlock(&client_mutex);
  return lock_protocol::OK;
}



rlock_protocol::status
lock_client_cache::handleRevoke(int clt, int seq, lock_protocol::lockid_t lid, int &r)
{
  //dont forget sequence number 
    pthread_mutex_lock(&client_mutex);
  client_lock *l = client_locks[lid];
  //the lock can only be locked or free
	if (l -> seq_id > seq) {
		//old request, discard
		pthread_mutex_unlock(&client_mutex);
		return rlock_protocol::OK;
	}
	if (l -> seq_id < seq) {
		l -> clstat = LOCKED;
	}
	if (l -> seq_id == seq) {
		//go on 
		
	}
	if (l -> clstat != FREE && l -> clstat != LOCKED) {
		pthread_mutex_unlock(&client_mutex);
		return rlock_protocol::OK;
	}
	assert (l-> clstat == FREE || l -> clstat == LOCKED);
	release_map[lid] = l;   //waiting for releasing
	if (l -> clstat == FREE) {
		l -> clstat = RELEASING;
		pthread_cond_signal(&release_queue_cond);
	}

//  pthread_cond_signal(&release_queue_cond);

  
  pthread_mutex_unlock(&client_mutex);

  return rlock_protocol::OK;  //successfully send 'revoke' rpc
}

rlock_protocol::status
lock_client_cache::handleRetry(int clt, int seq, lock_protocol::lockid_t lid, int &r)
{
  //handller for retry rpc, wake up the waiting thread for "acquire"
  
  pthread_mutex_lock(&client_mutex);
	
  client_lock *l = client_locks[lid];
	
	if (l -> seq_id > seq) {
		//old request, discard
		pthread_mutex_unlock(&client_mutex);
		return rlock_protocol::OK;
	}
	if (l -> seq_id < seq) {
		l -> clstat = ACQUIRING;
	}
	if (l -> seq_id == seq) {

	}
	assert( l -> clstat == ACQUIRING );
  //wake up the blocked thread
	l -> clstat =NONE;
	retry_map[lid] = l;
  pthread_cond_broadcast(&l->cv);

  pthread_mutex_unlock(&client_mutex);
  return rlock_protocol::OK; // successfully send retry rpc
}


