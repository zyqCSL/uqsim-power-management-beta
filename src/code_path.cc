#include "code_path.hh"


/********** CodePath ***********/
CodePath::CodePath(unsigned pathId, unsigned nstg): id(pathId), numStages(nstg) {
	// priority = new unsigned[numStages];
	for(unsigned i = 0; i < numStages; ++i)
		priority.push_back(numStages - i - 1);
	nextStage = 0;
	nextPrioId = 0;
	nonQueStage = nullptr;
	nextStageDecided = false;
}

unsigned
CodePath::addStage(Stage* stage) {
	assert(stages.size() < numStages);
	if(stages.size() > 0)
		stages[stages.size() - 1]->setNextStage(stage);
	stages.push_back(stage);
	return stages.size() - 1;
}

void
CodePath::checkStage() {
	assert(stages.size() == numStages);
}

void
CodePath::setPriority(const std::vector<unsigned>& prio) {
	assert(prio.size() == numStages);
	priority = prio;

	auto itr = priority.begin();
	// guarantee on repitition
	while(itr != priority.end()) {
		assert(std::find(priority.begin(), itr, *itr) == itr);
		++itr;
	}
	// guarantee all stages have priority levels
	for(unsigned i = 0; i < numStages; ++i)
		assert(std::find(priority.begin(), priority.end(), i) != priority.end());
}

Stage*
CodePath::getNextStage() {
	return stages[nextStage];
}

unsigned
CodePath::getNextPathStageId() {
	return nextStage;
}

void
CodePath::incNextStage() {
	assert(nextStage < numStages - 1);
	++nextStage;
}

void
CodePath::setNextStage(unsigned s) {
	assert(s < numStages);
	nextStage = s;
	nextStageDecided = true;
}

void
CodePath::resetNextStage() {
	nextStage = 0;
}

bool
CodePath::isLastStage() {
	return nextStage == numStages - 1;
}

bool
CodePath::isLowestPrio() {
	return nextPrioId == numStages - 1;
}

void
CodePath::resetStagePrio() {
	nextPrioId = 0;
	nextStage = priority[0];
}

void
CodePath::reducePriority() {
	assert(nextPrioId < numStages - 1);
	++nextPrioId;
	nextStage = priority[nextPrioId];
}

Stage* 
CodePath::getStage(unsigned stageId) {
	for(auto stage:stages) 
		if (stage->getStageId() == stageId)
			return stage;
	return nullptr;
}

void
CodePath::reset() {
	nextPrioId = 0;
	nextStage = 0;
}

void
CodePath::setNonQueStage(TimeModel* t) {
	nonQueStage = t;
}

void 
CodePath::incFullFreq() {
	auto itr = stages.begin();
	while(itr != stages.end()) {
		(*itr)->incFullFreq();
		++itr;
	}
}

void 
CodePath::incFreq() {
	auto itr = stages.begin();
	while(itr != stages.end()) {
		(*itr)->incFreq();
		++itr;
	}
}

void 
CodePath::decFreq() {
	auto itr = stages.begin();
	while(itr != stages.end()) {
		(*itr)->decFreq();
		++itr;
	}
}

void
CodePath::setFreq(unsigned freq) {
	// std::cout << "In CodePath::setFreq path id: " << id << std::endl;
	auto itr = stages.begin();
	while(itr != stages.end()) {
		// std::cout << "actualy set" << std::endl;
		(*itr)->setFreq(freq);
		++itr;
	}
}


