/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.

	Copyright (C) 2015-2020 Barcelona Supercomputing Center (BSC)
*/

#include <cassert>
#include <ctime>
#include <iostream>
#include <time.h>

#include "LeaderThread.hpp"
#include "PollingAPI.hpp"
#include "lowlevel/EnvironmentVariable.hpp"

#include <InstrumentLeaderThread.hpp>
#include <InstrumentThreadManagement.hpp>


LeaderThread *LeaderThread::_singleton;
CPU *LeaderThread::_leaderThreadCPU;


void LeaderThread::initialize()
{
	_singleton = new LeaderThread();
	_singleton->start(nullptr);
}

void LeaderThread::shutdown()
{
	assert(_singleton != nullptr);

	bool expected = false;
	_singleton->_mustExit.compare_exchange_strong(expected, true);
	assert(!expected);

	_singleton->join();

	delete _singleton;
	_singleton = nullptr;

	// Delete the LeaderThread's CPU
	assert(_leaderThreadCPU != nullptr);

	delete _leaderThreadCPU;
}

void LeaderThread::body()
{
	initializeHelperThread();

	// Minimum polling frequency in microseconds
	EnvironmentVariable<int> pollingFrequency("NANOS6_POLLING_FREQUENCY", 1000);

	while (!std::atomic_load_explicit(&_mustExit, std::memory_order_relaxed)) {
		struct timespec delay = {0, pollingFrequency * 1000};

		// The loop repeats the call with the remaining time in the event that
		// the thread received a signal with a handler that has SA_RESTART set
		Instrument::threadWillSuspend(getInstrumentationId());
		while (nanosleep(&delay, &delay)) {
		}
		Instrument::threadHasResumed(getInstrumentationId());

		PollingAPI::handleServices();

		Instrument::leaderThreadSpin();
	}

	Instrument::threadWillShutdown();

	return;
}
