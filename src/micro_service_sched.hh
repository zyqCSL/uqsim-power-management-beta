#ifndef _MSERV_SCHED_HH_
#define _MSERV_SCHED_HH_

#include <assert.h>
#include <random>
#include <stdlib.h>
#include <time.h>
#include <unordered_map>
#include <stdlib.h>

#include "thread.hh"
#include "core.hh"
#include "event.hh"

class MicroServScheduler
{
	protected:
		unsigned servId;
		// path
		std::vector<CodePath> paths;
		bool bindConn;	

		unsigned numThreads;
		unsigned numCores;

		std::vector<Thread*> threadPool;
		std::vector<Core*> corePool;
		std::unordered_map<unsigned, unsigned*> criSect;

		bool debug;

	public:
		MicroServScheduler(unsigned servid, const std::vector<CodePath>& path, 
			std::unordered_map<unsigned, unsigned*> cri_sec, bool bindconn, unsigned num_threads, 
			std::vector<Core*> core_list, bool debug);
		virtual ~MicroServScheduler() {}
		// for container scheduler
		virtual void assignThread() = 0;
		virtual void assignCore(Core* c);
		virtual void eventHandler(Event* event, Time globalTime, std::list<Event*>& eventList) = 0;
		virtual void setCoreAffinity(unsigned tid, std::vector<Core*> cores) = 0;

		// dvfs
		virtual void incFullFreq();
		virtual void incFreq();
		virtual void decFreq();
		virtual void setFreq(Time freq);
		double getCpuUtil(Time elapsed_time);

	protected:
		// assign a thread to job
		// return true if dispatch succeeds
		virtual bool dispatch(Job* j) = 0;
		// schedule new events (new computation)
		virtual void schedule(Time globalTime, std::list<Event*>& eventList) = 0;

	public:
		virtual bool jobLeft() = 0;
};


// coarse grained multi-threading
class CMTScheduler: public MicroServScheduler
{
	protected:
		// for round-robin scheduling
		unsigned threadId;
		// by default, every thread can run any core in scheduler's corePool
		std::unordered_map<unsigned, std::vector<Core*>> coreAffinity;

		void releaseRsc(Event* event);

	public:
		CMTScheduler(unsigned servid, const std::vector<CodePath>& path, std::unordered_map<unsigned, unsigned*> cri_sec,
				bool bindconn, unsigned num_threads, std::vector<Core*> core_list, bool debug);
		~CMTScheduler();
		void assignThread() override;

		void eventHandler(Event* event, Time globalTime, std::list<Event*>& eventList) override;
		bool dispatch(Job* j) override;

		// schedule new tasks on ready threads
		void schedule(Time globalTime, std::list<Event*>& eventList);

		bool jobLeft() override;

		void setCoreAffinity(unsigned tid, std::vector<Core*> cores);
};

// a simplifed scheduler without abstraction of threads
class SimpScheduler: public MicroServScheduler
{
	protected:
		// for round-robin queue selection
		unsigned nextQue;
		// no threads in SimpScheduler, so it directly holds queues
		std::vector< std::deque<Job*> > queues;
		// by default, every queue can run any core in scheduler's corePool
		std::unordered_map<unsigned, std::vector<Core*>> coreAffinity;

		void releaseRsc(Event* event);

	public:
		SimpScheduler(unsigned servid, const std::vector<CodePath>& path, bool bindconn, unsigned num_que,
				std::vector<Core*> core_list, bool debug);
		virtual ~SimpScheduler() {}

		void assignThread() override;
		void eventHandler(Event* event, Time globalTime, std::list<Event*>& eventList) override;
		virtual bool dispatch(Job* j) override;
		// schedule new tasks on ready threads
		void schedule(Time globalTime, std::list<Event*>& eventList);
		bool jobLeft() override;
		virtual void setCoreAffinity(unsigned qid, std::vector<Core*> cores);
};

// linux network stack's dedicated scheduler
class LinuxNetScheduler: public SimpScheduler
{
	protected:
		// each nic queue is actually a pair of bidirectional queues
		unsigned numBiDirQueues;

	public:
		LinuxNetScheduler(unsigned servid, const std::vector<CodePath>& path, bool bindconn, unsigned num_que,
				std::vector<Core*> core_list, bool debug);
		~LinuxNetScheduler() {}

		bool dispatch(Job* j) override;
		void setCoreAffinity(unsigned qid, std::vector<Core*> cores) override;
};

#endif