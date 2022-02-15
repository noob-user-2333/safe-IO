//
// Created by user on 2022/2/9.
//

#include <list>
#include <utility>
#include <sys/file.h>
#include <iostream>
#include <cassert>
#include "safe_IO.h"


class unixFile :public SafeIOFile{
private:
    std::string filePath;
    int fd;
    int flag;
    long lockType = NO_LOCK;
    unixInodeInfo* pUnixInode;
public:
    constexpr static long NO_LOCK = 0;
    constexpr static long SHARED_LOCK = 1;
    constexpr static long RESERVED_LOCK = 2;
    constexpr static long EXECUTE_LOCK = 3;

    unixFile(std::string filePath,int flag,int authority = 0666);
    ~unixFile();

    std::string FilePath(){return filePath;}
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



struct unixFileId {
    dev_t dev = 0;
    ino_t ino = 0;
};

struct unixInodeInfo {

public:
    unixFileId fileId;
    pthread_mutex_t mutex{};
    long nShared = 0;
    long nRef = 0;
    long lockType = unixFile::NO_LOCK;

    constexpr static long SHARED_BYTE = 0x10000;
    constexpr static long RESERVED_BYTE = 0x10001;
    constexpr static long EXECUTE_BYTE = 0x10002;


    unixInodeInfo(dev_t dev,ino_t ino){
        this->fileId.dev = dev;
        this-> fileId.ino = ino;
        pthread_mutex_init(&mutex,nullptr);
    }
    ~unixInodeInfo();
    static unixInodeInfo *Find(dev_t dev,ino_t ino);
    static void Unregister(unixInodeInfo *unregister_info);

};

struct inodeManage{
    std::list<unixInodeInfo *> inodeList;
    pthread_mutex_t listMutex{};
    inodeManage(){ pthread_mutex_init(&listMutex,nullptr);}
    unixInodeInfo* Find(dev_t dev,ino_t ino){
        pthread_mutex_lock(&listMutex);
        for(auto it = inodeList.begin(); it != inodeList.end(); ++it){
            if((*it)->fileId.dev == dev && (*it)->fileId.ino == ino) {
                pthread_mutex_unlock(& listMutex);
                return *it;
            }
        }
        pthread_mutex_unlock(&listMutex);
        return  nullptr;
    }
    int Erase(dev_t dev,ino_t ino){
        pthread_mutex_lock(&listMutex);
        for(auto it = inodeList.begin(); it != inodeList.end(); ++it){
            if((*it)->fileId.dev == dev && (*it)->fileId.ino == ino) {
                inodeList.erase(it);
                pthread_mutex_unlock(& listMutex);
                return 1;
            }
        }
        pthread_mutex_unlock(&listMutex);
        return  0;
    }
    int Add(unixInodeInfo *info){
        for (auto & it : inodeList) {
            if(it == info)
                return -1;
        }
        inodeList.push_back(info);
        return 0;
    }
}static inodeList;


unixInodeInfo::~unixInodeInfo(){
    pthread_mutex_destroy(&mutex);
    inodeList.Erase(fileId.dev,fileId.ino);
}

unixInodeInfo *unixInodeInfo::Find(dev_t dev,ino_t ino){
    unixInodeInfo *info = inodeList.Find(dev,ino);
    if(info == nullptr){
        info = new unixInodeInfo(dev,ino);
        inodeList.Add(info);
    }
    pthread_mutex_lock(&info->mutex);
    info->nRef++;
    pthread_mutex_unlock(&info -> mutex);
    return info;
}

void unixInodeInfo::Unregister(unixInodeInfo *unregister_info){
    pthread_mutex_lock(&unregister_info->mutex);
    unregister_info ->nRef--;
    pthread_mutex_unlock(&unregister_info -> mutex);
    if(unregister_info -> nRef == 0)
        delete unregister_info;
}
unixFile::unixFile(std::string filePath,int flag,int authority){
    fd = open(filePath.c_str(), flag, authority);
    if (fd <= 0)
        throw std::invalid_argument(filePath);
    struct stat st{};
    stat(filePath.c_str(), &st);
    pUnixInode = unixInodeInfo::Find(st.st_dev, st.st_ino);
    this->filePath = std::move(filePath);
    this->flag = flag;
}
unixFile::~unixFile() {
    close(fd);
    unixInodeInfo::Unregister(pUnixInode);
}

int unixFile::Read(void *buffer, ssize_t size) {
    int status = 0;
    pthread_mutex_t &mutex = pUnixInode->mutex;
    pthread_mutex_lock(&mutex);
    if (pUnixInode->lockType >= RESERVED_LOCK)
        goto error_handle;
    if (flock(fd, LOCK_SH) < 0)
        goto error_handle;
    pUnixInode->lockType = SHARED_LOCK;
    pUnixInode->nShared++;
    lockType = SHARED_LOCK;
    pthread_mutex_unlock(&mutex);


    status = read(fd, buffer, size);


    pthread_mutex_lock(&mutex);
    flock(fd, LOCK_UN);
    pUnixInode->nShared--;
    if (pUnixInode->nShared == 0)
        pUnixInode->lockType = NO_LOCK;
    lockType = NO_LOCK;
    pthread_mutex_unlock(&mutex);
    return status;

    error_handle:
    pthread_mutex_unlock(&mutex);
    return -2;
}

int unixFile::Write(void *buffer, ssize_t size) {
    int status = 0;
    pthread_mutex_t &mutex = pUnixInode->mutex;
    pthread_mutex_lock(&mutex);
    if (pUnixInode->lockType != NO_LOCK)
        goto error_handle;
    if (flock(fd, LOCK_EX) < 0)
        goto error_handle;
    pUnixInode->lockType = EXECUTE_LOCK;
    lockType = EXECUTE_LOCK;
    pthread_mutex_unlock(&mutex);

    status = write(fd, buffer, size);

    pthread_mutex_lock(&mutex);
    flock(fd, LOCK_UN);
    pUnixInode->lockType = NO_LOCK;
    lockType = NO_LOCK;
    pthread_mutex_unlock(&mutex);
    return status;

    error_handle:
    pthread_mutex_unlock(&mutex);
    return -2;
}


SafeIOFile *make_SafeIOFile(std::string file_path,int flag,int authority){
    auto*file = new unixFile(std::move(file_path), flag,authority);
    return file;
}



