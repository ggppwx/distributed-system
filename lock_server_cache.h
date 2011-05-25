#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"

#include "rsm.h"

class lock_server_cache: public rsm_state_transfer {

 private:
  class rsm *rsm;


 public:
  lock_server_cache(class rsm *rsm = 0);
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  void revoker();
  void retryer();
  lock_protocol::status handleAcquire(int, std::string, int, lock_protocol::lockid_t, int&);
  lock_protocol::status handleRelease(int, std::string, int, lock_protocol::lockid_t, int&);
	lock_protocol::status handleSubscribe(int , std::string , int &);
	lock_protocol::status handleUnsubscribe(int , std::string , int &);
	
	//lab8
	std::string marshal_state();
	void unmarshal_state(std::string);
	
	//change in lab 8
	enum server_lock_stat{
		FREE = 0,
		LOCKED
	};
	struct server_lock{
		server_lock_stat sestat;
		int owned_client_id;  //which client owns the lock, if free the value is 0
		//  int acquiring_client_id; // which client is acquiring the lock
		//    std::list<int> acquiring_client_id;
		std::vector<int> acquiring_client_id;
		int sequence_num;
		lock_protocol::lockid_t lock_id;
	};

 private:
//  enum server_lock_stat{
//    FREE,
//    LOCKED
//  };
	
//  struct server_lock{
//    server_lock_stat sestat;
//    int owned_client_id;  //which client owns the lock, if free the value is 0
//    //  int acquiring_client_id; // which client is acquiring the lock
////    std::list<int> acquiring_client_id;
//	  std::vector<int> acquiring_client_id;
//    int sequence_num;
//    lock_protocol::lockid_t lock_id;
//
//  };
	
  std::map<lock_protocol::lockid_t, server_lock *> server_locks;
  std::map<lock_protocol::lockid_t, server_lock *> revoke_map;
  std::map<lock_protocol::lockid_t, server_lock *> retry_map;
//	std::list<server_lock> revoke_list;    //lock needs to be revoked
//	std::list<server_lock> retry_list;     //lock needs to be retried

	
  
  std::map<int, rpcc*> rpcc_map;
	std::map<int, std::string> addr_map;
  //  rpcc* get_rpcc(int);  //get client addr from client id
	pthread_mutex_t addr_mutex;
	pthread_mutex_t server_mutex;
	pthread_mutex_t queue_mutex;
	pthread_cond_t cond_queue;
	pthread_mutex_t queue_mutex1;
	pthread_cond_t cond_queue1;
  
  std::map<int, std::map<lock_protocol::lockid_t, int> >  seq_table;



};

inline unmarshall &
operator >>(unmarshall &u, lock_server_cache::server_lock &a)
{
	int tempse;
	u >> tempse;
	a.sestat = (lock_server_cache::server_lock_stat)tempse;
	u >> a.owned_client_id;
	u >> a.acquiring_client_id;
	u >> a.sequence_num;
	u >> a.lock_id;
	return u;
}

inline marshall &
operator <<(marshall &m, lock_server_cache::server_lock a)
{
	int tempse;
	tempse = (int)a.sestat;
	m << tempse;
	m << a.owned_client_id;
	m << a.acquiring_client_id;
	m << a.sequence_num;
	m << a.lock_id;
	return m;
}

#endif
