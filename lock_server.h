// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

//lab1
#include <pthread.h>

//static pthread_mutex_t count_mutex;
//static pthread_cond_t count_threshold_cv;   //add static!!!
//static int lock_status[10];


class lock_server {

 protected:
  int nacquire;

 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
	
	//lab 1	
	lock_protocol::status handleAcquire(int clt, lock_protocol::lockid_t lid, int &);
	lock_protocol::status handleRelease(int clt, lock_protocol::lockid_t lid, int &);
	
	pthread_mutex_t count_mutex;
    pthread_cond_t count_threshold_cv;   //add static!!!
   // int lock_status[1000];
	std::map<lock_protocol::lockid_t, int > lock_status;
	
private:
	enum mylock_status{
		FREE,   //0
		LOCKED  //1
	};
	
	
	
};


//class lock {
//public:
//	lock();
//	~lock(){};
//	
//private:
//	pthread_mutex_t count_mutex;
//	pthread_mutex_t count_threshold_cv;
//	lock_protocol::lid lock_id;
//};
#endif 







