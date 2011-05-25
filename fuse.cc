/*
 * receive request from fuse and call methods of yfs_client
 *
 * started life as low-level example in the fuse distribution.  we
 * have to use low-level interface in order to get i-numbers.  the
 * high-level interface only gives us complete paths.
 */

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include "yfs_client.h"

int myid;
yfs_client *yfs;

int id() { 
  return myid;
}

yfs_client::status
getattr(yfs_client::inum inum, struct stat &st)
{
  yfs_client::status ret;

  bzero(&st, sizeof(st));

  st.st_ino = inum;
 // printf("getattr %016llx %d\n", inum, yfs->isfile(inum));
  if(yfs->isfile(inum)){
     yfs_client::fileinfo info;
     ret = yfs->getfile(inum, info);
     if(ret != yfs_client::OK)
       return ret;
     st.st_mode = S_IFREG | 0666;
     st.st_nlink = 1;
     st.st_atime = info.atime;
     st.st_mtime = info.mtime;
     st.st_ctime = info.ctime;
     st.st_size = info.size;
   //  printf("   getattr -> %llu\n", info.size);
   } else {
     yfs_client::dirinfo info;
     ret = yfs->getdir(inum, info);
     if(ret != yfs_client::OK)
       return ret;
     st.st_mode = S_IFDIR | 0777;
     st.st_nlink = 2;
     st.st_atime = info.atime;
     st.st_mtime = info.mtime;
     st.st_ctime = info.ctime;
    // printf("   getattr -> %lu %lu %lu\n", info.atime, info.mtime, info.ctime);
   }
   return yfs_client::OK;
}

//lab3 
yfs_client::status
setattr(yfs_client::inum inum, struct stat *attr, struct stat &st)
{
	printf("setattr %llx %d\n", inum, yfs->isfile(inum));
	yfs_client::status ret;
	bzero(&st, sizeof(st));
	st.st_ino=inum;
	if (yfs->isfile(inum)) {
		yfs_client::fileinfo info;		
		info.size=attr->st_size;
		info.atime=attr->st_atime;
		info.mtime=attr->st_mtime;
		info.ctime=attr->st_ctime;
		ret = yfs->setfile(inum, info);
		if (ret != yfs_client::OK) {
			return ret;
		}
		
		st.st_mode = S_IFREG | 0666;
		st.st_nlink = 1;
		st.st_atime = info.atime;
		st.st_mtime = info.mtime;
		st.st_ctime = info.ctime;
		st.st_size = info.size;
		//printf("   setattr -> %llu\n", info.size);
		
	}else {
		yfs_client::dirinfo info;
		info.atime=attr->st_atime;
		info.mtime=attr->st_mtime;
		info.ctime=attr->st_ctime;
		
		ret = yfs->setdir(inum, info);
		if(ret != yfs_client::OK)
			return ret;
		st.st_mode = S_IFDIR | 0777;
		st.st_nlink = 2;
		st.st_atime = info.atime;
		st.st_mtime = info.mtime;
		st.st_ctime = info.ctime;
		//printf("   setattr -> %lu %lu %lu\n", info.atime, info.mtime, info.ctime);
		
	}
	return yfs_client::OK;

}


void
fuseserver_getattr(fuse_req_t req, fuse_ino_t ino,
          struct fuse_file_info *fi)
{
    struct stat st;
    yfs_client::inum inum = ino; // req->in.h.nodeid;
    yfs_client::status ret;
	
//	yfs->acquire(inum);//lock
    ret = getattr(inum, st);
    if(ret != yfs_client::OK){
		yfs -> release(inum);
      fuse_reply_err(req, ENOENT);
		
      return;
    }
//	yfs->release(inum);  //lock
	
    fuse_reply_attr(req, &st, 0);
}

void
fuseserver_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi)
{
  printf("fuseserver_setattr 0x%x\n", to_set);
  if (FUSE_SET_ATTR_SIZE & to_set) {
    printf("   fuseserver_setattr set size to %llu\n", attr->st_size);
    struct stat st;
    // You fill this in
	  printf("*************SETATTR inum: %lld\n",ino);
    	  yfs_client::inum inum = ino;
	  yfs_client::status ret;
	  
	  yfs->acquire(inum);  //lock
	  
	  ret=getattr(inum, st);
	  if (ret != yfs_client::OK) {
		  yfs -> release(inum);
		  fuse_reply_err(req, ENOENT);
		  return;
	  }
	  st.st_size = attr -> st_size;	  
	  ret=setattr(inum, &st, st);
	  if (ret != yfs_client::OK) {
		  yfs -> release(inum);
		  fuse_reply_err(req, ENOENT);
		  return;
	  }
	
	  //do truncate  !!!!
	  ret = yfs->do_truncate(inum, st.st_size);
	  if (ret != yfs_client::OK) {
		  yfs -> release(inum);
		  fuse_reply_err(req, ENOENT);
		  return;
	  }
	  
	  yfs->release(inum);  //lock
	  
#if 1   
    fuse_reply_attr(req, &st, 0);
#else
    fuse_reply_err(req, ENOSYS);
#endif
  } else {
    fuse_reply_err(req, ENOSYS);
  }
}

void
fuseserver_read(fuse_req_t req, fuse_ino_t ino, size_t size,
      off_t off, struct fuse_file_info *fi)
{
  // You fill this in
	yfs_client::inum inum = ino;
	std::string buf1;
	
	yfs->acquire(inum);  //acquire
	
	if (yfs->readfile(inum, buf1, size ) != yfs_client::OK){
		yfs -> release(inum);
		fuse_reply_err(req, ENOENT);
		return;
	}
	std::string buftemp=buf1.substr(off, size);
	const char* buf=buftemp.c_str();
	
	yfs->release(inum);  //release
#if 1
  fuse_reply_buf(req, buf, size);
#else
  fuse_reply_err(req, ENOSYS);
#endif
}

void
fuseserver_write(fuse_req_t req, fuse_ino_t ino,
  const char *buf, size_t size, off_t off,
  struct fuse_file_info *fi)
{
  // You fill this in
	printf("entering WRITE\n");
	std::string test(buf);
	printf("------buf size is %d\n",test.size());	
	yfs_client::inum inum = ino;
	size_t bytes_written=0;
	struct stat attr;
	yfs_client::status ret;
	struct stat st;
	
	yfs->acquire(inum);//acquire the lock
	
	ret=getattr(inum, attr);
	// if size > buf truncate, if size< truncate
	std::string buf1(buf,size);
	size_t newsize;
	if (yfs->writefile(inum, buf1, size, off, newsize ) != yfs_client::OK){
		yfs -> release(inum);
		fuse_reply_err(req, ENOENT);
		return;
	}
	bytes_written=size;
	attr.st_size=newsize;
	
	//set new time  mtime ctime
	attr.st_mtime=time(NULL);
	attr.st_ctime=attr.st_mtime;
	
	ret=setattr(inum, &attr, st);
	if (ret!=yfs_client::OK) {
		yfs -> release(inum);
		fuse_reply_err(req, ENOENT);
		return;
	}
	
	yfs->release(inum);//release the lock
	
#if 1
  fuse_reply_write(req, bytes_written);
#else
  fuse_reply_err(req, ENOSYS);
#endif
}

//................lab2.....................
yfs_client::status
fuseserver_createhelper(fuse_ino_t parent, const char *name,
     mode_t mode, struct fuse_entry_param *e)
{
  // You fill this in
	if(!yfs->isdir(parent)){
		return yfs_client::NOENT;
	}
	
	
	yfs_client::filenode fn;
	yfs_client::inum inum;
	memset(e,0,sizeof(struct fuse_entry_param));
	yfs->acquire(parent); //acquire the lock of dir
	
	inum=yfs->get_fileinum();  //get file inum
	
	yfs->acquire(inum);//acquire a lock of file
	
	if (yfs->createfile(name, parent, fn, inum)==yfs_client::OK){
		printf("inum of the file is %llx\n",inum);
		//set attr of file 
		yfs_client::status ret;
		struct stat attr; 
		struct stat st;
		ret=getattr(inum, attr);
		attr.st_ctime=time(NULL);
		attr.st_mtime=attr.st_ctime;
		ret=setattr(inum, &attr, st);
		if (ret!=yfs_client::OK) {
			yfs -> release(parent);
			yfs -> release(inum);
			return yfs_client::NOENT;
		}
		
		
		
		//set attr of dir
		struct stat dirattr; 
		
		
		ret=getattr(parent, dirattr);
		dirattr.st_ctime=attr.st_ctime;
		dirattr.st_mtime=attr.st_mtime;
		ret=setattr(parent, &dirattr, st);
		if (ret!=yfs_client::OK) {
			yfs -> release(parent);
			yfs -> release(inum);
			return yfs_client::NOENT;
		}
		
		
		
		e->ino=(unsigned long)inum;
		e->attr_timeout = 0;
		e->entry_timeout = 0;
		e->generation=1;
		ret=getattr(inum,e->attr);
		if (ret!=yfs_client::OK) {
			yfs -> release(parent);
			yfs -> release(inum);
			return yfs_client::NOENT;
		}
		
		yfs->release(inum);
		
		yfs->release(parent);  //release parent
		
		return yfs_client::OK;
	}
	
	
	
  return yfs_client::NOENT;
}

void
fuseserver_create(fuse_req_t req, fuse_ino_t parent, const char *name,
   mode_t mode, struct fuse_file_info *fi)
{
  struct fuse_entry_param e;
  if( fuseserver_createhelper( parent, name, mode, &e ) == yfs_client::OK ) {
	  fuse_reply_create(req, &e, fi);

	  		
  } else {
    fuse_reply_err(req, ENOENT);
  }
}

void fuseserver_mknod( fuse_req_t req, fuse_ino_t parent, 
    const char *name, mode_t mode, dev_t rdev ) {
  struct fuse_entry_param e;
  if( fuseserver_createhelper( parent, name, mode, &e ) == yfs_client::OK ) {
    fuse_reply_entry(req, &e);
  } else {
    fuse_reply_err(req, ENOENT);
  }
}

//..................lab2  LOOKUP......................
void
fuseserver_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
  struct fuse_entry_param e;
  bool found = false;
  e.attr_timeout = 0.0;
  e.entry_timeout = 0.0;

  // You fill this in:
  // Look up the file named `name' in the directory referred to by
  // `parent' in YFS. If the file was found, initialize e.ino and
  // e.attr appropriately.
	memset(&e, 0, sizeof(struct fuse_entry_param));
	yfs_client::inum inum;
	
	if (yfs->isfile(parent)) {
		fuse_reply_err(req, ENOENT);
		return;
	}
	
	yfs->acquire(parent);  //require a lock to parent
	//so we can not change the content of the parent
	
    if(yfs->lookupfile(name, parent, inum, found)!=yfs_client::OK){
		goto release;
	}
	
//	if (found) {
//		printf(".....!!!..look up found....!!!..\n");
//
//	}
	if (!found) {
		goto release;
	}
	
	e.ino=(unsigned long)inum;
	e.generation=1;
	e.entry_timeout=0;
	e.attr_timeout = 0;
	
	yfs_client::status ret;
	
	yfs->acquire(inum);  //inner lock
	ret=getattr(inum,e.attr);
	if (ret!=yfs_client::OK) {
		yfs -> release(inum);
		yfs -> release(parent);
		fuse_reply_err(req, ENOENT);
		return;
	}
	
	yfs->release(inum);
release:
	
	
	yfs->release(parent);   //release a lock to parent
	
	if (found){
	  fuse_reply_entry(req, &e);
	  
	}
	
  else
	  
    fuse_reply_err(req, ENOENT);
}


struct dirbuf {
    char *p;
    size_t size;
};

void dirbuf_add(struct dirbuf *b, const char *name, fuse_ino_t ino)
{
    struct stat stbuf;
    size_t oldsize = b->size;
    b->size += fuse_dirent_size(strlen(name));
    b->p = (char *) realloc(b->p, b->size);
    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;
    fuse_add_dirent(b->p + oldsize, name, &stbuf, b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
          off_t off, size_t maxsize)
{
  if (off < bufsize)
    return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
  else
    return fuse_reply_buf(req, NULL, 0);
}

//..................lab2.........................
void
fuseserver_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
          off_t off, struct fuse_file_info *fi)
{
  yfs_client::inum inum = ino; // req->in.h.nodeid;
  struct dirbuf b;
  yfs_client::dirent e;
	std::vector<yfs_client::dirent> dents; 
  printf("fuseserver_readdir\n");

  if(!yfs->isdir(inum)){
    fuse_reply_err(req, ENOTDIR);
    return;
  }

  memset(&b, 0, sizeof(b));

  // fill in the b data structure using dirbuf_add
	yfs->acquire(inum);  //acquire lock
	
	yfs->readdir(inum,dents);
	
	for (std::vector<yfs_client::dirent>::iterator it=dents.begin(); it!=dents.end(); it++) {
		printf("the inum is : %lld\n",(*it).inum);
		printf("the name is : %s\n",(*it).name.c_str());
		dirbuf_add(&b, (*it).name.c_str()  ,(unsigned long)(*it).inum);
	}
	
	yfs->release(inum);  //release lock

  reply_buf_limited(req, b.p, b.size, off, size);//req, buf,bufsize, off, size
  free(b.p);
}

//......................lab2................
void
fuseserver_open(fuse_req_t req, fuse_ino_t ino,
     struct fuse_file_info *fi)
{
  // You fill this in
	printf("entering open\n");
	//change atime
	yfs_client::inum inum=ino;
	yfs_client::status ret;
	struct stat attr; 
	
	yfs->acquire(inum);  //acquire lock
	ret=getattr(inum, attr);
	attr.st_atime=time(NULL);
	if (ret!=yfs_client::OK) {
		yfs -> release(inum);
		fuse_reply_err(req, ENOENT);
		return;
	}
	yfs->release(inum);  //release lock
	
#if 1
  fuse_reply_open(req, fi);
#else
  fuse_reply_err(req, ENOSYS);
#endif
}

void
fuseserver_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
     mode_t mode)
{
  struct fuse_entry_param e;

  // You fill this in
	if(!yfs->isdir(parent)){
		printf("not dir\n");
		fuse_reply_err(req, ENOENT);
		return;
	}
	
	yfs->acquire(parent);  //lock the parent
	
	//we need parent, name, inum 
	yfs_client::inum inum;
	inum=yfs->get_dirinum();//get dir inum
	
		
	yfs->acquire(inum);  //acquire the lock
	
	if (yfs->createdir(name, parent, inum)!=yfs_client::OK) {
		yfs -> release(inum);
		yfs -> release(parent);
		fuse_reply_err(req, ENOENT);
		return;
	}
	//set attr  of own
	yfs_client::status ret;
	struct stat attr; 
	struct stat st; 
	ret=getattr(inum, attr);
	attr.st_ctime=time(NULL);
	attr.st_mtime=attr.st_ctime;
	ret=setattr(inum, &attr, st);
	if (ret!=yfs_client::OK) {
		yfs -> release(inum);
		yfs -> release(parent);
		fuse_reply_err(req, ENOENT);
		return;
	}
	//set attr of parent
	struct stat dirattr; 
	ret=getattr(parent, dirattr);
	dirattr.st_ctime=attr.st_ctime;
	dirattr.st_mtime=attr.st_mtime;
	ret=setattr(parent, &dirattr, st);
	if (ret!=yfs_client::OK) {
		yfs -> release(inum);
		yfs -> release(parent);
		fuse_reply_err(req, ENOENT);
		return;
	}	
	//get attr and pass to e
	e.ino=(unsigned long)inum;
	e.attr_timeout = 0;
	e.entry_timeout = 0;
	e.generation=1;
	ret=getattr(inum,e.attr);
	if (ret!=yfs_client::OK) {
		yfs -> release(inum);
		yfs -> release(parent);
		fuse_reply_err(req, ENOENT);
		return;
	}
	
	
	yfs->release(inum);//release the lock 
	
	yfs->release(parent);  //release parent
	
	// return  entry
#if 1
  fuse_reply_entry(req, &e);
#else
  fuse_reply_err(req, ENOSYS);
#endif
}

void
fuseserver_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{

  // You fill this in
  // Success:	fuse_reply_err(req, 0);
  // Not found:	fuse_reply_err(req, ENOENT);
	
	yfs->acquire(parent);//acquire a lock to parent
	
	yfs_client::inum inum;
	if (yfs->remove(parent, name, inum)!=yfs_client::OK) {
		yfs -> release(parent);
		fuse_reply_err(req, ENOENT);
		return;
	}
	//change the attr of parent
	printf(">>>>>>delete mark<<<<<<<");
	struct stat dirattr; 
	struct stat st;
	yfs_client::status ret;
	
	ret=getattr(parent, dirattr);
	dirattr.st_ctime=time(NULL);
	dirattr.st_mtime=dirattr.st_ctime;
	ret=setattr(parent, &dirattr, st);
	if (ret!=yfs_client::OK) {
		yfs -> release(parent);
		fuse_reply_err(req, ENOENT);
		return;
	}	
	
	yfs->release(parent); //release a lock
	
  fuse_reply_err(req, 0);
}

void
fuseserver_statfs(fuse_req_t req)
{
  struct statvfs buf;

  printf("statfs\n");

  memset(&buf, 0, sizeof(buf));

  buf.f_namemax = 255;
  buf.f_bsize = 512;

  fuse_reply_statfs(req, &buf);
}

struct fuse_lowlevel_ops fuseserver_oper;

int
main(int argc, char *argv[])
{
  char *mountpoint = 0;
  int err = -1;
  int fd;

  setvbuf(stdout, NULL, _IONBF, 0);

  if(argc != 4){
    fprintf(stderr, "Usage: yfs_client <mountpoint> <port-extent-server> <port-lock-server>\n");
    exit(1);
  }
  mountpoint = argv[1];

  srandom(getpid());

  myid = random();

  yfs = new yfs_client(argv[2], argv[3]);

  fuseserver_oper.getattr    = fuseserver_getattr;
  fuseserver_oper.statfs     = fuseserver_statfs;
  fuseserver_oper.readdir    = fuseserver_readdir;  //lab2 
  fuseserver_oper.lookup     = fuseserver_lookup;   //lab2
  fuseserver_oper.create     = fuseserver_create;   //lab2
  fuseserver_oper.mknod      = fuseserver_mknod;    //lab2
  fuseserver_oper.open       = fuseserver_open;     
  fuseserver_oper.read       = fuseserver_read;
  fuseserver_oper.write      = fuseserver_write;   
  fuseserver_oper.setattr    = fuseserver_setattr;
  fuseserver_oper.unlink     = fuseserver_unlink;  //lab4 remove
  fuseserver_oper.mkdir      = fuseserver_mkdir;  //lab4

  const char *fuse_argv[20];
  int fuse_argc = 0;
  fuse_argv[fuse_argc++] = argv[0];
#ifdef __APPLE__
  fuse_argv[fuse_argc++] = "-o";
  fuse_argv[fuse_argc++] = "nolocalcaches"; // no dir entry caching
  fuse_argv[fuse_argc++] = "-o";
  fuse_argv[fuse_argc++] = "daemon_timeout=86400";
#endif

  // everyone can play, why not?
  //fuse_argv[fuse_argc++] = "-o";
  //fuse_argv[fuse_argc++] = "allow_other";

  fuse_argv[fuse_argc++] = mountpoint;
  fuse_argv[fuse_argc++] = "-d";

  fuse_args args = FUSE_ARGS_INIT( fuse_argc, (char **) fuse_argv );
  int foreground;
  int res = fuse_parse_cmdline( &args, &mountpoint, 0 /*multithreaded*/, 
        &foreground );
  if( res == -1 ) {
    fprintf(stderr, "fuse_parse_cmdline failed\n");
    return 0;
  }
  
  args.allocated = 0;

  fd = fuse_mount(mountpoint, &args);
  if(fd == -1){
    fprintf(stderr, "fuse_mount failed\n");
    exit(1);
  }

  struct fuse_session *se;

  se = fuse_lowlevel_new(&args, &fuseserver_oper, sizeof(fuseserver_oper),
       NULL);
  if(se == 0){
    fprintf(stderr, "fuse_lowlevel_new failed\n");
    exit(1);
  }

  struct fuse_chan *ch = fuse_kern_chan_new(fd);
  if (ch == NULL) {
    fprintf(stderr, "fuse_kern_chan_new failed\n");
    exit(1);
  }

  fuse_session_add_chan(se, ch);
  // err = fuse_session_loop_mt(se);   // FK: wheelfs does this; why?
  err = fuse_session_loop(se);
    
  fuse_session_destroy(se);
  close(fd);
  fuse_unmount(mountpoint);

  return err ? 1 : 0;
}
