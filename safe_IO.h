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


class unixFile;

class SafeIOFile{
public:
    SafeIOFile() = default;
    virtual ~SafeIOFile() = default;
    virtual std::string FilePath(){throw std::bad_exception();}
    virtual int Read(void *buffer,ssize_t size,off_t offset){throw std::bad_exception();}
    virtual int Read(void *buffer,ssize_t size) {throw std::bad_exception();}
    virtual int Write(void *buffer,ssize_t size,off_t offset){throw std::bad_exception();}
    virtual int Write(void *buffer,ssize_t size){throw std::bad_exception();}
};

SafeIOFile *make_SafeIOFile(std::string file_path,int flag,int authority = 0666);

#endif //SAFE_IO_SAFE_IO_H
