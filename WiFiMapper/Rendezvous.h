/*
* A module for synchronizing concurrent threads.
*
* Written by Marc Katzef
*/

#pragma once

#include "stdafx.h"
#include <strsafe.h>
#include <mutex>
#include <condition_variable>

class Rendezvous {
public:
	Rendezvous(unsigned int threadCount) : m_threadCount(threadCount), m_waiting(0) { }
	
	void arrive(void) {
		std::unique_lock<std::mutex> lock(m_countMutex);
		m_waiting++;
		if (m_waiting < m_threadCount) {
			int wakeCountSample = wakeCount;
			do {
				m_waitingCV.wait(lock);
			} while (wakeCount == wakeCountSample); // avoid spurious wakeups
		} else {
			m_waiting = 0;
			wakeCount++;
			m_waitingCV.notify_all();
		}
		lock.unlock();
	}

	void destroy() {
		m_waitingCV.notify_all();
	}

private:
	unsigned int m_threadCount;
	unsigned int m_waiting;
	std::mutex m_countMutex;
	std::condition_variable m_waitingCV;
	int wakeCount = 0;
};