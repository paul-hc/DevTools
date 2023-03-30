#ifndef StdThread_h
#define StdThread_h
#pragma once


#ifdef IS_CPP_11
	#include <thread>
	#include <condition_variable>
	#include <mutex>
	#include <future>
#else
	#include <boost/thread.hpp>

	namespace std
	{
		using boost::thread;
		using boost::mutex;
		using boost::lock_guard;
		using boost::unique_lock;
		using boost::condition_variable;
	}
#endif


#endif // StdThread_h
