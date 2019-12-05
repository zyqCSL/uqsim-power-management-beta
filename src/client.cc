#include <iostream>
#include <stdio.h>

#include "client.hh"

int compareDouble(const void* a, const void* b) {
	if(*(double*)a == *(double*)b)
		return 0;
	else if(*(double*)a > *(double*)b)
		return 1;
	else
		return -1;
}

Client::Client(unsigned total, unsigned nconn, Time net_lat, bool debug, Time monitor_interval): 
		numTotal(total), numConn(nconn), netLat(net_lat), debug(debug), monitorInterval(monitor_interval), 
		lastMonitorTime(0), targTailLat(0), tailLat(0) {
	count = 0;
	nextTime = 0;
	srand(time(NULL));
	if(numTotal > 0) {
		finalTime = new Time[numTotal];
		startTime = new Time[numTotal];
		complete = new bool[numTotal];
		for(unsigned i = 0; i < numTotal; ++i)
			complete[i] = false;
	}

	allJobIssued = false;
	curEpoch = 0;

	// performance monitoring, assume 50k qps
	uint64_t init_size = uint64_t(monitorInterval/1000000000.0*50);
	respTimeRecords = new JobTimeRecords(init_size);
}

Client::~Client() {
	delete tm;
	delete finalTime;
	delete respTimeRecords;
}

void 
Client::setEntry(Cluster* c) {
	entry = c;
}

void
Client::setTimeModel(TimeModel* t) {
	tm = t;
}

void
Client::addEpoch(Time epoch_end_time, uint64_t rps) {
	if(epochEndTime.size() > 0)
		assert(epochEndTime[epochEndTime.size() - 1] < epoch_end_time);
	assert(rps > 0);
	epochEndTime.push_back(epoch_end_time);
	epochQps.push_back(rps);
}

bool
Client::isAllJobIssued() {
	return allJobIssued;
}

// TODO:change this logic to include monitor event & epoch defined approach
Time 
Client::nextEventTime() {
	// first check if all job issued
	if(allJobIssued)
		// return lastMonitorTime + monitorInterval;
		return INVALID_TIME;
	else {
		return (lastMonitorTime + monitorInterval) < nextTime ? (lastMonitorTime + monitorInterval): nextTime;
		// return nextTime;
	}
}

bool
Client::needSched(Time cur_time) {
	// std::cout << "in need sched" << std::endl;
	// std::cout << "cur_time = " << cur_time << std::endl;
	// std::cout << "lastMonitorTime = " << lastMonitorTime << std::endl;
	// std::cout << "monitorInterval = " << monitorInterval << std::endl;
	if(cur_time >= lastMonitorTime + monitorInterval) {
		// std::cout << "cur_time = " << cur_time << std::endl;
		// std::cout << "lastMonitorTime = " << lastMonitorTime << std::endl;
		// std::cout << "monitorInterval = " << monitorInterval << std::endl;
		return true;
	}
	else 
		return false;
}

bool
Client::needRun(Time cur_time) {
	if(allJobIssued)
		return false;
	if(cur_time >= nextTime)
		return true;
	else
		return false;
}

void
Client::setLastMonitorTime(Time cur_time) {
	lastMonitorTime = cur_time;
}

void
Client::run(Time time) {
	if(!allJobIssued && time >= nextTime) {
		unsigned connId = rand() % numConn;
		// if(numTotal > 0)
		startTime[count] = nextTime;

		Job *j = new Job(count, connId, respTimeRecords, nullptr, nextTime, &(finalTime[count]), &(complete[count]), debug);

		if(debug) {
			// std::cout << "client interval = " << interval << std::endl;
			std::cout << "In Client job: " << j->id << " start at " << nextTime << std::endl << std::endl;
			// std::cout << std::endl;
		}

		j->time += netLat;
		entry->enqueue(j);
		++count;
		Time interval = tm->lat();

		// if(debug) {
		// 	std::cout << "client interval = " << interval << std::endl;
		// 	std::cout << "job: " << j->idx << " start at " << nextTime << std::endl;
		// 	std::cout << std::endl;
		// }
		nextTime += interval;

		// check epoch change and all job issued
		if(count >= numTotal && numTotal != 0)
			allJobIssued = true;
		// check epoch change
		if(epochEndTime.size() > 0 && nextTime >= epochEndTime[curEpoch]) {
			assert(!allJobIssued);
			if(curEpoch == epochEndTime.size() - 1)
				allJobIssued = true;
			else {
				// change epoch, reset time distribution
				++curEpoch;
				Time new_lat = (Time)(1000000000.0/epochQps[curEpoch]);
				tm->reset(new_lat);
			}
		}
	}
	
	curTime = time;

	// // rapl control logic
	// if(curTime - lastMonitorTime >= monitorInterval) {

	// 	Time tail_lat = respTimeRecords->getTailLat();

	// 	// for(unsigned i = 0; i < count; ++i) {
	// 	// 	if(!complete[i] && (curTime - startTime[i]) >= targTailLat)
	// 	// 		respTimeRecords->jobCompl(curTime - startTime[i]);
	// 	// }

	// 	// double tail_lat_all = respTimeRecords->getTailLat();
	// 	respTimeRecords->clear();

	// 	// rapl report
	// 	std::cout << std::endl << "99% tail (job complete) lat within [" << (double)lastMonitorTime/1000000000.0 << "s, " <<
	// 		(double)curTime/1000000000.0 << "s) = " << (double)tail_lat/1000000.0 <<  ' ms' << std::endl;

	// 	// std::cout << "99% tail (job complete & issued in-complete) lat within [" << lastMonitorTime << ", " <<
	// 	// 	curTime << ") = " << tail_lat_all << std::endl;

	// 	// use all timing info to make decision
	// 	// tail_lat = tail_lat_all;

	// 	// update timer
	// 	lastMonitorTime = curTime;

	// 	// no job completed in this period, do nothing
	// 	// must be done after timer is updated
	// 	if(tail_lat < 0)
	// 		return;
	// 	double rapl_event_time = curTime + netLat;
	// 	if(tail_lat >= targTailLat) {
	// 		// simple fall back path
	// 		if(fallbackPolicy == RaplFallbackPolicy::IMMEDIATE) {
	// 			// increase to full freq
	// 			if(memcachedFreq != 2600)
	// 				entry->incServFreqFull(rapl_event_time, "memcached");
	// 			if(ngxFreq != 2600)
	// 				entry->incServFreqFull(rapl_event_time, "nginx");
	// 			memcachedFreq = 2600;
	// 			ngxFreq = 2600;
	// 		} else if(fallbackPolicy == RaplFallbackPolicy::GRADUAL) {
	// 			// increase gradually
	// 			if(memcachedFreq != 2600) {
	// 				entry->incServFreq(rapl_event_time, "memcached");
	// 				memcachedFreq += 200;
	// 			}
	// 			if(ngxFreq != 2600) {
	// 				entry->incServFreq(rapl_event_time, "nginx");
	// 				ngxFreq += 200;
	// 			}
	// 		}
			
	// 	} else if((targTailLat - tail_lat)/targTailLat <= 200) {
	// 		// stop decreasing freq
	// 		return;
	// 	} else {
	// 		// decrease freq
	// 		if(policy == RaplPolicy::MEMCACHED_FIRST) {
	// 			if(memcachedFreq > 1200) {
	// 				entry->decServFreq(rapl_event_time, "memcached");
	// 				memcachedFreq -= 200;
	// 			} else if(ngxFreq > 1200) {
	// 				entry->decServFreq(rapl_event_time, "nginx");
	// 				ngxFreq -= 200;
	// 			}
	// 		} else if(policy == RaplPolicy::NGX_FIRST) {
	// 			if(ngxFreq > 1200) {
	// 				entry->decServFreq(rapl_event_time, "nginx");
	// 				ngxFreq -= 200;
	// 			} else if(memcachedFreq > 1200) {
	// 				entry->decServFreq(rapl_event_time, "memcached");
	// 				memcachedFreq -= 200;
	// 			}
	// 		} else if(policy == RaplPolicy::LOCKSTEP) {
	// 			if(ngxFreq > 1200) {
	// 				entry->decServFreq(rapl_event_time, "nginx");
	// 				ngxFreq -= 200;
	// 			} 
	// 			if(memcachedFreq > 1200) {
	// 				entry->decServFreq(rapl_event_time, "memcached");
	// 				memcachedFreq -= 200;
	// 			}
	// 		}
	// 	}
	// }
	// // rapl control logic ends
}

void
Client::show() {
	if(debug) {
		// double* table = new double[numTotal];
		// for(unsigned i = 0; i < numTotal; ++i) 
		// 	table[i] = finalTime[i] - startTime[i];
		// // std::cout << "before qsort" << std::endl;
		// qsort(table, numTotal, sizeof(double), compareDouble);
		// for(unsigned i = 0; i < numTotal; ++i)
		// 	printf("%.20f\n", table[i]);
		// 	// std::cout << table[i] << std::endl;
		// delete[] table;
		return;
	} else {
		// for(unsigned i = 0; i < numTotal; ++i)
		// 	printf("%.20f\n", finalTime[i] - startTime[i]);
			// std::cout << finalTime[i] - startTime[i] << std::endl;

		// only show events after next epoch event
		Time tail_lat = respTimeRecords->getTailLat();
		std::cout << "99% tail lat within [" << (double)lastMonitorTime/1000000000.0 << "s, sim_end) = " << (double)tail_lat/1000000.0
			<< "ms" << std::endl; 

		Time avg_lat = respTimeRecords->getAvgLat();
		std::cout << "average lat within [" << (double)lastMonitorTime/1000000000.0 << "s, sim_end) = " << (double)avg_lat/1000000.0
			<< "ms" << std::endl; 
	}
}

void
Client::setTimeArray() {
	if(numTotal == 0) {
		Time lastTime = 0;
		for(unsigned i = 0; i < epochEndTime.size(); ++i) {
			Time interval = epochEndTime[i] - lastTime;
			lastTime = epochEndTime[i];		// in nanoseconds
			numTotal += uint64_t(interval/1000000000.0 * epochQps[i]);
		}

		std::cout << "numTotal = " << numTotal << std::endl;

		finalTime = new Time[numTotal];
		startTime = new Time[numTotal];
		complete  = new bool[numTotal];
		for(uint64_t i = 0; i < numTotal; ++i)
			complete[i] = false;

		// also set up cli time model according to first epoch
		Time new_lat = (Time)(1000000000/epochQps[0]);
		assert(tm != nullptr);
		tm->reset(new_lat);
	} else {
		Time lastTime = 0;
		finalTime = new Time[numTotal];
		startTime = new Time[numTotal];
		complete = new bool[numTotal];
		for(uint64_t i = 0; i < numTotal; ++i)
			complete[i] = false;
	}
}

Time
Client::getTailLat() {
	return respTimeRecords->getTailLat();
}

void
Client::clearRespTime() {
	respTimeRecords->clear();
}

uint64_t
Client::getCurQps() {
	return epochQps[curEpoch];
}