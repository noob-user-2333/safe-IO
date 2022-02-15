#include <iostream>
#include "safe_IO.h"
void* test(void *arg){
    std::unique_ptr<SafeIOFile> file(make_SafeIOFile("/dev/shm/test",O_RDWR | O_CREAT,0666));
    file->Write((void *) "test", 4);
//    std::cout << "succeed for test " << std::endl;
    return 0;
}
void* test1(void *arg){
    std::unique_ptr<SafeIOFile> file(make_SafeIOFile("/dev/shm/test",O_RDWR | O_CREAT,0666));
    file->Write((void *) "test", 4,4);
//    std::cout << "test1 running normal" << std::endl;
    return 0;
}
pthread_t thread1,thread2;
int volatile num = 5;
int* p = (int*)&num;
int main() {
    *p = 11;
//    SafeIOFile* file(make_SafeIOFile("/dev/shm/test",O_RDWR | O_CREAT,0666));
//    file->Write((void *) "test", 4,4);
//    std::cout << "test1 running normal" << std::endl;
//    delete file;
    pthread_create(&thread1,NULL,test,NULL);
    pthread_create(&thread2,NULL,test1,NULL);
    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);
//    std::cout << "Hello, World!" << std::endl;
    return 0;
}
