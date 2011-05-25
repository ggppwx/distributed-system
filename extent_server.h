// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>   //it is a hit
#include "extent_protocol.h"

class extent_server {

 public:
  extent_server();
	std::map<extent_protocol::extentid_t, std::string> data;
	std::map<extent_protocol::extentid_t, std::string> meta_data;
	
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
	
	//lab3 
	int setattr(extent_protocol::extentid_t id, extent_protocol::attr, int &);
	
private:
	pthread_mutex_t extent_mutex;

};

#endif 







