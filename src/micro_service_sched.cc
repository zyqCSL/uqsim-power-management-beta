#include "micro_service_sched.hh"
#include <iostream>
#include <stdio.h>

/************** MicroServScheduler ******************/
MicroServScheduler::MicroServScheduler(unsigned servid, const std::vector<CodePath>& path, 
		std::unordered_map<unsigned, unsigned*> cri_sec, bool bindconn, unsigned num_threads,
		std::vector<Core*> core_list, bool debug): servId(servid), bindConn(bindconn), debug(debug) {
	paths = path;
	criSect = cri_sec;
	corePool = core_list;
}

void
MicroServScheduler::assignCore(Core* c) {
	corePool.push_back(c);
}

void
MicroServScheduler::incFullFreq() {
	auto itr = threadPool.begin();
	while(itr != threadPool.end()) {
		(*itr)->incFullFreq();
		++itr;
	}
}

void
MicroServScheduler::incFreq() {
	auto itr = threadPool.begin();
	while(itr != threadPool.end()) {
		(*itr)->incFreq();
		++itr;
	}
}

void
MicroServScheduler::decFreq() {
	auto itr = threadPool.begin();
	while(itr != threadPool.end()) {
		(*itr)->decFreq();
		++itr;
	}
}

void
MicroServScheduler::setFreq(Time freq) {
	// std::cout << "in MicroServScheduler::setFreq" << std::endl; 
	auto itr = threadPool.begin();
	while(itr != threadPool.end()) {
		(*itr)->setFreq(freq);
		++itr;
	}
}

double
MicroServScheduler::getCpuUtil(Time elapsed_time) {
	if(elapsed_time == 0)
		return 0;
	else {
		Time total_used = 0;
		for(Core* c: corePool)
			total_used += c->total_time;
		return (double)total_used/(double)(corePool.size() * elapsed_time);
	}
}

/************** CMTScheduler ******************/
CMTScheduler::CMTScheduler(unsigned servid, const std::vector<CodePath>& path, 
		std::unordered_map<unsigned, unsigned*> cri_sec, bool bindconn, unsigned num_threads,
		std::vector<Core*> core_list, bool debug):
		MicroServScheduler(servid, path, cri_sec, bindconn, num_threads, core_list, debug) {
	threadId = 0;
	// printf("In service %d, there are %d threads and %d cores\n", servid, numThreads, numCores);
	for(unsigned i = 0; i < num_threads; ++i) {
		// change constructor if thread has multiple jobs in it
		// Thread* t = new Thread(i, service->numStages, service->stages);
		Thread* t = new Thread(i, bindConn, paths, debug);
		threadPool.push_back(t);
	}
	corePool = core_list;
	// std::cout << "service " << servId << " get Cores: ";
	// for(auto core: corePool)
	// 	std::cout << core->getId() << " ";
	// std::cout << std::endl;
}

CMTScheduler::~CMTScheduler() {
	for(auto itr: threadPool)
		delete itr;
}

void
CMTScheduler::assignThread() {
	Thread* t = new Thread(threadPool.size(), bindConn, paths, debug);
	threadPool.push_back(t);
}

void 
CMTScheduler::eventHandler(Event* event, Time globalTime, std::list<Event*>& eventList) {
	if(event->type == Event::EventType::JOB_RECV) {
		for(auto job: event->jobList) {
			// std::cout << "IN cmt event handler jid = " << job->id << std::endl;
			dispatch(job);
			schedule(globalTime, eventList);
		}
		// delete event;
		event->del = true;
	} else if(event->type == Event::EventType::COMPUT_COMPLETE) {
		// unsigned cid = event->coreId;
		unsigned tid = event->threadId;

		Thread* thread = threadPool[tid];
		for(auto job: event->pendList) {

			if(tid != job->getThread()) {
				std::cout << "tid = " << tid << std::endl;
				std::cout << "job->getThread() = " << job->getThread() << std::endl;
			}

			assert(tid == job->getThread());
			thread->enqueue(job);
		}

		// clear event's job records
		event->pendList.clear();
		event->del = false;
		// event->jobList.clear() done in microservice's event handler

		// always need to release the lock held by this event first
		if(event->criSec) {
			assert(event->thdCnt != nullptr);
			*(event->thdCnt) += 1;
		}

		// check if this thread can continue execution
		bool runnable = true;
		bool lockAvail = true;
		bool jobInNextStage = true;
		if(!thread->isBlocked()) {
			// schedule the thread
			bool suc = thread->schedule();
			if(suc) {
				unsigned nextStageId = thread->getNextStageId();
				auto criItr = criSect.find(nextStageId);
				if(criItr != criSect.end()) {
					unsigned* lock = (*criItr).second;
					assert(*lock >= 0);
					lockAvail = (*lock > 0);
				}

				jobInNextStage = thread->jobInNextStage();
				if(!event->servCompl && jobInNextStage && lockAvail) {
					// the thread can continue executing
					if(debug)
						printf("In CMTScheduler::eventHandler, thread: %d continues to run on core: %d at %lu\n", 
							thread->getId(), event->core->getId(), globalTime);
					
					thread->run(globalTime, event);
					eventList.push_back(event);
				} else
					runnable = false;
			} else {
				runnable = false;
				if(debug)
					printf("Original thread scheduling fails\n");
			}
		} else
			runnable = false;

		if(!runnable) {
			if(debug) {
				printf("thread->isBlocked() = %d, event->servCompl = %d, thread->jobInNextStage() = %d, lockAvail = %d\n",
					thread->isBlocked(), event->servCompl, jobInNextStage, lockAvail);
				printf("In CMTScheduler::eventHandler, thread: %d, core: %d is descheduled at %lu\n", 
					thread->getId(),  event->core->getId(), globalTime);
			}
				
			releaseRsc(event);
			// delete event;
			event->del = true;
			schedule(globalTime, eventList);
		}
	} else {
		printf("Error: Invalid Event Type\n");
		exit(1);
	}

	// event delete is done in cluster
}

bool
CMTScheduler::dispatch(Job* j) {
	assert(j->syncDone());
	Thread* thread = nullptr;
	if(j->threadAssigned()) {
		unsigned tid = j->getThread();
		thread = threadPool[tid];
	} else {
		// std::cout << "j->connIdx = " << j->connIdx << std::endl;
		if(bindConn) {
			unsigned conn = j->connId;
			for(auto t: threadPool) {
				if(t->hasConn(conn)) {
					thread = t;
					break;
				}
			}
		}

		if(thread == nullptr) {
			// std::cout << "threadId = " << threadId << std::endl;
			// std::cout << "threadPool.size() = " << threadPool.size() << std::endl;
			thread = threadPool[threadId];
			threadId = (threadId + 1) % threadPool.size();
		}
		j->assignThread(thread->getId());
	}

	thread->enqueue(j);
	return true;
}

void
CMTScheduler::schedule(Time globalTime, std::list<Event*>& eventList) {
	// try to schedule as many threads as possible
	for(auto thread: threadPool) {
		if(!thread->isReady())
			continue;

		bool suc = thread->schedule();
		if(!suc)
			continue;

		Core* core = nullptr;
		if(coreAffinity.find(thread->getId()) != coreAffinity.end()) {
			// first check core mask
			for(auto c: coreAffinity[thread->getId()]) {
				if(c->isAvail()) {
					core = c;
					break;
				}
			}
		} else {
			// check scheduler's core pool
			for(auto c: corePool) {
				if(c->isAvail()) {
					core = c;
					break;
				}
			}
		}

		if(core == nullptr)
			continue;
		// the actual stageId (not pathStageId)
		// std::cout << "In sched: schedule" << std::endl;
		unsigned nextStageId = thread->getNextStageId();
		auto criItr = criSect.find(nextStageId);
		if(criItr != criSect.end()) {
			// lock stage, check availability
			unsigned* lock = (*criItr).second;
			assert(*lock >= 0);
			if(*lock == 0)
				continue;
		}

		if(debug) 
			printf("In CMTScheduler::schedule thread: %d on core: %d at time %lu ns\n", thread->getId(), core->getId(), globalTime);
		Event* event = new Event(Event::EventType::COMPUT_COMPLETE);
		event->core = core;
		event->threadId = thread->getId();
		core->setRunning();
		thread->run(globalTime, event);
		eventList.push_back(event);
	}
}

void
CMTScheduler::releaseRsc(Event* event) {
	// lock relase is not done here since the lock always needs releasing
	// whether the job has finished all computations at the micro service
	if(debug)
		printf("Released core %d\n", event->core->getId());
	event->core->setIdle();
	if(!threadPool[event->threadId]->isBlocked())
		threadPool[event->threadId]->unsetRunning();

	// std::cout << "release thread " << comp->threadId << std::endl;
	// std::cout << "release core " << comp->coreId << std::endl;
}

bool
CMTScheduler::jobLeft() {
	for(auto thread: threadPool) {
		if(thread->jobLeft()) 
			return true;
	}

	return false;
}

void 
CMTScheduler::setCoreAffinity(unsigned tid, std::vector<Core*> cores) {
	coreAffinity[tid] = cores;
}

/************** SimpScheduler ******************/
SimpScheduler::SimpScheduler(unsigned servid, const std::vector<CodePath>& path, bool bindconn, unsigned num_que,
	std::vector<Core*> core_list, bool debug): 
	MicroServScheduler(servid, path, std::unordered_map<unsigned, unsigned*>(), bindconn, 0, core_list, debug) {
	nextQue = 0;
	queues = std::vector< std::deque<Job*> >(num_que);
}

void
SimpScheduler::assignThread() {
	printf("Cannot assign thread to SimpScheduler\n");
	exit(-1);
}

void 
SimpScheduler::eventHandler(Event* event, Time globalTime, std::list<Event*>& eventList) {
	if(event->type == Event::EventType::JOB_RECV) {
		for(auto job: event->jobList) {
			// std::cout << "In SimpScheduler event handler JOB_RECV jid = " << job->id << std::endl;
			dispatch(job);
			schedule(globalTime, eventList);
		}
		// delete event;
		event->del = true;
	} else if(event->type == Event::EventType::COMPUT_COMPLETE) {

		if(debug)
			printf("In SimpScheduler::eventHandler core: %d is descheduled at %lu\n", event->core->getId(), globalTime);
		releaseRsc(event);
		// event->del = true set in schedule
		schedule(globalTime, eventList);
	} else {
		printf("Error: Invalid Event Type\n");
		exit(1);
	}
	// event delete is done in cluster
	// std::cout << "eventHandler done in SimpScheduler" << std::endl;
}

bool 
SimpScheduler::dispatch(Job* j) {
	assert(j != nullptr);
	auto& queue = queues[nextQue];
	queue.push_back(j);

	nextQue = (nextQue + 1) % queues.size();
	// std::cout << "dispatch in SimpScheduler done" << std::endl;
	return true;
}

void 
SimpScheduler::schedule(Time globalTime, std::list<Event*>& eventList) {
	// std::cout << "In SimpScheduler::schedule" << std::endl;
	for(unsigned i = 0; i < queues.size(); ++i) {
		if(queues[i].empty())
			continue;
		Core* core = nullptr;
		if(coreAffinity.find(i) != coreAffinity.end()) {
			// std::cout << "que " << i <<  " in core aff list" << std::endl;
			for(auto c: coreAffinity[i]) {
				// std::cout << "searching core aff list" << std::endl;
				if(c->isAvail()) {
					core = c;
					break;
				}
			}
		} else {
			// std::cout << "que " << i << " not in core aff list" << std::endl;
			for(auto c: corePool) {
				if(c->isAvail()) {
					// std::cout << "searching corePool" << std::endl;
					core = c;
					break;
				}
			}
		}

		if(core == nullptr) {
			if(debug)
				printf("In SimpScheduler::schedule queue: %d not scheduled for no usable core\n", i);
			continue;
		}

		// std::cout << "In TCP, schedules queue: " << i << "at time " << globalTime << "on core " << core->getId() << std::endl;

		Job* j = *(queues[i].begin());
		queues[i].pop_front();
		unsigned pid = 0;
		// std::cout << "in_nic = " << j->in_nic << std::endl;
		if(j->in_nic)
			pid = unsigned(j->net_code_path);
		else
			pid = j->getCodePath();
		Time execTime = globalTime;

		// double enqueTime = j->time;

		// std::cout << "target path id = " << pid << std::endl;
		// std::cout << "paths.size() = " << paths.size() << std::endl;
		// std::cout << "numStages = " << paths[pid].numStages << std::endl;
		for(unsigned i = 0 ; i < paths[pid].numStages; ++i) {
			// std::cout << "i = " << i << std::endl;
			paths[pid].stages[i]->run(execTime, j, INVALID_TIME);
			execTime = j->time;
		}

		// std::cout << "one stage, j queue + proc time = " << j->time - enqueTime << std::endl;

		if(debug) 
			printf("In SimpScheduler::schedule queue: %d on core: %d at time %lu ns\n", i, core->getId(), globalTime);
		Event* event = new Event(Event::EventType::COMPUT_COMPLETE);
		event->core = core;
		// do all processing of the event
		event->del = true;
		event->criSec = false;
		event->thdCnt = nullptr;
		// we don't need threadId in SimpScheduler
		event->threadId = 0;
		event->servCompl = true;
		event->jobList.push_back(j);
		event->time = j->time;
		core->setRunning();
		eventList.push_back(event);


		// std::cout << "TCP processing done " << std::endl;
	}
}

bool 
SimpScheduler::jobLeft() {
	for(unsigned i = 0; i < queues.size(); ++i) {
		if(!queues[i].empty())
			return true;
	}
	return false;
}

void 
SimpScheduler::setCoreAffinity(unsigned qid, std::vector<Core*> cores) {
	coreAffinity[qid] = cores;
}

void
SimpScheduler::releaseRsc(Event* event) {
	if(debug)
		printf("Released core %d\n", event->core->getId());
	event->core->setIdle();
}


/************** LinuxNetScheduler ******************/
LinuxNetScheduler::LinuxNetScheduler(unsigned servid, const std::vector<CodePath>& path, bool bindconn, unsigned num_que,
	std::vector<Core*> core_list, bool debug): 
	SimpScheduler(servid, path, bindconn, 2*num_que, core_list, debug) {
	numBiDirQueues = num_que;
}

bool 
LinuxNetScheduler::dispatch(Job* j) {
	// place tx and rx jobs on seprate queues
	auto& queue = (j->netSend) ? queues[nextQue + numBiDirQueues] : queues[nextQue];
	// insert req jobs from back of the queue
	// the thread is not blocked, put ready jobs onto queue
	queue.push_back(j);
	nextQue = (nextQue + 1) % numBiDirQueues;
	// std::cout << "dispatch done in LinuxNetScheduler" << std::endl;
	return true;
}

void
LinuxNetScheduler::setCoreAffinity(unsigned qid, std::vector<Core*> cores) {
	// std::cout << "set coreAffinity for queue " << qid << " & " << qid+numBiDirQueues << std::endl;
	// std::cout << "cores.size() = " << cores.size() << std::endl;
	coreAffinity[qid] = cores;
	coreAffinity[qid+numBiDirQueues] = cores;
}