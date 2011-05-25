#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>

#include "lock_protocol.h"
//#include "lock_client.h"
#include "lock_client_cache.h"

class yfs_client: public lock_release_user{  //change
  extent_client *ec;
	lock_client_cache *clientlock;
 public:
  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, FBIG, EXIST };

  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    unsigned long long inum;
  };
	
//	std::vector<dirent>  dirents;
	
	struct filenode {
		fileinfo info;
		std::string filedata;
	};
	
	struct dirnode {
		dirinfo info;
		std::vector<dirent>  dircontents;
	};

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
	
	
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);
	pthread_mutex_t count_m;
  //lsb2 
	
	
	unsigned long get_fuseid(inum);
	
	inum get_fileinum();
	inum get_dirinum();
	int createfile(std::string, inum, filenode, inum&); //lab2
	int createdir(std::string, inum, inum&);  //lab4
	int lookupfile(std::string, inum, inum &, bool &);	//lab2
	int readdir(inum, std::vector<dirent> &); //lab2
	//lab3 readfile writefile
	int do_truncate(inum , size_t );
	
	
	int readfile(inum, std::string &, size_t &);
	int writefile(inum, std::string, size_t, off_t, size_t & );
	int modifyfile(inum, std::string, size_t, off_t, size_t);
	int setfile(inum, fileinfo);
	int setdir(inum, dirinfo);
	int remove(inum, std::string, inum&); //lab4
	
	//lock
	void acquire(inum inum);
	void release(inum inum);
	
	//lab6
	virtual void dorelease(lock_protocol::lockid_t);
	
};


inline unmarshall &
operator>>(unmarshall &u, yfs_client::dirent &a)
{
	u >> a.name;
	u >> a.inum;

	return u;
}

inline marshall &
operator<<(marshall &m, yfs_client::dirent a)
{
	
	m << a.name;
	m << a.inum;

	return m;
}
#endif 
