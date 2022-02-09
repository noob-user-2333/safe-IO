//
// Created by user on 2022/2/9.
//

#ifndef SAFE_IO_SAFE_IO_H
#define SAFE_IO_SAFE_IO_H
#include <string>
#include <utility>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory>

class unixInodeInfo;

class unixFile{
private:
    std::string filePath;
    int fd;
    int flag;
    long lockType = NO_LOCK;
    std::shared_ptr<unixInodeInfo> pUnixInode;
public:
    constexpr static long NO_LOCK = 0;
    constexpr static long SHARED_LOCK = 1;
    constexpr static long RESERVED_LOCK = 2;
    constexpr static long EXECUTE_LOCK = 3;
    unixFile(std::string filePath,int flag,int authority);
    unixFile(std::string filePath,int flag);
    ~unixFile(){
        close(fd);
    }

    int Read(void *buffer,ssize_t size,off_t offset){
        lseek(fd,offset,SEEK_SET);
        return Read(buffer,size);
    }
    int Read(void *buffer,ssize_t size);
    int Write(void *buffer,ssize_t size,off_t offset){
        lseek(fd,offset,SEEK_SET);
        return Write(buffer,size);
    }
    int Write(void *buffer,ssize_t size);
};


#endif //SAFE_IO_SAFE_IO_H
