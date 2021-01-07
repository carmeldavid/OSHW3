#ifndef _QUEUEL_H
#define _QUEUEL_H
#include "Headers.hpp"
// Single Producer - Multiple Consumer queue
template <typename T>class PCQueue
{
public:
	// Blocks while queue is empty. When queue holds items, allows for a single
	// thread to enter and remove an item from the front of the queue and return it. 
	// Assumes multiple consumers.
	T pop(){


        pthread_mutex_lock(&m);

        while (writers_inside > 0 || writers_waiting > 0 || q.empty())
            pthread_cond_wait(&read_c, &m);



        readers_inside++;
        T returnT = q.front();

        q.pop();
       /* pthread_mutex_unlock(&m);
        pthread_mutex_lock(&m);*/
        readers_inside--;
        if (readers_inside == 0)
            pthread_cond_signal(&write_c);
        pthread_mutex_unlock(&m);
        return returnT;
    }
    PCQueue()  : readers_inside(0),writers_inside(0),writers_waiting(0) {
	    pthread_mutex_init(&m,NULL);
        pthread_cond_init(&read_c,NULL);
        pthread_cond_init(&write_c,NULL);

    };
    ~PCQueue() {
        pthread_mutex_destroy(&m);
        pthread_cond_destroy(&read_c);
        pthread_cond_destroy(&write_c);
    };
	// Allows for producer to enter with *minimal delay* and push items to back of the queue.
	// Hint for *minimal delay* - Allow the consumers to delay the producer as little as possible.  
	// Assumes single producer 
	void push(const T& item)  {

       pthread_mutex_lock(&m);


        writers_waiting++;

        while (writers_inside + readers_inside > 0)
            pthread_cond_wait(&write_c, &m);
        writers_waiting--;
        writers_inside++;

        q.push(item);
       /* pthread_mutex_unlock(&m);

       pthread_mutex_lock(&m);*/
        writers_inside--;

        if (writers_inside == 0) {
            pthread_cond_broadcast(&read_c);
            pthread_cond_signal(&write_c);

        }
        pthread_mutex_unlock(&m);

    }


private:
    int readers_inside, writers_inside, writers_waiting;
    pthread_cond_t read_c;
    pthread_cond_t write_c;
    pthread_mutex_t m;
    std::queue<T> q;
};
// Recommendation: Use the implementation of the std::queue for this exercise
#endif
