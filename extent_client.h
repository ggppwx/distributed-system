// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "rpc.h"   //use marshall and unmarshall

class extent_client {
 private:
  rpcc *cl;
	std::map<extent_protocol::extentid_t, std::string> extent_cache;
	std::map<extent_protocol::extentid_t, extent_protocol::attr> extent_attr_cache;
	std::map<extent_protocol::extentid_t, bool> dirty;
	
	pthread_mutex_t mutex;
	
 public:
  extent_client(std::string dst);

  extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
	
	//lab3 
	extent_protocol::status setattr(extent_protocol::extentid_t eid, 
									extent_protocol::attr a);
	//lab6
	extent_protocol::status flush(extent_protocol::extentid_t);
	
	
};

#endif 

