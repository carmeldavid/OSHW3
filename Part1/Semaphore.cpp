#define  DEAFAULT_VAL 0
#include "Semaphore.hpp"


Semaphore::Semaphore(): counter(DEAFAULT_VAL) {}
Semaphore::Semaphore(unsigned int val): counter(val) {}

void Semaphore::down(){
    pthread_mutex_lock(&m);
    while (counter == 0) {
        pthread_cond_wait(&c, &m);
    }
    counter--;
    pthread_mutex_unlock(&m);
}
void Semaphore::up(){
    pthread_mutex_lock(&m);
    counter++;
    pthread_cond_signal(&c);
    pthread_mutex_unlock(&m);
}
