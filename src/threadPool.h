#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <algorithm>
#include <exception>

namespace myWebServ 
{

template <typename T>
class threadPool
{
	public:
		threadPool(int thread_nums=4, int max_requests=1000):m_thread_nums(thread_nums),
		m_max_requests(max_requests),m_running(false)
		{
		}
		~threadPool()
		{
			if(m_running)
			{
				stop();
			}
		}

		threadPool(const threadPool&) = delete;
		threadPool& operator= (const threadPool&) = delete;

		void start()
		{

			if(m_thread_nums<=0 || m_max_requests<=0)
			{
				throw std::exception();
			}
			m_running = true;
			m_threads.reserve(m_thread_nums);
			for(int i=0; i<m_thread_nums; ++i)
			{
				m_threads.push_back(std::thread(std::bind(&threadPool::run, this)));
			}
		}

		void stop()
		{
			{
				std::unique_lock<std::mutex> ul(m_mutex);
				m_running = false;
				m_not_empty.notify_all();
			}
			for(auto &it : m_threads)
			{
				it.join();
			}
		}

		void append(T t)
		{
			if(!m_running || m_threads.empty())
			{
				throw std::exception();
			}
			std::unique_lock<std::mutex> ul(m_mutex);
			m_not_full.wait(ul, [this](){ return m_queue.size()<m_max_requests;} );
			m_queue.push_back(t);
			m_not_empty.notify_one();
		}
		T take()
		{

			std::unique_lock<std::mutex> ul(m_mutex);
			m_not_empty.wait(ul, [this](){ return m_queue.size()>0; });
			T t=m_queue.front();
			m_queue.pop_front();
			m_not_full.notify_one();
			return t;
		}
	private:
		void run()
		{
			while(m_running)
			{
				T t(take());// 取任务
				if(!t)
				{
					continue;
				}
				else
				{
					t->process();// 
				}
			}
		}
	private:
		int m_thread_nums;
		int m_max_requests;
		std::vector<std::thread> m_threads;
		std::deque<T> m_queue;
		std::mutex  m_mutex;
		std::condition_variable m_not_empty;
		std::condition_variable m_not_full;
		bool m_running;
};

}



#endif
