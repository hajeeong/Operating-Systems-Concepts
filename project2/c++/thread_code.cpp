#include<iostream>
#include <thread>
#include "semaphore.h"

int gCount = 0;
Semaphore gLock(1);

void threadCode(int i) {
    gLock.wait();
    std::cout << "Thread " << i << " has count " << gCount << std::endl;
    gCount++;
    gLock.signal();
}

int main() {

    std::thread threads[5];

    for(int i=0; i<5;i++) {
        threads[i] = std::thread(threadCode, i);
    }

    for(int i=0; i<5; i++) {
        threads[i].join();
    }

    return 0;
}
