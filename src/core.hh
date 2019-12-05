#ifndef __CORE_HH__
#define __CORE_HH__
#include "util.hh"

// only indicates avail at this moment
// leave space for future expansion
class Core
{
	public:
		enum State {IDLE, RUNNING};
		Time total_time;
	private:
		unsigned id;
		State stat;
	public:
		Core(unsigned id);

		bool isAvail();
		void setIdle();
		void setRunning();
		unsigned getId();
};

#endif