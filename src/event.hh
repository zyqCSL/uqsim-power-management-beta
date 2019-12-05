#ifndef _EVENT_HH_
#define _EVENT_HH_

#include <string>
#include <functional>
#include <deque>

#include "core.hh"
#include "job.hh"

class Event
{
	public:
		enum class EventType {JOB_RECV, COMPUT_COMPLETE, INC_FULL_FREQ,
							  INC_FREQ, DEC_FREQ, SET_FREQ};

		EventType type;

		bool executed;

		// TODO: all flags should be changed to hex as in gem5 later

		Time time;
		// for JOB_RECV, the jobs to be delivered
		// for COMPUT_COMPLETE, the jobs executed
		std::list<Job*> jobList;
		// for COMPUT_COMPLETE, the jobs to be enqueued back to continue execution
		std::list<Job*> pendList;
		/*** for COMPUT_COMPLETE ***/
		// resources
		Core* core;
		unsigned threadId;
		// if job of the event has finished all computation at current service
		bool servCompl;
		// lock
		bool criSec;
		unsigned* thdCnt;
		// if this event should be deleted
		bool del;
		
		// dvfs
		unsigned freq;

	private:
		std::list< std::function<bool (Time)>> callbacks;

	public:
		Event(EventType type);
		void registerCallBack(std::function<bool (Time)> cb);
		void run(Time globalTime);
		std::string present();
};

class EventQueue
{
	protected:
		std::deque<Event*> queue;
		unsigned numEvents;
		void filterUp(unsigned idx);
		void filterDown(unsigned idx);

	public:
		EventQueue(): numEvents(0) {}
		void enqueue(Event* event);
		Event* pop();
		Time nextEventTime();
		void show();
};

#endif