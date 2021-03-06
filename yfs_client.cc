// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
//#include "lock_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <algorithm>
#include "lock_client_cache.h"
yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
	srand ( (unsigned)time(NULL) );
	clientlock = new lock_client_cache(lock_dst, this); //!!!also init lock_releaser_user
	assert(0 == pthread_mutex_init(&count_m, NULL));
}

void yfs_client::acquire(inum inum)
{	
//	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>acquire the lock: %lld\n",inum);
	lock_protocol::lockid_t a = inum;
	clientlock->acquire(a);
	//printf("---------------------------------acquire the lock: %lld\n",inum);
}
void yfs_client::release(inum inum)
{
//	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>acquire the lock: %lld\n",inum);
	lock_protocol::lockid_t a = inum;
	clientlock->release(a);
	//printf("---------------------------------release the lock: %lld\n", inum);
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;


  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

  return r;
}

//lab3 set file
int yfs_client::setfile(inum inum, fileinfo fin)
{
	int r=OK;
	printf("setfile %016llx\n", inum);
	extent_protocol::attr a;
	a.atime=fin.atime;
	a.mtime=fin.mtime;
	a.ctime=fin.ctime;
	a.size=fin.size;
 //setattr in server is not implemented
	if (ec->setattr(inum, a) != extent_protocol::OK) {
		r=IOERR;
		goto release;
	}
	
release:
	return r;
}


int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;


  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

//lab3 set dir
int yfs_client::setdir(inum inum, dirinfo din)
{
	int r=OK;
	printf("setfile %016llx\n", inum);
	extent_protocol::attr a;
	a.atime=din.atime;
	a.mtime=din.mtime;
	a.ctime=din.ctime;
	// setattr in server is not implemented
		if (ec->setattr(inum, a) != extent_protocol::OK) {
			r=IOERR;
			goto release;
		}
	
release:
	return r;
}


//............lab2 
//in lab 2, we add createfile,  lookfile,  readdir

int yfs_client::do_truncate(inum inum, size_t size)
{
	int r=OK;
	std::string content;
	if (ec->get(inum, content) != extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	if (size < content.size()) {
		content = content.substr(0, size);
	}
	if (size > content.size()) {
		content.append((size - content.size()),'\0');
	}
	if (ec->put(inum, content) != extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	return r;
}

yfs_client::inum yfs_client::get_fileinum()
{
	//get the file inum
	unsigned long long tinum;
	tinum= rand();
	pid_t pid=getpid();
	tinum = (tinum * (pid+1))%2147483647;
	inum inum;
	inum= tinum | 0x80000000;  //make the inum a file inum 

	return inum;
}
yfs_client::inum yfs_client::get_dirinum()
{
	//get the dir inum
	unsigned  long long tinum;
	tinum= rand();
	pid_t pid=getpid();
	tinum = (tinum * (pid+1))%2147483647;
	inum inum;
	inum= tinum & 0x7fffffff;  //make the inum a file inum   FUCK!!!!!!!!that 

	return inum;	
}

int yfs_client::createfile(std::string name, inum parent, filenode fn, inum &inum)
{
	printf(".....create file %s\n",name.c_str());
	int r=OK;
	
	
	
	std::string buf;
	printf(".....%lld..\n",inum);
	
//	marshall ma;  //put the file data into the marshall

	buf.clear();

	//put the file content in file
	if (ec->put(inum, buf)!=extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	
	//put the dircontent into dir
	std::string dirbuf;
	if (ec->get(parent,dirbuf)!=extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	std::vector<dirent> dirents;
	unmarshall u_dirbuf(dirbuf);
	u_dirbuf>>dirents;
		
	bool found=false;
	for(std::vector<dirent>::iterator it= dirents.begin();it!=dirents.end();it++){
		if ((*it).name.compare(name)==0){ 
			(*it).inum = inum;
			found=true;
			break;
		}
	}
	if(!found){
		dirent dt;
		dt.name=name;
		dt.inum=inum;	
		dirents.push_back(dt);	
	}
	
	marshall md;  //put the struct into marshall, the sequence is the same as the struct
	md<<dirents;
	
	buf=md.get_content();	
	//put the mapping in dir
	if (ec->put(parent,buf)!=extent_protocol::OK){
		r=IOERR;
		return r;
	}
	//printf("...****..%lld..",inum);
	return r;
}

int yfs_client::createdir(std::string name, inum parent, inum &inum)
{
	printf(".....create dir %s\n",name.c_str());
	int r=OK;
	
	//get the dir inum
	
	//put dir content into parent dir, dir content is empty.
	std::string buf;
	buf.clear();
	if (ec->put(inum, buf)!=extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	
	//change the parent dir content
	std::string dirbuf;
	if (ec->get(parent,dirbuf)!=extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	std::vector<dirent> dirents;
	unmarshall u_dirbuf(dirbuf);
	u_dirbuf>>dirents;
	
	bool found=false;
	for(std::vector<dirent>::iterator it= dirents.begin();it!=dirents.end();it++){
		if ((*it).name.compare(name)==0){ 
			(*it).inum = inum;
			found=true;
			break;
		}
	}
	if(!found){
		dirent dt;
		dt.name=name;
		dt.inum=inum;	
		dirents.push_back(dt);	
	}
	
	marshall md;  //put the struct into marshall, the sequence is the same as the struct
	md<<dirents;
	
	buf=md.get_content();	
	//put the mapping in dir
	if (ec->put(parent,buf)!=extent_protocol::OK){
		r=IOERR;
		return r;
	}
	
	return r;
}

int yfs_client::lookupfile(std::string name, inum parent, inum &re_inum, bool &re_found)
{
	//check if the file name exists in the directory 
	int r=OK;	  //!!!!important should initialize
	std::string dirbuf;
	if (ec->get(parent,dirbuf)!=extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	
    printf(">>>>> ......%s\n", name.c_str());  //!!!!!!!!!!!very important
    //printf("....dir buf is %s\n",dirbuf.c_str());  // u MUST use this c_str() OR error will occur
	std::vector<dirent> dirents;
	unmarshall u_dirbuf(dirbuf);
	u_dirbuf>>dirents;
	for(std::vector<dirent>::iterator it= dirents.begin();it!=dirents.end();it++){
		if ((*it).name.compare(name)==0){ 

				re_inum=(*it).inum;
				re_found=true;
				printf("!!!!!!!!!!!!now the found is ture: %lld\n",re_inum);
				break;
			
		}
	}

	return r;
}

//lab3 read file
int yfs_client::readfile(inum inum, std::string &buf, size_t &res)
{
	int r=OK;
	if (ec->get(inum,buf)!=extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	
	return r;
}

//lab3 write file
int yfs_client::writefile(inum inum, std::string buf, size_t size, off_t off, size_t& newsize)
{
	int r=OK;
	printf(">>>>>>>>>entering directly add \n");
	std::string content;
	if (ec->get(inum, content) != extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	if (off >= content.size()) {  //append
		content.append((off - content.size()),'\0');
		content += buf;
		newsize = off + size;
	}
	if (off < content.size()){
		newsize = std::max((size_t)(off + size), (size_t)content.size());
		if (newsize > content.size()) {
			content.append((newsize - content.size()), '\0');
			content.replace(off, size, buf);
		}else {
			content.replace(off, size, buf);
		}

		
	}

	if (ec->put(inum, content) != extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	
	return r;
}


int yfs_client::readdir(inum inum, std::vector<dirent> &dents)
{
	int r=OK;
	printf("..........read dir file %lld\n",inum);
	std::string dirbuf;
	if (ec->get(inum,dirbuf)!=extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	std::vector<dirent> dirents;
	unmarshall u_dirbuf(dirbuf);
	u_dirbuf>>dirents;
	
	dents=dirents;
	
	
	return r;
}

int yfs_client::remove(inum parent, std::string name, inum &delinum)
{
	int r=OK;
	bool found=false;
	printf("..........remove dir/file \n");
	//check if name exists
	std::string dirbuf;
	
	if (ec->get(parent,dirbuf)!=extent_protocol::OK) {
		r=IOERR;
		return r;
	}
	inum inum;
	std::vector<dirent> dirents;
	unmarshall u_dirbuf(dirbuf);
	u_dirbuf>>dirents;
	for(std::vector<dirent>::iterator it= dirents.begin();it!=dirents.end();it++){
		//printf("..name: %s and  inum: %lld\n", (*it).name.c_str(),(*it).inum);   //ok 
		if ((*it).name.compare(name)==0){ 
			//delete it in dir content
			found=true;
			printf("find delete one\n");
			inum=(*it).inum;
			dirents.erase(it);
			break;
		}
	}
	delinum=inum; //pass back the delinum
	
	if(!found){  // if not found
		r=NOENT;
		return r;
	}
	marshall md;  //put the struct into marshall, the sequence is the same as the struct
	md<<dirents;
	std::string buf=md.get_content();	
	//put the mapping in dir
	if (ec->put(parent,buf)!=extent_protocol::OK){
		r=IOERR;
		return r;
	}

	acquire(inum); //dont know if it is necessary
	//delete its data and attr
	if (ec->remove(inum)!=extent_protocol::OK) {
		r=IOERR;
		release(inum);
		return r;
	}
	
	release(inum);
	
	return r;
	
}

//lab6
void yfs_client::dorelease(lock_protocol::lockid_t lid)
{
	//lid = inum
	inum inum = lid;
	ec -> flush(inum);
	printf("[>>>>>>DORELEASE]\n");
}
