// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

static void *
revokethread(void *x)
{
  lock_server_cache *sc = (lock_server_cache *) x;
  sc->revoker();
  return 0;
}

static void *
retrythread(void *x)
{
  lock_server_cache *sc = (lock_server_cache *) x;
  sc->retryer();
  return 0;
}

lock_server_cache::lock_server_cache(class rsm *_rsm) 
  : rsm (_rsm)
{

  pthread_t th;
  int r = pthread_create(&th, NULL, &revokethread, (void *) this);
  assert (r == 0);
  r = pthread_create(&th, NULL, &retrythread, (void *) this);
  assert (r == 0);
	
	
	
	pthread_mutex_init(&server_mutex,NULL);
	pthread_cond_init(&cond_queue, NULL);
	pthread_cond_init(&cond_queue1, NULL);
	
	pthread_mutex_init(&queue_mutex,NULL);
	pthread_mutex_init(&queue_mutex1,NULL);
	pthread_mutex_init(&addr_mutex,NULL);
	
	//lab8
	rsm -> set_state_transfer(this);
	
}

void
lock_server_cache::revoker()
{

  // This method should be a continuous loop, that sends revoke
  // messages to lock holders whenever another client wants the
  // same lock
	 pthread_mutex_lock(&server_mutex);
  while(1){

	  printf(".....START revoker...... \n");
  retry1:
	  while(!revoke_map.empty()){
		  std::map<lock_protocol::lockid_t, server_lock *>::iterator it=revoke_map.begin();
		  lock_protocol::lockid_t lid = (*it).first;
		  if ((*it).second-> owned_client_id == -1) {  
			  revoke_map.erase(lid);
			  goto retry1;
		  }
		  
		  int clt = (*it).second->owned_client_id;
		  int se_num = seq_table[clt][lid];
		  rpcc *scrpc;
		  if (rpcc_map.count(clt) != 0){
			  scrpc = rpcc_map[clt];
		  }else {
			  printf("not find in rpcc map\n");
		  }

//		   if (rsm -> amiprimary()) {
//		  if (rpcc_map.count(clt) != 0){
//			  scrpc = rpcc_map[clt];
//		  }else {
//			  assert(addr_map.count(clt) != 0);
//			  std::string tmpid = addr_map[clt];
//			  int rt;
//			  pthread_mutex_unlock(&server_mutex);
//			 
//				  handleSubscribe(clt,tmpid,rt);
//			 
//			  pthread_mutex_lock(&server_mutex);
//			  scrpc = rpcc_map[clt];
//		  }
//		   }
		  
		  revoke_map.erase(lid);
		  int r;
		  printf("[sending revoke to clt %d for lock %lld]\n",clt, lid);
		
		  pthread_mutex_unlock(&server_mutex);
		  if (rsm -> amiprimary()){
			  printf("sending rpc\n");
			  scrpc->call(rlock_protocol::revoke, clt, se_num, lid, r, rpcc::to(10000));
		  }
		  pthread_mutex_lock(&server_mutex);
		
		  
		  printf("[revoker]: succeed sending call to clt %d for lock %lld\n",clt,lid);
		  goto retry1;
		
	  }
	  //check before sleep
//	  if (!revoke_map.empty()) {
//		  
//	  }
	  printf("......revoker to sleep\n");
	  pthread_cond_wait(&cond_queue, &server_mutex);
	  
  }
	pthread_mutex_unlock(&server_mutex);
}


void
lock_server_cache::retryer()
{

  // This method should be a continuous loop, waiting for locks
  // to be released and then sending retry messages to those who
  // are waiting for it.
	pthread_mutex_lock(&server_mutex);
  while(1){
	  
	  printf(".....START retrier...... \n");
  retry2:
	  while(!retry_map.empty()){
		  std::map<lock_protocol::lockid_t, server_lock *>::iterator it = retry_map.begin();
		  lock_protocol::lockid_t lid = (*it).first;

		  if ((*it).second-> acquiring_client_id.empty()) {
			  retry_map.erase(lid);
			  goto retry2;
		  }
//		  int clt = (*it).second-> acquiring_client_id.front();
//		  (*it).second-> acquiring_client_id.pop_front();
		  
		  int clt = (*it).second-> acquiring_client_id.back();
		  (*it).second-> acquiring_client_id.pop_back();
		  rpcc *scrpc;
		  int se_num = seq_table[clt][lid];
		  
		  if (rpcc_map.count(clt) != 0){
			  scrpc = rpcc_map[clt];
		  }else {
			  printf("not find in rppc map\n");
		  }

		  
//		   if (rsm -> amiprimary()) {
//			   
//		  if (rpcc_map.count(clt) != 0){
//			  scrpc = rpcc_map[clt];
//		  }else {
//			  //not bind to the
//			  assert(addr_map.count(clt) != 0);
//			  std::string tmpid = addr_map[clt];
//			  int rt;
//			  pthread_mutex_unlock(&server_mutex);
//			 
//				  handleSubscribe(clt,tmpid,rt);
//			  
//			  pthread_mutex_lock(&server_mutex);
//			  scrpc = rpcc_map[clt];
//			  
//		  }
//			   
//		  }
		  int r;
		  
		  printf("[sending retry to clt %d for lock %lld]\n",clt, lid );
		  
		  pthread_mutex_unlock(&server_mutex);
		  if (0){
			  printf("sending rpc\n");
			  scrpc->call(rlock_protocol::retry, clt, se_num, lid, r, rpcc::to(10000));
		  }
		  pthread_mutex_lock(&server_mutex);
	
		  printf("[retrier]: successfully sending call to client %d\n",clt);
		  goto retry2;
      //printf("[retrier]: all calls have been  sent \n");

    }
//	  if (!retry_map.empty()) {
//		  
//	  }
	  printf("......retrier to sleep\n");
	  pthread_cond_wait(&cond_queue1, &server_mutex);
	  
  }
	pthread_mutex_unlock(&server_mutex);
}



lock_protocol::status
lock_server_cache::handleAcquire(int clt, std::string id, int seq, lock_protocol::lockid_t lid, int &r)
{
  printf("[handleAcquire]: invoked clt: %d  lock %lld  seq num %d\n ",clt, lid, seq);

  assert(pthread_mutex_lock(&server_mutex) == 0);
	
	
  seq_table[clt][lid] = seq;  //record the seq number 
  if(server_locks.count(lid) == 0){
    server_lock *newl = new server_lock();
    newl->sestat = FREE;
    newl->owned_client_id = -1;
    newl->sequence_num = 0;
    newl->lock_id = lid;
    server_locks[lid] = newl;
  }
	
	
  server_lock *l = server_locks[lid];
	printf("handAcq:lockname is %lld cond is %d\n", lid, l -> sestat);
	
	assert( l -> sestat == FREE || l -> sestat == LOCKED );
	
	/*change in lab 8*/
	for (std::vector<int>::iterator tit = l -> acquiring_client_id.begin(); tit != l -> acquiring_client_id.end(); tit++) {
		if ((*tit) == clt){
			goto cinlab;
		}
	}
	l->acquiring_client_id.push_back(clt);   // !! the client which is acquiring this lock ++
cinlab:
//	l->acquiring_client_id.push_back(clt);   // !! the client which is acquiring this lock ++
//	l -> acquiring_client_id.unique();    //del all duplicate clt
  if(l->sestat == LOCKED){
	  printf(">>>>>>>put lock %lld into revoke queue \n",lid);

    //add to revoke map
	  
//	  server_lock temp;
//	  temp.sestat = l -> sestat;
//	  temp.owned_client_id = l->owned_client_id;
//	  temp.acquiring_client_id = l->acquiring_client_id;
//	  temp.lock_id = l -> lock_id;
//	  

	  revoke_map[lid] = l;  //put it into revoke map
	  pthread_cond_signal(&cond_queue);   //signal revoke queue

	  
    assert(pthread_mutex_unlock(&server_mutex) == 0);
    return lock_protocol::RETRY;
  }
  if(l->sestat == FREE){
    printf("clt %d>>>>>>>>>get the lock %lld\n",clt, lid);
    l->sestat = LOCKED;
    l->owned_client_id = clt;
//	 l->acquiring_client_id.remove(clt); //if a lock get the client , del it from acquiring
  cinlabretry:
	  for (std::vector<int>::iterator tit = l -> acquiring_client_id.begin(); tit != l ->acquiring_client_id.end(); tit++) {
		  if ((*tit) == clt){
			  l -> acquiring_client_id.erase(tit);
			  goto cinlabretry;
		  }
	  }
	  

	assert(pthread_mutex_unlock(&server_mutex) == 0);
    return lock_protocol::OK;
  }
}

lock_protocol::status
lock_server_cache::handleRelease(int clt, std::string id,int seq, lock_protocol::lockid_t lid, int &r)
{
  //  addr_map[clt] = id;

	
  pthread_mutex_lock(&server_mutex);
	 printf("[handleRelease]: invoked  clt: %d  lock %lld\n",clt, lid);
  server_lock *l = server_locks[lid];
	//handle duplicate release
//	if (l -> sestat != LOCKED) {
//		pthread_mutex_unlock(&server_mutex);
//		return lock_protocol::OK;
//	}
//	
//  assert(l->sestat == LOCKED);  
	
  l->sestat = FREE;     // free
  l->owned_client_id = -1;   // no one owns the lock

  printf("[SERVER get the lock]--------------\n");
  //add to retry map
	retry_map[lid] = l; 
//  server_lock temp;
//  temp.sestat = l -> sestat;
//  temp.owned_client_id = l->owned_client_id;
//  temp.acquiring_client_id = l->acquiring_client_id;
//  temp.sequence_num = l->sequence_num;
//  temp.lock_id = l -> lock_id;
	
//	pthread_mutex_lock(&queue_mutex1);  
//	retry_list.push_back(temp);
//	pthread_cond_signal(&cond_queue1);   //wake up retrier
//	pthread_mutex_unlock(&queue_mutex1);
	
  pthread_mutex_unlock(&server_mutex);
  return lock_protocol::OK;
}

lock_protocol::status
lock_server_cache::handleSubscribe(int clt, std::string id, int &r)
{
	pthread_mutex_lock(&addr_mutex);

	if(rpcc_map.count(clt) == 0){  //no addr in this clt
		printf("create new connection\n");
		sockaddr_in dstsock;
		make_sockaddr(id.c_str(), &dstsock);
		rpcc* srpc = new rpcc(dstsock);
		if (srpc->bind() < 0) {
			printf("lock_server_cache: call bind\n");
		}
		rpcc_map[clt] = srpc;
		addr_map[clt] = id;
	}else {
		fprintf(stderr, "what's wrong ?\n");
	}

	pthread_mutex_unlock(&addr_mutex);
	return lock_protocol::OK;
}

lock_protocol::status
lock_server_cache::handleUnsubscribe(int clt, std::string id, int &r)
{
	pthread_mutex_lock(&addr_mutex);
	rpcc* d = rpcc_map[clt];
	rpcc_map.erase(clt);
	delete d;
	pthread_mutex_unlock(&addr_mutex);
	return lock_protocol::OK;
}

//lab 8 
std::string 
lock_server_cache::marshal_state()
{
	assert(pthread_mutex_lock(&server_mutex) == 0);
	printf("-------marshall------\n");
	marshall rep;
	//marshall server_locks
	rep << (unsigned int)server_locks.size();
	for (std::map<lock_protocol::lockid_t, server_lock *>::iterator it = server_locks.begin(); 
		 it != server_locks.end(); it++) {
		lock_protocol::lockid_t lockname = (*it).first;
		server_lock serverlock = *(server_locks[lockname]);
		rep << lockname;
		rep << serverlock;
		printf("Mmarshall:lockname is %lld cond is %d\n", lockname, serverlock.sestat);
		printf("Mmarshall:acquring size %d\n",serverlock.acquiring_client_id.size());
	}
	//marshall revoke map
//	rep << (unsigned int)revoke_map.size();
//	for (std::map<lock_protocol::lockid_t, server_lock *>::iterator it = revoke_map.begin(); 
//		 it != revoke_map.end(); it++) {
//		lock_protocol::lockid_t lockname = (*it).first;
//		server_lock serverlock = *(revoke_map[lockname]);
//		rep << lockname;
//		rep << serverlock;
//	}
	//marshall retry map
//	rep <<(unsigned int)retry_map.size();
//	for (std::map<lock_protocol::lockid_t, server_lock *>::iterator it = retry_map.begin(); 
//		 it != retry_map.end(); it++) {
//		lock_protocol::lockid_t lockname = (*it).first;
//		server_lock serverlock = *(retry_map[lockname]);
//		rep << lockname;
//		rep << serverlock;
//	}
	//marshall addr map
//	rep << (unsigned int)addr_map.size();
//	for (std::map<int, std::string>::iterator it = addr_map.begin(); it != addr_map.end(); it++) {
//		int clientnum = (*it).first;
//		std::string clientid = addr_map[clientnum];
//		rep << clientnum;
//		rep << clientid;
//	}
	
	
	assert(pthread_mutex_unlock(&server_mutex) == 0);
	return rep.str();
	
	
}

void 
lock_server_cache::unmarshal_state(std::string state)
{
	
	assert(pthread_mutex_lock(&server_mutex) == 0);
	printf("-------unmarshall------\n");
	unmarshall rep(state);
	unsigned int locksize;
	//unmarshall server locks
	rep >> locksize;
	for (unsigned int ii = 0; ii != locksize; ii++) {
		lock_protocol::lockid_t lockname;
		rep >> lockname;
		server_lock serverlock;
		rep >> serverlock;
		
		server_lock *newl = new server_lock();   //important!!!!
		newl->sestat = serverlock.sestat;
		newl->owned_client_id = serverlock.owned_client_id;
//		newl->acquiring_client_id = serverlock.acquiring_client_id;  //!!!!!u always miss sth
		newl->sequence_num = serverlock.sequence_num;
		newl->lock_id = serverlock.lock_id;
		server_locks[lockname] = newl;
		
		printf("Uunmarshall:lockname is %lld cond is %d\n", lockname, serverlock.sestat);
		printf("Uunmarshall:acquring size %d\n",serverlock.acquiring_client_id.size());
	}
	//unmarshall revoke map
//	unsigned int revokesize;
//	rep >> locksize;
//	for (unsigned int ii = 0; ii != revokesize; ii++) {
//		lock_protocol::lockid_t lockname;
//		rep >> lockname;
//		server_lock serverlock;
//		rep >> serverlock;
//		
//		server_lock *newl = new server_lock();   //important!!!!
//		newl->sestat = serverlock.sestat;
//		newl->owned_client_id = serverlock.owned_client_id;
//		newl->sequence_num = serverlock.sequence_num;
//		newl->lock_id = serverlock.lock_id;
//		revoke_map[lockname] =newl;
//	}
	//unmarshall retry map
//	unsigned int retrysize;
//	rep >> locksize;
//	for (unsigned int ii = 0; ii != retrysize; ii++) {
//		lock_protocol::lockid_t lockname;
//		rep >> lockname;
//		server_lock serverlock;
//		rep >> serverlock;
//		
//		server_lock *newl = new server_lock();   //important!!!!
//		newl->sestat = serverlock.sestat;
//		newl->owned_client_id = serverlock.owned_client_id;
//		newl->sequence_num = serverlock.sequence_num;
//		newl->lock_id = serverlock.lock_id;
//		retry_map[lockname] = newl;
//	}
	//unmarshall addr map
//	unsigned int addrsize;
//	rep >> addrsize;
//	for (unsigned int ii = 0; ii != addrsize; ii++) {
//		int clientnum;
//		std::string clientid;
//		rep >> clientnum;
//		rep >> clientid;
//		
//		addr_map[clientnum] = clientid;
//		
//		printf("Uunmarshall: clientnum: %d clientid: %s\n",clientnum, clientid.c_str());
//	}
	assert(pthread_mutex_unlock(&server_mutex) == 0);
}
