/********************************************************************

   spinlock.h

   Derived from https://www.codeproject.com/Articles/184046/Spin-Lock-in-C

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include <windows.h>

const unsigned int YIELD_ITERATION = 30; // yield after 30 iterations
const unsigned int MAX_SLEEP_ITERATION = 40;
const int SeedVal = 100;

// This class acts as a synchronization object similar to a mutex

struct SpinLock
{
	volatile LONG dest;
	LONG exchange;
	LONG compare;
	unsigned int m_iterations;

public:
	SpinLock()
	{
		dest = 0;
		exchange = SeedVal;
		compare = 0;
		m_iterations = 0;
	}

	void Lock();
	void Unlock();

	inline bool HasThresholdReached() { return (m_iterations >= YIELD_ITERATION); }
};


void SpinLock::Lock()
{
	m_iterations = 0;
	while (true)
	{
		// A thread already owning the lock shouldn't be allowed to wait to acquire the lock - reentrant safe
		if (this->dest == GetCurrentThreadId())
			break;
		/*
		Spinning in a loop of interlockedxxx calls can reduce the available memory bandwidth and slow
		down the rest of the system. Interlocked calls are expensive in their use of the system memory
		bus. It is better to see if the 'dest' value is what it is expected and then retry interlockedxxx.
		*/
		if (InterlockedCompareExchange(&this->dest, this->exchange, this->compare) == 0)
		{
			//assign CurrentThreadId to dest to make it re-entrant safe
			this->dest = GetCurrentThreadId();
			// lock acquired 
			break;
		}

		// spin wait to acquire 
		while (this->dest != this->compare)
		{
			if (HasThresholdReached())
			{
				if (m_iterations + YIELD_ITERATION >= MAX_SLEEP_ITERATION)
					Sleep(0);

				if (m_iterations >= YIELD_ITERATION && m_iterations < MAX_SLEEP_ITERATION)
				{
					m_iterations = 0;
					SwitchToThread();
				}
			}
			// Yield processor on multi-processor but if on single processor then give other thread the CPU
			m_iterations++;
			YieldProcessor(/*no op*/);
		}
	}
}


void SpinLock::Unlock()
{
	if (this->dest != GetCurrentThreadId())
		throw std::runtime_error("Unexpected thread-id in release");

	// lock released
	InterlockedCompareExchange(&this->dest, this->compare, GetCurrentThreadId());
}


