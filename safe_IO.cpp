//
// Created by user on 2022/2/9.
//

#include <list>
#include <sys/file.h>
#include "safe_IO.h"

static std::list<unixInodeInfo *> inodeList;
static pthread_mutex_t listMutex;
static long listInit = 0;
struct unixFileId {
    dev_t dev;
    unsigned long ino;
};

struct unixInodeInfo {

public:
    unixFileId fileId;
    pthread_mutex_t mutex;
    long nShared = 0;
    long lockType = unixFile::NO_LOCK;

    constexpr static long SHARED_BYTE = 0x10000;
    constexpr static long RESERVED_BYTE = 0x10001;
    constexpr static long EXECUTE_BYTE = 0x10002;


    unixInodeInfo(dev_t dev, unsigned long ino) {
        fileId.dev = dev;
        fileId.ino = ino;
        pthread_mutex_init(&mutex, nullptr);
        if(listInit == 0){
            listInit = 1;
            pthread_mutex_init(&listMutex,nullptr);
        }
    }

    ~unixInodeInfo() {
        pthread_mutex_destroy(&mutex);

        pthread_mutex_lock(&listMutex);
        for (auto it = inodeList.begin(); it != inodeList.end(); ++it) {
            if (*it == this) {
                inodeList.erase(it);
                break;
            }
        }
        pthread_mutex_unlock(&listMutex);
    }

    static unixInodeInfo *Find(dev_t dev, unsigned long ino) {
        pthread_mutex_lock(&listMutex);
        for (auto it = inodeList.begin(); it != inodeList.end(); it++) {
            if ((*it)->fileId.dev == dev &&
                (*it)->fileId.ino == ino) {
                pthread_mutex_unlock(&listMutex);
                return *it;
            }
        }
        pthread_mutex_unlock(&listMutex);
        auto newInode = new unixInodeInfo(dev, ino);

        pthread_mutex_lock(&listMutex);
        inodeList.push_back(newInode);
        pthread_mutex_unlock(&listMutex);
        return newInode;
    }
};

unixFile::unixFile(std::string filePath, int flag, int authority) {
    fd = open(filePath.c_str(), flag, authority);
    if (fd <= 0)
        throw std::invalid_argument(filePath);
    struct stat st{};
    stat(filePath.c_str(), &st);
    pUnixInode.reset(unixInodeInfo::Find(st.st_dev, st.st_ino));
    this->filePath = std::move(filePath);
    this->flag = flag;
}

unixFile::unixFile(std::string filePath, int flag) {
    fd = open(filePath.c_str(), flag);
    if (fd <= 0)
        throw std::invalid_argument(filePath);
    struct stat st{};
    stat(filePath.c_str(), &st);
    pUnixInode.reset(unixInodeInfo::Find(st.st_dev, st.st_ino));
    this->filePath = std::move(filePath);
    this->flag = flag;
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





