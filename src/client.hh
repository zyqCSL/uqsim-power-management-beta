#ifndef _CLIENT_HH_
#define _CLIENT_HH_

#include <time.h>
#include <string>

#include "cluster.hh"

int compareDouble(const void* a, const void* b);

class Client
{
	protected:
		Cluster* entry;
		Time nextTime;

		uint64_t count;
	public:
		uint64_t numTotal;
	protected:
		unsigned numConn;

		TimeModel* tm;
		Time netLat;
		Time* finalTime;
		Time* startTime;
		bool* complete;

		bool debug;

		// all jobs have been issued
		bool allJobIssued;

		// dvfs
		Time curTime; // client clock to issue monitor event
		
		Time monitorInterval;
		Time lastMonitorTime;

		Time targTailLat;
		// job resp time after the last monitor event
		std::vector<Time> respTime;

		// workload pattern
		std::vector<Time> epochEndTime;	// unit: us
		std::vector<uint64_t> epochQps;
		unsigned curEpoch;

		// tail latency before the previous monitor event
		Time tailLat;

	public:
		// job response time monitoring
		JobTimeRecords* respTimeRecords;

	public:
		Client(unsigned total, unsigned nconn, Time net_lat, bool debug, Time monitor_interval);
		~Client();
		void setEntry(Cluster* s);
		void setTimeModel(TimeModel* t);

		void addEpoch(Time epoch_end_time, uint64_t rps);
		bool isAllJobIssued();

		Time nextEventTime();
		bool needSched(Time cur_time);
		bool needRun(Time cur_time);
		void setLastMonitorTime(Time cur_time);

		void run(Time time);
		void show();

		void setTimeArray();

		Time getTailLat();
		void clearRespTime();
		uint64_t getCurQps();
};

#endif