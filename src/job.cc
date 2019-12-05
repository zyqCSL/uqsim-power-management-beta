#include <iostream>
#include <stdio.h>
#include <limits.h>

#include <algorithm>

#include "job.hh"

/***************** JobTimeRecords ********************/
JobTimeRecords::JobTimeRecords(unsigned init_size) {
	jobComplTime.reserve(init_size);
}

void
JobTimeRecords::jobCompl(Time time) {
	jobComplTime.push_back(time);
}

Time
JobTimeRecords::getTailLat() {
	if(jobComplTime.size() == 0)
		return INVALID_TIME;
	std::sort(jobComplTime.begin(), jobComplTime.end());
	unsigned pos = unsigned(0.99 *jobComplTime.size());
	return jobComplTime[pos]; 
}

Time
JobTimeRecords::getAvgLat() {
	if(jobComplTime.size() == 0)
		return INVALID_TIME;
	Time total = 0;
	for(Time lat: jobComplTime)
		total += lat;

	return total/jobComplTime.size();
}

void
JobTimeRecords::clear() {
	jobComplTime.clear();
}

/***************** Job ********************/
Job::Job(uint64_t id, unsigned connid, JobTimeRecords* time_records, MicroServPathNode* node, Time initime, Time* ftime, bool* complete, bool debug): 
	finalTime(ftime), complete(complete), pathNode(node), debug(debug), complRecords(time_records), id(id), connId(connid), startTime(initime), time(initime),
	enq_time(initime), newPathNode(true), del(false), tailStage(false), net(false), srcServId(unsigned(-1)), targServId(unsigned(-1)), netSend(false)
{
	// std::cout << "in job constructor id = " << idx << ", ";
	// std::cout << "numServices = " << numServices << std::endl;

	assert(time_records != nullptr);
	assert(complete != nullptr);

	records = new std::unordered_map<unsigned, std::list<JobRecord*>>();
	servMap = new std::unordered_map<std::string, unsigned> ();

	cnt = new int;
	*cnt = 1;

	curRecord = nullptr;
	assert(finalTime != nullptr);
	*finalTime = startTime;

	ngxProc = false;
}


Job::Job(Job* j) {
	// std::cout << "in Job *j copy function ***********" << std::endl;
	numServices = j->numServices;
	records = j->records;
	servMap = j->servMap;

	curRecord = nullptr;
	in_nic = j->in_nic;

	cnt = j->cnt;
	++(*cnt);

	finalTime = j->finalTime;
	complete = j->complete;

	debug = j->debug;

	id = j->id;
	connId = j->connId;

	startTime = j->startTime;
	time = j->time;
	enq_time = j->enq_time;

	net = j->net;

	ngxProc = j->ngxProc;

	// srcSevId targServId should is copied here only for tcp service
	// since tcp does not decide job's next service and just relays the job
	targServId = j->targServId;
	srcServId = j->srcServId;
	netSend = j->netSend;

	// performance monitoring
	complRecords = j->complRecords;
	assert(complRecords != nullptr);

	del = false;
}

Job::~Job() {
	if(time > *finalTime)
		*finalTime = time;
	--(*cnt);
	// std::cout << "job cnt = " << *(cnt) << std::endl;
	if(*cnt == 0) {
		*complete = true;

		// std::cout << "job " << id << " completed\n";

		// insert complete time into records
		assert(complRecords != nullptr);
		complRecords->jobCompl(*finalTime - startTime);

		delete cnt;

		for(auto ele: *records) {
			std::list<JobRecord*>& jrList = ele.second;
			for(auto jr: jrList)
				delete jr;
		}
		delete records;
		delete servMap;
	}
}

/** time keeping **/
Time 
Job::getEnqTime() {
	assert(curRecord != nullptr);
	return curRecord->initTime;
}

/**** intra-micro service and CodePath ****/
unsigned
Job::getThread() {
	assert(curRecord != nullptr);
	return curRecord->threadId;
}

int
Job::getCodePath() {
	assert(curRecord != nullptr);
	// printf("curRecord: servId = %d, codePath = %d, stage = %d, threadAssigned = %d, threadId = %d\n",
	// 	 curRecord->servId, curRecord->codePath, curRecord->stage, curRecord->threadAssigned, curRecord->threadId);
	return curRecord->codePath;
}

void
Job::setCodePath(int code_path) {
	assert(curRecord->codePath == -1);
	curRecord->codePath = code_path;
}

// thread related
bool
Job::threadAssigned() {
	assert(curRecord != nullptr);
	return curRecord->threadAssigned;
}

void 
Job::assignThread(unsigned thread_id) {
	assert(curRecord != nullptr);
	curRecord->threadAssigned = true;
	curRecord->threadId = thread_id;
}

void
Job::blockThread() {
	assert(curRecord != nullptr);
	curRecord->threadBlocked = true;
	if(debug)
		printf("In Job::blockThread job: %lu blocks thread %u\n", id, curRecord->threadId);
}

bool
Job::unblockThread() {
	assert(curRecord != nullptr);
	if(curRecord->threadBlocked) {
		curRecord->threadBlocked = false;
		if(debug)
			printf("In Job::unblockThread job: %lu unblocks thread %u\n", id, curRecord->threadId);
		return true;
	}
	return false;
}

/***** micro service path ******/
std::string 
Job::getServName() {
	return pathNode->getServName();
}

std::string 
Job::getServDomain() {
	return pathNode->getServDomain();
}

void 
Job::setServId(std::string serv_name, std::string serv_func, unsigned servId) {
	// std::pair<std::string, std::string> key = std::pair<std::string, std::string> (serv_name, serv_func);
	std::string key = serv_name + '_' + serv_func;
	// std::cout << "set serv id serv_name = " << serv_name << std::endl;

	auto itr = servMap->find(key);
	assert(itr == servMap->end());
	(*servMap)[key] = servId;
}

bool 
Job::getServId(std::string serv_name, std::string serv_func, unsigned& servId) {
	auto itr = servMap->find(serv_name + '_' + serv_func);
	if(itr == servMap->end())
		return false;
	servId = (*itr).second;
	return true;
}

MicroServPathNode*
Job::getPathNode() {
	return pathNode;
}

void
Job::setPathNode(MicroServPathNode* node) {
	pathNode = node;
}

bool
Job::lastStage() {
	// always need to leave current pathNode if chunk processing is not done
	if(chunkSet())
		return true;

	int endStg = pathNode->getEndStg();
	if(endStg == -1)
		return false;
	else {
		assert(unsigned(endStg) >= curRecord->stage);
		if(unsigned(endStg) == curRecord->stage)
			return true;
		else
			return false;
	}
}

unsigned
Job::getStage() {
	return curRecord->stage;
}

void
Job::setStage(unsigned s) {
	curRecord->stage = s;
}

void
Job::incStage() {
	assert(!tailStage);
	assert(!lastStage());
	curRecord->stage += 1;
	curRecord->stat = JobRecord::State::WAIT_REQ;
}

/*** synchronization & JobRecord ***/
void
Job::createRecord() {
	unsigned pathId = pathNode->getPathId();
	unsigned nodeId = pathNode->getNodeId();
	unsigned syncNum = pathNode->getSyncNum();
	unsigned servId = 0;
	assert(getServId(getServName(), getServDomain(), servId));
	JobRecord* jr = new JobRecord(pathId, nodeId, servId, syncNum, time);
	// if(pathNode->getCodePath() > 0)
	jr->codePath = pathNode->getCodePath();
	jr->stage = pathNode->getStartStg();

	// std::pair<std::string, std::string> servIdent = std::pair<std::string, std::string> (pathNode->getServName(), pathNode->getServDomain());
	std::string servIdent = pathNode->getServName() + '_' + pathNode->getServDomain();
	unsigned key = (*servMap)[servIdent];

	if(records->find(key) == records->end())
		(*records)[key] = std::list<JobRecord*>();
	(*records)[key].push_back(jr);
	curRecord = jr;
}

void
Job::updateRecord(MicroServPathNode* nextNode) {
	assert(nextNode != nullptr);
	assert(curRecord != nullptr);
	// cannot update record when job is leaving current micro service
	// for chunk processing, it is possible that a job leaves a node and returns again even if the endStage is -1
	// (chunking at the first stage represented by the path node)
	if(!chunkSet())
		assert(pathNode->getEndStg() != -1);
	unsigned servId = curRecord->servId;

	assert(!chunkSet());
	// if(!chunkSet()) {
	// 	if(pathNode->needSync()) 
	// 		assert(nextNode->getStartStg() == pathNode->getEndStg());
	// 	else
	// 		assert(nextNode->getStartStg() == pathNode->getEndStg() + 1);
	// } else {
	// 	assert(!chunkDone());
	// 	// here we can not use pathNode->getEndStg() to replace curRecord->stage
	// 	// because chunk recv node's last stage is most probably not the chunk stage
	// 	assert(unsigned(nextNode->getStartStg()) == curRecord->stage);
	// }

	unsigned pathId = nextNode->getPathId();
	unsigned nodeId = nextNode->getNodeId();
	// search if record already exists
	// a record could exist if this nextNode is synchronizing an input from previous stage
	// and input from outside micro services and input from outside micro services arrive first
	JobRecord* newRec = nullptr;

	for(auto record: (*records)[servId]) {
		if(record != curRecord && record->microServPath == pathId && record->pathNode == nodeId) {
			// chunk stage does not synchronize requests except the chunk resp message
			assert(!chunkSet());
			newRec = record;
			assert(curRecord->threadAssigned);
			record->assignThread(curRecord->threadId);
			record->codePath = curRecord->codePath;
			break;
		}
	}

	if(newRec == nullptr) {
		// update curRecord to reflect nextNode
		curRecord->pathNode = nextNode->getNodeId();
		curRecord->syncNum = nextNode->getSyncNum();
		curRecord->stage = nextNode->getStartStg();
	} else {
		// remove curRecord
		auto itr = (*records)[servId].begin();
		bool found = false;
		while(itr != (*records)[servId].end()) {
			if(*itr == curRecord) {
				(*records)[servId].erase(itr);
				found = true;
				break;
			}
			++itr;
		}
		assert(found);
		delete curRecord;
		curRecord = newRec;
	}
}

bool 
Job::matchRecord() {
	// a job's curRecord might be up to date
	if(curRecord != nullptr) {
		if(curRecord->microServPath == pathNode->getPathId() && curRecord->pathNode == pathNode->getNodeId())
			return true;
	}

	// get current service 
	// std::cout << "matchRecord serv_name = " << getServName() << std::endl;

	// std::pair<std::string, std::string> key = std::pair<std::string, std::string> (getServName(), getServDomain());
	std::string key = getServName() + '_' + getServDomain();
	unsigned servId = (*servMap)[key];

	for(auto r: (*records)[servId]) {
		if(r->microServPath == pathNode->getPathId() && r->pathNode == pathNode->getNodeId()) {
			curRecord = r;

			// std::cout << "record match succeeds at " << time << ", service id = " << servId << std::endl;

			return true;
		}
	}

	// std::cout << "record match fails at " << time << std::endl;
	return false;
}

void
Job::clearRecord() {
	// delete corresponding record
	std::list<JobRecord*>& jrList = (*records)[curRecord->servId];
	auto itr = jrList.begin();
	bool suc = false;
	while(itr != jrList.end()) {
		if(*itr == curRecord) {
			jrList.erase(itr);
			delete curRecord;
			curRecord = nullptr;
			suc = true;
			break;
		}
		++itr;
	}
	// if(!pathNode->isTemp())
	assert(suc);
}

void
Job::recv() {
	// std::cout << "In Job::recv syncNum = " << curRecord->syncNum << std::endl;
	if(curRecord->syncNum < 1) {
		std::cout << "curRecord->initTime = " << curRecord->initTime << std::endl;
		std::cout << "curRecord->syncNum = " << curRecord->syncNum << std::endl;
		std::cout << "j->time = " << time << std::endl; 
	}


	assert(curRecord->syncNum >= 1);
	--(curRecord->syncNum);
	assert(curRecord->time <= time);
	curRecord->time = time;
}

void
Job::enterPathNode() {
	assert(newPathNode);
	newPathNode = false;

	bool match = matchRecord();
	if(!match)
		createRecord();

	recv();
}

void
Job::leavePathNode(std::list<Job*>& sendL, std::list<Job*>& pendL, bool& servCompl) {
	servCompl = false;
	std::vector<MicroServPathNode*> childs = pathNode->getChilds();
	if(!chunkSet()) {
		if(pathNode->needSync()) {
			assert(curRecord->stat == JobRecord::State::WAIT_REQ);
			assert(pathNode->getEndStg() != -1);
			MicroServPathNode* syncNode = pathNode->getSyncNode();
			// check already done in path check, just in case
			assert(syncNode->getServName() == getServName());
			assert(syncNode->getServDomain() == getServDomain());
			assert(syncNode->getCodePath() == -1 || syncNode->getCodePath() == getCodePath());
			updateRecord(syncNode);
			// delete current pathNode if it is temporary
			if(pathNode->isTemp()) 
				delete pathNode;

			for(auto node: childs) {
				Job* j = new Job(this);
				j->pathNode = node;
				j->newPathNode = true;
				// only in chunk node we faked, a node that needs sync can have child in the same micro service
				assert( !(node->getServName() == getServName() && node->getServDomain() == getServDomain() ) ); 
				sendL.push_back(j);
			}
			this->del = true;
			// change stat after updateRecord
			curRecord->stat = JobRecord::State::WAIT_RESP;
			if(debug)
				printf("Job: %lu status at service: %u set to WAIT_RESP\n", id, curRecord->servId);
		} else {
			// whether there is a child in the same micro service
			bool childSameServ = false;
			for(auto node: childs) {
				if(node->getServName() == getServName() && node->getServDomain() == getServDomain() ) {
					// it makes no sense to enter the same micro service again immediately after finishing it 
					assert(pathNode->getEndStg() != -1);
					// can at most have one child in the same micro service
					assert(!childSameServ);

					childSameServ = true;
					updateRecord(node);
					this->pathNode = node;
					this->newPathNode = true;
					this->del = false;
					pendL.push_back(this);
				} else {
					Job* j = new Job(this);
					j->pathNode = node;
					j->newPathNode = true;
					sendL.push_back(j);
				}
			}

			if(pathNode->getEndStg() != -1) {
				// will definitely enter the same micro service if job has travelled all stages (no sync needed here)
				assert(childSameServ);
				curRecord->stat = JobRecord::State::WAIT_REQ;
				if(debug)
					printf("Job: %lu status at service: %u set to WAIT_REQ\n", id, curRecord->servId);
			} else {
				// when job finishes last stage of the service and does not need to wait for sync
				// it's safe to delete the record for current stage
				clearRecord();
				servCompl = true;
			}

			// delete current pathNode if it is temporary
			// TODO: also need to delete the record?
			if(pathNode->isTemp()) {
				delete pathNode;
				// delete curRecord;
				// curRecord = nullptr;
			}

			if(!childSameServ)
				this->del = true;
		}
	} else {
		assert(!chunkDone());
		servCompl = false;
		// chunk processing
		// can't split probability space in terms of needSync
		// because chunk send definitely needSync and chunk recv could also need sync (see setChunk function)
		if(waitReq()) {
			assert(pathNode->needSync());
			// chunk send phase
			bool nextNode = false;
			MicroServPathNode* syncNode = pathNode->getSyncNode();
			for(auto node: childs) {
				if(node->getServName() == getServName() && node->getServDomain() == getServDomain() ) {
					// we are chunking on a internal stage here, so there could only be 1 child
					assert(childs.size() == 1);
					// in this case the unique child must also be the node sync waiting for resp
					assert(node == pathNode->getSyncNode());
					this->pathNode = node;
					this->newPathNode = true;
					this->del = false;
					nextNode = true;
					pendL.push_back(this);
				} else {
					Job* j = new Job(this);
					j->pathNode = node;
					j->newPathNode = true;
					sendL.push_back(j);
				}
			}

			updateRecord(syncNode);
			if(!nextNode)
				this->del = true;

			curRecord->stat = JobRecord::State::WAIT_RESP;
			if(debug)
				printf("Job: %lu status at service: %u set to WAIT_RESP\n", id, curRecord->servId);
		} else {
			// chunk recv phase
			// must updateRecord before changing stat since check is done according to stat in updateRecord
			if(debug) 
				printf("Job: %lu status at service: %u currently in chunk recv phase (about to leave)\n", id, curRecord->servId);

			bool sameServ = false;
			MicroServPathNode* sendNode = curRecord->chunkSendNode;
			if(sendNode->getServName () == getServName() && sendNode->getServDomain() == getServDomain()) 
				sameServ = true;

			// std::cout << "in chunk recv phase sendNode: " << sendNode->getServName() << ", func: " << sendNode->getServDomain() << std::endl;

			updateRecord(curRecord->chunkSendNode);
			this->pathNode = sendNode;
			this->newPathNode = true;
			this->del = false;

			if(sameServ)
				pendL.push_back(this);
			else
				sendL.push_back(this);
			// switch to chunk send node, waiting for request
			curRecord->stat = JobRecord::State::WAIT_REQ;
			if(debug)
				printf("Job: %lu status at service: %u set to WAIT_REQ\n", id, curRecord->servId);
		}
	}
}

bool
Job::syncDone() {
	return(curRecord->syncNum == 0);
}

bool
Job::needSync() {
	assert(curRecord != nullptr);
	if(pathNode->getEndStg() >= 0 && unsigned(curRecord->stage) == unsigned(pathNode->getEndStg()) && pathNode->needSync())
		return true;
	else
		return false;
}

bool
Job::waitReq() {
	assert(curRecord != nullptr);
	return (curRecord->stat == JobRecord::State::WAIT_REQ);
}

bool
Job::waitResp() {
	assert(curRecord != nullptr);
	return (curRecord->stat == JobRecord::State::WAIT_RESP);
}

// chunk processing
bool
Job::setChunk(unsigned chkNum) {
	assert(!curRecord->chunk);
	curRecord->chunk = true;
	assert(curRecord->stat == JobRecord::State::WAIT_REQ);
	curRecord->chunkNum = chkNum;
	// assert(pathNode->getEndStg() != -1);
	MicroServPathNode* node = nullptr;
	bool chunkInternal = false;
	
	if(pathNode->needSync() && pathNode->getEndStg() >= 0 && curRecord->stage == unsigned(pathNode->getEndStg()) ) {
		// chunking on a outside micro-service
		// create a fake pathNode as chunkSendNode whose StartStg and EndStg both equal the endStg (chunk send stg) of current node
		node = new MicroServPathNode(pathNode->getServName(), pathNode->getServDomain(),
			pathNode->getCodePath(), pathNode->getEndStg(), pathNode->getEndStg(), 
			pathNode->getPathId(), pathNode->getNodeId(), true, pathNode->getSyncNodeId());

		node->setSyncNode(pathNode->getSyncNode());
		node->setChilds(pathNode->getChilds());
		// only need to sync one resp from chunk recv node
		node->setSyncNum(1);
		// mark node as temporary
		node->setTemp();
	} else {
		chunkInternal = true;
		// chop up current pathNode to two parts, which are [chunk stage, chunk stage] and [chunk stage, end stage]
		// chunkSendNode: create a fake pathNode whose StartStg and EndStg both equal the current stage
		// first half of current node excluding non-chunk stages already traversed
		// fake a node id for the chunk stage node and make it sync on the original node id
		node = new MicroServPathNode(pathNode->getServName(), pathNode->getServDomain(),
			pathNode->getCodePath(), curRecord->stage, curRecord->stage, 
			pathNode->getPathId(), std::numeric_limits<unsigned>::max()-pathNode->getNodeId(), 
			true, pathNode->getNodeId());

		// second half of current node
		MicroServPathNode* newChild = new MicroServPathNode(pathNode->getServName(), pathNode->getServDomain(),
			pathNode->getCodePath(), curRecord->stage, pathNode->getEndStg(), 
			pathNode->getPathId(), pathNode->getNodeId(), pathNode->needSync(), pathNode->getSyncNodeId());

		// newChild's children are current node's children
		newChild->setChilds(pathNode->getChilds());
		newChild->setSyncNum(1);
		if(pathNode->needSync())
			newChild->setSyncNode(pathNode->getSyncNode());
		// mark as temporary
		newChild->setTemp();

		// chunk stage node's child is newChild
		std::vector<MicroServPathNode*> c;
		c.push_back(newChild);
		node->setChilds(c);
		node->setSyncNode(newChild);
		node->setSyncNum(1);
		// mark node as temporary
		node->setTemp();
		pathNode = node;
	}

	if(debug) 
		printf("Job: %lu set chunk to %u, chunkInternal = %d\n", id, chkNum, chunkInternal);

	curRecord->chunkSendNode = node;
	return chunkInternal;
}

bool 
Job::chunkInternal() {
	assert(chunkSet());
	assert(curRecord->chunkSendNode != nullptr);
	unsigned numChilds = 0;
	bool internal = false;
	MicroServPathNode* sendNode = curRecord->chunkSendNode;
	for(auto child: sendNode->getChilds()) {
		++numChilds;
		if(child->getServName() == sendNode->getServName() && child->getServDomain() == sendNode->getServDomain()) {
			internal = true;
			assert(numChilds == 1);
		}
	}
	return internal;
}

bool
Job::chunkDone() {
	assert(curRecord->chunk);
	return (curRecord->chunkNum == 0);
}

void 
Job::decChunkNum() {
	assert(curRecord->chunk);
	assert(curRecord->chunkNum > 0);
	--(curRecord->chunkNum);
	if(debug)
		printf("job: %lu has %u chunks left\n", id, curRecord->chunkNum);
}

bool
Job::chunkSet() {
	return (curRecord->chunk);
}

unsigned
Job::getChunkNum() {
	assert(curRecord->chunk);
	return (curRecord->chunkNum);
}

void
Job::clearChunk() {
	assert(curRecord->chunk);
	assert(curRecord->chunkNum == 0);
	curRecord->chunk = false;
	assert(curRecord->chunkSendNode->isTemp());
	delete curRecord->chunkSendNode;
	curRecord->chunkSendNode = nullptr;
	if(debug)
		printf("Job: %lu clearChunk() executed\n", id);
}

/***************** JobRecord ********************/
JobRecord::JobRecord(unsigned path_id, unsigned node_id, unsigned serv_id, unsigned sync_num, Time init_time): 
	microServPath(path_id), pathNode(node_id), syncNum(sync_num), threadAssigned(false), threadId(0),
	servId(serv_id), codePath(0), stage(0), initTime(init_time), time(0), 
	chunk(false), chunkNum(0), chunkSendNode(nullptr), threadBlocked(false) {
	stat = State::WAIT_REQ;
}

void 
JobRecord::assignThread(unsigned thread_id) {
	threadAssigned = true;
	threadId = thread_id;
}


