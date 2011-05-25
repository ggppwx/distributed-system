// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() {
	pthread_mutex_init(&extent_mutex, NULL);
}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  //lab2 id is 64 bit long 
	pthread_mutex_lock(&extent_mutex);
	data[id]=buf;
	pthread_mutex_unlock(&extent_mutex);
	
	printf("put id is : %lld\n",id);
	printf("buf size is %d\n",buf.size());
	return extent_protocol::OK;
	
//   return extent_protocol::IOERR;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  //lab2
	pthread_mutex_lock(&extent_mutex);
	buf=data[id];
	pthread_mutex_unlock(&extent_mutex);
	
	printf("get id is : %lld\n",id);
	printf("buf size is %d\n",buf.size());
	return extent_protocol::OK;
//	return extent_protocol::IOERR;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  // You replace this with a real implementation. We send a phony response
  // for now because it's difficult to get FUSE to do anything (including
  // unmount) if getattr fails.
	//.......this is a good example for how to use marshall and unmarshall
	pthread_mutex_lock(&extent_mutex);
	std::string temp=meta_data[id];
	pthread_mutex_unlock(&extent_mutex);
	
	unmarshall at(temp);
	extent_protocol::attr attr_temp; 
	at>>attr_temp;
	
	a.size=attr_temp.size;
	a.atime=attr_temp.atime;
	a.mtime=attr_temp.mtime;
	a.ctime=attr_temp.ctime;
	

  return extent_protocol::OK;
}

int extent_server::setattr(extent_protocol::extentid_t id, extent_protocol::attr a, int &)
{
	marshall at;
	at<<a;
	std::string temp=at.get_content();
	
	pthread_mutex_lock(&extent_mutex);
	meta_data[id]=temp;
	pthread_mutex_unlock(&extent_mutex);
	
	printf("setattr id is : %lld\n",id);
	printf("setattr size is %d\n",temp.size());
	return extent_protocol::OK;

}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  //lab2
	pthread_mutex_lock(&extent_mutex);
	data.erase(id);
	meta_data.erase(id);
	pthread_mutex_unlock(&extent_mutex);
	
  return extent_protocol::OK;
}

//lab3

