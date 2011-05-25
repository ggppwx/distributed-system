// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
	pthread_mutex_init(&mutex, NULL);
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;
	// new check if the file in the cache
	pthread_mutex_lock(&mutex);
	if (extent_cache.count(eid) != 0) {
		//eid in the cache, get from the cache
		buf = extent_cache[eid];
		printf("[get eid from cache is %lld\n",eid);
	}else {
		//not in the cache, get from the server and cache in local
		ret = ret = cl->call(extent_protocol::get, eid, buf);
		extent_cache[eid] = buf;
		//make the file "clean"
		dirty[eid] = false;
		printf("[get eid from server is %lld\n",eid);
	}
	pthread_mutex_unlock(&mutex);
	
//ret = cl->call(extent_protocol::get, eid, buf);
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&mutex);
	if (extent_attr_cache.count(eid) != 0) {
		//attr in the cache, get from the cache
		attr = extent_attr_cache[eid];
		printf("[getattr eid from cache is %lld\n",eid);
	}else {
		//not in the cache, get from the server
		ret = cl->call(extent_protocol::getattr, eid, attr);
		extent_attr_cache[eid] = attr;
		dirty[eid] = false;
		printf("[getattr eid from server is %lld\n",eid);
	}
	pthread_mutex_unlock(&mutex);
	
//	ret = cl->call(extent_protocol::getattr, eid, attr);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
	//put should NOT send it to server
  extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&mutex);
	if (extent_cache.count(eid) != 0) {
		//in the cache, change the cache
		extent_cache[eid] = buf;
//		int r;
//		ret = cl->call(extent_protocol::put, eid, buf, r);
		//make the file dirty
		dirty[eid] = true;
		printf("[put eid to cache is %lld\n",eid);
		
	}else {
		//not in the cache, add cache and send rpc call to server
		extent_cache[eid] = buf;
//		int r;
//		ret = cl->call(extent_protocol::put, eid, buf, r);
		dirty[eid] = true;
		printf("[put eid to server is %lld\n",eid);
	}
	pthread_mutex_unlock(&mutex);
	
	
//  int r;
//  ret = cl->call(extent_protocol::put, eid, buf, r);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
	//only remove the content in local cache
  extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&mutex);
	if (extent_cache.count(eid) != 0) {
		//eid in the cache, del both content and attr
		extent_cache.erase(eid);
		extent_attr_cache.erase(eid);
//		int r;
//		ret = cl->call(extent_protocol::remove, eid, r);
		dirty[eid] = true;
		printf("[delete eid in cache and server is %lld\n",eid);
	}else {
		//not in the cache, just send del call to server
		
		int r;
		ret = cl->call(extent_protocol::remove, eid, r);
		printf("[delete eid in server is %lld\n",eid);
	}
	pthread_mutex_unlock(&mutex);
	
//  int r;
//  ret = cl->call(extent_protocol::remove, eid, r);
  return ret;
}

//lab3 
extent_protocol::status 
extent_client::setattr(extent_protocol::extentid_t eid, extent_protocol::attr attr)
{
	//should update(make it dirty) it once the file is accessed 
	//same as put
	extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&mutex);
	if (extent_attr_cache.count(eid) != 0) {
		//attr in the cache
		extent_attr_cache[eid] = attr;
//		int r;
//		ret=cl->call(extent_protocol::setattr, eid, attr, r);
		dirty[eid] = true;
		printf("[setattr eid in cache is %lld\n",eid);
	}else {
		//not in the cache, do the same
		extent_attr_cache[eid] = attr;
//		int r;
//		ret=cl->call(extent_protocol::setattr, eid, attr, r);
		dirty[eid] = true;
		printf("[setattr eid in server is %lld\n",eid);
	}
	pthread_mutex_unlock(&mutex);
	
//	int r;
//	ret=cl->call(extent_protocol::setattr, eid, attr, r);
	return ret;
}


extent_protocol::status
extent_client::flush(extent_protocol::extentid_t eid)
{
	//once the file is drity, the attr is also dirty
	//first check if the extent is dirty 
	extent_protocol::status ret = extent_protocol::OK;
	std::string buf;
	extent_protocol::attr attr;
	pthread_mutex_lock(&mutex);
	if (false == dirty[eid]) {
		//not dirty, no need to send to server
		goto release;
	}
	if (extent_cache.count(eid) == 0) {
		//already removed, send remove rpc to extent server
		assert(extent_attr_cache.count(eid) == 0); //the attr must be removed 
		int r;
		ret = cl->call(extent_protocol::remove, eid, r);
		//directly to release;
		goto release;
	}

	// the content is dirty and exists, send to extent server
	buf = extent_cache[eid];
	attr = extent_attr_cache[eid];
	int r;
	ret = cl -> call(extent_protocol::put, eid, buf, r);
	ret = cl -> call(extent_protocol::setattr, eid, attr, r);
	printf("successfully flush %lld to server\n",eid);
	
release:
	//then remove the dirty, extent_cache, extent_attr_cache
	extent_cache.erase(eid);
	extent_attr_cache.erase(eid);
	dirty.erase(eid);
	pthread_mutex_unlock(&mutex);
	
	return ret;
}