#ifndef __SEMAPHORE_H_
#define __SEMAPHORE_H_
#include <mutex>
#include <condition_variable>
#include <exception>

class Semaphore {
    public:
        Semaphore() : _init(false) {}
        Semaphore(int init) : _init(true), _count(init) {}
        void initialize(int value);
        void wait();
        void signal();
        class reinit_error : std::exception {
            public:
                const char* what() const noexcept {
                    return "Semaphore already initialized";
                }
        };
    private:
        std::mutex _semLock;
        std::condition_variable _signaled;
        int _count;
        bool _init;
};
#endif
