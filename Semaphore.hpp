#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H
#include "Headers.hpp"
// Synchronization Warm up
class Semaphore {
private:
    pthread_mutex_t mutex;
    pthread_cond_t condvar;
    int counter;

public:
	Semaphore() { // Constructs a new semaphore with a counter of 0
	    pthread_mutex_init(&mutex,nullptr);
	    pthread_cond_init(&condvar,nullptr);
	    counter = 0;
	}
	Semaphore(unsigned val) { // Constructs a new semaphore with a counter of val
        pthread_mutex_init(&mutex,nullptr);
        pthread_cond_init(&condvar,nullptr);
	    counter = val;
	}

	void up() { // Mark: 1 Thread has left the critical section
       pthread_mutex_lock(&mutex);
       if(counter++ == 0){
           pthread_cond_signal(&condvar);
       }
       pthread_mutex_unlock(&mutex);
	}

    void down() { // Block until counter >0, and mark - One thread has entered the critical section.
	    pthread_mutex_lock(&mutex);
	    while(counter == 0){
	        pthread_cond_wait(&condvar,&mutex);
	    }
	    counter--;
	    pthread_mutex_unlock(&mutex);
	}
};
#endif
