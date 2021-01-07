
#include "Thread.hpp"



	Thread::Thread(uint thread_id) : m_thread_id(thread_id) {}


	bool Thread::start(){

		return (!pthread_create(&m_thread, NULL, entry_func, this));
	}

	void Thread::join()
	{
		       pthread_join(m_thread, NULL);
	}

	uint Thread::thread_id()
	{
		return m_thread_id;
	}

