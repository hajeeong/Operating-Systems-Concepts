#include "semaphore.h"

void Semaphore::initialize(int value) {
    if(_init) throw reinit_error();
    _init = true;
    _count = value;
}

void Semaphore::wait() {
    std::unique_lock<std::mutex> lock(_semLock);
    while(_count <= 0) _signaled.wait(lock);
    _count--;
}

void Semaphore::signal() {
    std::lock_guard<std::mutex> lock(_semLock);
    _count++;
    _signaled.notify_one();
}
