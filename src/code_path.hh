#ifndef _PATH_HH_
#define _PATH_HH_

#include <vector>
#include <algorithm>

#include "stage.hh"

class CodePath
{
	private:
		// path id
		unsigned id;
		std::vector<unsigned> priority;
		// next stage's idx in priority array
		unsigned nextPrioId;
		// next stage's index
		unsigned nextStage;

	public:
		bool nextStageDecided;

	public:
		// # stages in this path
		unsigned numStages;
		std::vector<Stage*> stages;

		// if there is a non-queueing stage at the head of the path
		TimeModel* nonQueStage;

		CodePath(unsigned pathId, unsigned nstg);
		// since the pointers in CodePath object is shared, only delete then once in the MicroService
		virtual ~CodePath() {}

		// here stageId refers to the stage idx in the micro-service
		// there could be multiple stages in different path (one in each path)
		// that corresponds to the same stage in micro-service
		unsigned addStage(Stage* stage);
		void checkStage();

		void setPriority(const std::vector<unsigned>& prio);

		// nextStage to execute
		Stage* getNextStage();
		unsigned getNextPathStageId();
		// increment next stage to the next path stage
		void incNextStage();
		void setNextStage(unsigned s);
		// resest next stage to 0
		void resetNextStage();
		// if curstage is the last pathStage in the path
		bool isLastStage();

		// priority
		// if this path has reached lowest priority level
		bool isLowestPrio();
		// rest next stage to highest priority stage
		void resetStagePrio();
		// reduce nxtPrioStgId to next lower priority level
		void reducePriority();
		// get stage by idx in micro-service (not idx in path or pathId)
		Stage* getStage(unsigned stageId);

		void reset();

		void setNonQueStage(TimeModel *t);

		// dvfs
		void incFullFreq();
		void incFreq();
		void decFreq();
		void setFreq(unsigned freq);
};

#endif