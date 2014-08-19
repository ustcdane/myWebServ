// Copyright (c) 2014 Daniel Wang. All rights reserved.
// https://github.com/ustcdane/myWebServ
// Use of this source code is governed by a BSD-style license
// that can be found in the License file
// Author: Daniel Wang(daneustc at gmail dot com)

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

		void start() // 开始线程池
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

		void stop()// 停止线程池
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

		void append(T t)// 任务队列中添加任务
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
		T take()// 取走任务
		{

			std::unique_lock<std::mutex> ul(m_mutex);
			m_not_empty.wait(ul, [this](){ return m_queue.size()>0; });
			T t=m_queue.front();
			m_queue.pop_front();
			m_not_full.notify_one();
			return t;
		}
	private:
		void run()// 工作线程实际操作
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
		int m_thread_nums;// 线程池个数 
		int m_max_requests;// 最大容量
		std::vector<std::thread> m_threads;// 线程池 
		std::deque<T> m_queue;// 任务队列
		std::mutex  m_mutex;// 互斥锁
		std::condition_variable m_not_empty;// 非空条件变量
		std::condition_variable m_not_full;// 非满条件变量
		bool m_running;// 是否运行
};

}



#endif
