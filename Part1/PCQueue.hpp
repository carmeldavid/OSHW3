#ifndef _QUEUEL_H
#define _QUEUEL_H
#include "Headers.hpp"
#include "Semaphore.hpp"
// Single Producer - Multiple Consumer queue
template <typename T>class PCQueue
{

public:
	// Blocks while queue is empty. When queue holds items, allows for a single
	// thread to enter and remove an item from the front of the queue and return it. 
	// Assumes multiple consumers.
	T pop(){
        pthread_mutex_lock(&mutex_);
        while (qsize==0){
            pthread_cond_wait(&condvar_,&mutex_);
        }
        T retval = queue_.front();
        queue_.pop();
        qsize--;
        pthread_mutex_unlock(&mutex_);
        return retval;

    }
	// Allows for producer to enter with *minimal delay* and push items to back of the queue.
	// Hint for *minimal delay* - Allow the consumers to delay the producer as little as possible.  
	// Assumes single producer 
    void push(const T& item){
        pthread_mutex_lock(&mutex_);
        queue_.push(item);
        qsize++;
        pthread_cond_signal(&condvar_);
        pthread_mutex_unlock(&mutex_);
    }


private:
    std::queue<T> queue_;
    pthread_mutex_t mutex_;
    pthread_cond_t condvar_;
    int qsize = 0;
    // Add your class memebers here
};
// Recommendation: Use the implementation of the std::queue for this exercise
#endif
