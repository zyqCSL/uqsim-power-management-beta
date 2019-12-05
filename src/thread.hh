#ifndef _Thread_HH_
#define _Thread_HH_
#include <assert.h>
#include <set>
// #include <time.h>
// #include <stdlib.h>

#include "code_path.hh"
#include "event.hh"

// Thread also models part of applicaiton level scheduling
class Thread
{
	public:
		enum State {RUNNING, BLOCKED, READY};
	protected:
		unsigned threadId;
		bool bindConn;

		// path
		std::vector<CodePath> paths;
		// receive queues
		std::list<Job*>** queues;

		std::set<unsigned> connections;
		State state;

		// next event
		unsigned nextPath;
		// if this job can be scheduled to run
		bool schedSuc;

		bool debug;

		// sequence number for blocking
		// used to deal with stages that are both batching and blocking
		unsigned blkSeqNum;

	public:
		Thread(unsigned id, bool bindcon, const std::vector<CodePath>& path, bool debug);
		virtual ~Thread();
		unsigned getId();

		bool hasConn(unsigned conn);
		void removeConn(unsigned conn);
		virtual void enqueue(Job* j);
		// need to ovrride this to change to task time setting for preemptive scheduling
		virtual void run(Time globalTime, Event* event);

		void setRunning();
		void unsetRunning();
		bool isRunning();

		void block();
		void unblock();
		bool isBlocked();

		bool isReady();

		// return the stageId (not pathStageId) of the stage scheduled to run next
		unsigned getNextStageId();

		// check if there are jobs in thread's next scheduled stage
		// must check before continue running because in non-blocking stages with chunk/send req
		// thread remains in the same stage, but there is no job in front of the stage
		bool jobInNextStage();

		bool jobLeft();

		// dvfs
		void incFullFreq();
		void incFreq();
		void decFreq();
		void setFreq(unsigned freq);

	protected:
		// set the next stage of certain path
		// in next round of nextEventTime(), thread only check the stage decided by this function
		// to find executable jobs.
		// override this function to implement more specific application level scheduling
		// when reset is true, reset next stage according to priority (for non-blking stages with syncRespNum > 0)
		virtual void setNextStage(unsigned path, bool reset);

		// return the next job to execute at a certain stage
		// override this function to implement more specific application level sched,
		// such as SJF, delay scheduling
		virtual Job* getNextJob(unsigned path, unsigned stage);

		// get all jobs from a specific connection (for batching stage bound to connection)
		virtual void getJobByConn(unsigned path, unsigned stage, unsigned conn, std::list<Job*>& jList);
		virtual void getJobPerConn(unsigned path, unsigned stage, std::list<Job*>&jList);
		virtual unsigned jobNumByConn(unsigned path, unsigned stage, unsigned conn);

		// get the number of active connections in the queue
		virtual unsigned numConn(unsigned path, unsigned stage);

	public:
		// reset the nextGroup and nextPath fields
		// return true if a schedulable event is found
		// override this for other application level scheduling algorithm
		virtual bool schedule();
};

#endif