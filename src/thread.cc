#include "thread.hh"
#include <stdio.h>
#include <iostream>

Thread::Thread(unsigned id, bool bindcon, const std::vector<CodePath>& path, bool debug):
			threadId(id), bindConn(bindcon),schedSuc(false), debug(debug) {
	paths = path;
	for(auto& p: paths)
		p.reset();

	// create a queue for every stage in every path
	queues = new std::list<Job*>* [paths.size()];

	for(unsigned i = 0; i < paths.size(); ++i) {
		// for each path
		queues[i] = new std::list<Job*> [paths[i].numStages];
	}

	state = READY;
	nextPath = 0;
}

Thread::~Thread() {
	for(unsigned i = 0; i < paths.size(); ++i)
		delete[] queues[i];
	delete[] queues;
}

unsigned
Thread::getId() {
	return threadId;
}

bool
Thread::hasConn(unsigned conn) {
	return !(connections.find(conn) == connections.end());
}

void 
Thread::removeConn(unsigned conn) {
	connections.erase(conn);
}

void
Thread::enqueue(Job* j) {
	if(debug)
		printf("In thread: %d enqueue job: %lu into code path: %d, stage: %d\n", threadId, j->id, j->getCodePath(), j->getStage());

	assert(j->getThread() == threadId);
	assert(j->syncDone());
	if(bindConn)
		connections.insert(j->connId);

	unsigned path = j->getCodePath();
	unsigned stage = j->getStage();

	// unblock the thread
	bool unblk = j->unblockThread();
	if(unblk) {
		if(debug)
			std::cout << "thread " << threadId << " unblocked by " << j->id << std::endl;
		if(!j->chunkSet())
			assert(j->waitResp());
		// At this moment, batching does not support blocking
		assert(!paths[path].stages[stage]->isBatching());
		unblock();
		paths[path].setNextStage(stage);
		if(debug)
			printf("In Thread::enqueue, thread: %d unblocks\n", threadId);	
	}

	auto& queue = queues[path][stage];

	// std::cout << "in Thread::enqueue(), queue size = " << queue.size() << std::endl;
	unsigned offset = 0;

	if(j->waitResp()) {
		// std::cout << "waitResp" << std::endl;

		// insert response jobs into head of the queue
		if(queue.empty())
			queue.push_back(j);
		else {
			auto itr = queue.begin();
			while(itr != queue.end()) {
				if( (*itr)->waitReq() || ((*itr)->waitResp() && (*itr)->time > j->time) )
					break;
				++itr;

				++offset;
			}
			queue.insert(itr, j);
		}
	} else {

		// std::cout << "waitReq" << std::endl;

		// insert req jobs from back of the queue
		// the thread is not blocked, put ready jobs onto queue
		auto itr = queue.end();
		if(itr == queue.begin())
			queue.push_back(j);
		else {
			--itr;
			bool suc = false;
			while(itr != queue.begin()) {
				if( (*itr)->waitResp() || ((*itr)->waitReq() && (*itr)->time < j->time) ) {
					++itr;
					queue.insert(itr, j);
					suc = true;
					break;
				}
				--itr;

				++offset;
			}

			if(!suc) {
				if( (*itr)->waitReq() && (*itr)->time > j->time )
					queue.insert(itr, j);
				else {
					++itr;
					queue.insert(itr, j);
				}
			}
		}
	}

	// std::cout << "offset = " << offset << std::endl << std::endl;
}

void
Thread::run(Time globalTime, Event* event) {
	assert(schedSuc);
	assert(paths[nextPath].nextStageDecided);
	// std::cout << "in thread before run stage" << std::endl;
	unsigned nextStage = paths[nextPath].getNextPathStageId();
	Stage* stage = paths[nextPath].stages[nextStage];
	// perhaps the thdNum alreay reached limit
	// assert(stage->criSecAvail());
	if(stage->isCriSec()) {
		event->criSec = true;
		event->thdCnt = stage->getThdCnt();
	}

	Time proc_time = INVALID_TIME;
	unsigned conn = 0;
	// decide next stage of the path
	bool updStg = true;
	bool rstStg = false;
	bool thdBlk = false;

	// count number of batched jobs for all code paths
	// in epoll, effectively it's counting the number of active connections
	unsigned batchedJobs = 0;
	conn = (*queues[nextPath][nextStage].begin())->connId;
	if(stage->isBatching()) {
		unsigned stageId = stage->getStageId();

		for(unsigned pathId = 0; pathId < paths.size(); ++pathId) {
			CodePath& path = paths[pathId];
			Stage* s = path.getStage(stageId);
			if(s == nullptr)
				continue;
			unsigned pstgId = s->getPathStageId();

			if(s->isSocket()) {
				assert(stage->isSocket());
				batchedJobs += jobNumByConn(pathId, pstgId, conn);
			} else if(s->isEpoll()) {
				assert(stage->isEpoll());
				batchedJobs += numConn(pathId, pstgId);
			} else {
				assert(!stage->isSocket());
				batchedJobs += queues[pathId][pstgId].size();
			}
		}
		assert(batchedJobs > 0);
		// decide processing time
		proc_time = stage->getProcTime(batchedJobs, false);

		// if(stage->isEpoll()) {
			// std::cout << "epoll events = " << batchedJobs << std::endl;
			// std::cout << "proc_time = " << proc_time << std::endl;
		// }
	}

	if(!stage->isBatching()) {
		// for non-batching stage
		Job* j = getNextJob(nextPath, nextStage);
		assert(j != nullptr);
		if(stage->isNgxProc()) {
			proc_time = stage->getProcTime(0, j->ngxProc);
			proc_time = stage->run(globalTime, j, proc_time);
		} else
			proc_time = stage->run(globalTime, j, INVALID_TIME);
		assert(proc_time != INVALID_TIME);

		// std::cout << "proc stage time = "

		if(j->chunkSet()) {
			// with chunk set, the path always needs to wait in the current stage
			// no matter if it's chunking on outside service or internally
			updStg = false;
			thdBlk = stage->isBlocking();
		} else if(j->needSync()) {
			if(stage->isBlocking()) {
				updStg = false;
				thdBlk = true;
			} else
				rstStg = true;
		}
		event->jobList.push_back(j);
	} else {
		// for batching stage
		bool decided = false;
		bool need_sync = false;
		if(debug)
			printf("In Thread::run thread: %d, current path batching starts\n", threadId);
		if(!stage->isSocket() && !stage->isEpoll()) {
			// non-socket
			while(true) {
				Job* j = getNextJob(nextPath, nextStage);
				if(j == nullptr)
					break;
				else {
					stage->run(globalTime, j, proc_time);

					event->jobList.push_back(j);
					if(!decided) {
						decided = true;
						if(j->needSync()) {
							// assume batching stage can't be blocking
							need_sync = true;
							rstStg = true;
						}
					} else 
						assert(need_sync == j->needSync());
				}
			}
		} else {
			// socket & epoll stage
			std::list<Job*> jList;
			// for epoll stage, return the first job in each active connection
			if(stage->isEpoll())
				getJobPerConn(nextPath, nextStage, jList);
			else 
				// conn = (*queues[nextPath][nextStage].begin())->connId;
				getJobByConn(nextPath, nextStage, conn, jList);

			for(auto j: jList) {
				stage->run(globalTime, j, proc_time);

				event->jobList.push_back(j);
				if(!decided) {
					decided = true;
					if(j->needSync()) {
						need_sync = true;
						rstStg = true;
					}
				} else 
					assert(need_sync == j->needSync());
			}
		}
		if(debug)
			printf("In Thread::run thread: %d, current path batching ends\n", threadId);
	}

	if(thdBlk) {
		block();
		assert(event->jobList.size() == 1);
		Job* j = *(event->jobList.begin());
		j->blockThread();
		// std::cout << "thread " << threadId << " blocked by " << j->id << std::endl;
		if(debug)
			printf("In Thread::run thread: %d is blocked\n", threadId);
	} else if(state != RUNNING) 
		setRunning();

	if(updStg) {
		assert(!thdBlk);
		setNextStage(nextPath, rstStg);
	}

	// for batch stage, execute the same batch stage in other paths
	// the same stage is assigned the same stageId (but can have different pathStageId)

	if(stage->isBatching()) {
		unsigned stageId = stage->getStageId();
		if(debug)
			printf("In Thread::run, thread: %d run other batch stage: %d (stageId)\n", threadId, stageId);

		for(unsigned pathId = 0; pathId < paths.size(); ++pathId) {
			// no need to setNextStage here since we should execute as long as there are jobs in front of the batching stage
			if(pathId == nextPath)
				continue;
			CodePath& path = paths[pathId];
			Stage* s = path.getStage(stageId);
			if(s == nullptr)
				continue;
			else {
				unsigned pstgId = s->getPathStageId();
				if(queues[pathId][pstgId].empty())
					continue;

				updStg = false;
				rstStg = false;
				bool decided = false;
				bool need_sync = false;
				if(!s->isSocket() && !stage->isEpoll()) {
					while(true) {
						Job* j = getNextJob(pathId, pstgId);
						if(j == nullptr)
							break;
						s->run(globalTime, j, proc_time);

						event->jobList.push_back(j);
						if(!decided) {
							decided = true;
							updStg = true;
							if(j->needSync()) {
								need_sync = true;
								rstStg = true;
							}
						} else 
							assert(need_sync == j->needSync());
					}
				} else {
					std::list<Job*> jList;
					if(stage->isSocket())
						getJobByConn(pathId, pstgId, conn, jList);
					else
						getJobPerConn(pathId, pstgId, jList);

					for(auto j: jList) {
						proc_time = s->run(globalTime, j, proc_time);

						event->jobList.push_back(j);
						if(!decided) {
							decided = true;
							updStg = true;
							if(j->needSync()) {
								need_sync = true;
								rstStg = true;
							}
						} else 
							assert(need_sync == j->needSync());
					}
				}

				// update next stage only when needs to update and path's next stage is exactly the batching stage executed
				if(path.getNextStage()->getPathStageId() == pstgId && updStg)
					setNextStage(pathId, rstStg);
			}
		}
	}

	// set schedSuc false for next round of sche/run
	schedSuc = false;

	// update event time
	event->time = globalTime + proc_time;
	event->core->total_time += proc_time;

	if(debug)
		printf("In Thread::run thread: %d run complete\n\n", threadId);
}

void
Thread::setRunning() {
	assert(state != RUNNING);
	state = RUNNING;
}

void
Thread::unsetRunning() {
	assert(state == RUNNING);
	state = READY;
}

bool
Thread::isRunning() {
	return (state == RUNNING);
}

void
Thread::block() {
	assert(state != BLOCKED);
	state = BLOCKED;
}

void
Thread::unblock() {
	assert(state == BLOCKED);
	state = READY;
}

bool
Thread::isBlocked() {
	return (state == BLOCKED);
}

bool
Thread::isReady() {
	return(state == READY);
}

unsigned
Thread::getNextStageId() {
	assert(schedSuc);
	return paths[nextPath].getNextStage()->getStageId();
}

bool
Thread::jobInNextStage() {
	assert(paths[nextPath].nextStageDecided);
	unsigned nextStage = paths[nextPath].getNextPathStageId();
	return !queues[nextPath][nextStage].empty();
}

bool
Thread::jobLeft() {
	bool deadlock = false;
	for(unsigned path = 0; path < paths.size(); ++path) {
		if(debug)
			printf("Thread: %d, path %d, nextStage %d\n", threadId, path, paths[path].getNextPathStageId());
		for(unsigned stage = 0; stage < paths[path].numStages; ++stage) {
			if(!queues[path][stage].empty()) {
				auto itr = queues[path][stage].begin();
				while(itr != queues[path][stage].end()) {
					Job* j = *itr;
					if(debug) {
						printf("Thread: %d, Job %lu, connId: %u, left at codePath: %d, stage %d, stageName: %s\n", threadId, j->id, j->connId, path, stage, paths[path].stages[stage]->getStageName().c_str());
						printf("thread->blocked = %d\n", isBlocked());
					}
					++itr;
				}
				deadlock = true;
			}

			// Stage* s = paths[path].stages[stage];
			// if(s->isNgxProc()) {
			// 	for(unsigned i = 0; i < s->numConn; ++i) {
			// 		if(s->connStates[i].valid && debug) {
			// 			printf("conn: %u, blocked: %d, jid: %lu\n", s->connStates[i].connId, 
			// 				s->connStates[i].blocked, s->connStates[i].jid);
			// 		}
			// 	}
			// }
		}
	}
	return deadlock;
}

void
Thread::incFullFreq() {
	auto itr = paths.begin();
	while(itr != paths.end()) {
		(*itr).incFullFreq();
		++itr;
	}
}

void 
Thread::incFreq() {
	auto itr = paths.begin();
	while(itr != paths.end()) {
		(*itr).incFreq();
		++itr;
	}
}

void 
Thread::decFreq() {
	auto itr = paths.begin();
	while(itr != paths.end()) {
		(*itr).decFreq();
		++itr;
	}
}

void 
Thread::setFreq(unsigned freq) {
	// std::cout << "in Thread::setFreq thread id: " << threadId << std::endl;
	auto itr = paths.begin();
	while(itr != paths.end()) {
		(*itr).setFreq(freq);
		++itr;
	}
	// std::cout << std::endl;
}

// void 
// Thread::setNextStage(unsigned path, bool reset) {
// 	CodePath& p = paths[path];
// 	// proceed to next stage is job is incomplete
// 	if(!p.isLastStage() && !reset) {
// 		p.incNextStage();
// 		p.nextStageDecided = true;
// 	} else {
// 		p.resetStagePrio();
// 		while(true) {
// 			unsigned stage = p.getNextPathStageId();
// 			// Stage* s = p.stages[stage];
// 			// schedule to the highest priority stage with jobs
// 			// even if there is lock contention

// 			Stage* stg = paths[path].stages[stage];
// 			// 
// 			if(!queues[path][stage].empty()) {
// 				if(!stg->isNgxProc()) {
// 					p.nextStageDecided = true;
// 					if(debug) {
// 						printf("Thread %d, path %d nextStageDecided = %d after setNextStage\n", threadId, path, p.nextStageDecided);
// 						printf("Thread %d, path %d nextStage set to id: %d, name: %s\n", threadId, path, p.getNextPathStageId(), p.getNextStage()->getStageName().c_str());
// 					}
// 					return;
// 				} else {
// 					// for ngx process stage, might not be able to execute 
// 					auto itr = queues[path][stage].begin();
// 					while(itr != queues[path][stage].end()) {
// 						Job* j = *itr;
// 						unsigned cid = j->connId;
// 						if( (stg->connStates[cid].blocked && stg->connStates[cid].jid == j->id) ||
// 							!stg->connStates[cid].blocked) {
// 							p.nextStageDecided = true;

// 							if(debug) {
// 								printf("Thread %d, path %d nextStageDecided = %d after setNextStage\n", threadId, path, p.nextStageDecided);
// 								printf("Thread %d, path %d nextStage set to id: %d, name: %s\n", threadId, path, p.getNextPathStageId(), p.getNextStage()->getStageName().c_str());
// 							}

// 							return;
// 						}
// 						++itr;
// 					}


// 					// std::cout << "failed to set next stage to ngx proc" << std::endl;
// 				}
// 			}
// 			if(p.isLowestPrio())
// 				break;
// 			else
// 				p.reducePriority();
// 		}
// 		// no jobs in any queue, set the job to be undecided
// 		p.resetNextStage();
// 		p.nextStageDecided = false;
// 	}

// 	// if(debug) {
// 		// printf("Thread %d, path %d nextStageDecided = %d after setNextStage\n", threadId, path, p.nextStageDecided);
// 	// }
// }

void 
Thread::setNextStage(unsigned path, bool reset) {
	CodePath& p = paths[path];
	// proceed to next stage is job is incomplete
	if(!p.isLastStage() && !reset) {
		p.incNextStage();
		p.nextStageDecided = true;
	} else {
		p.resetStagePrio();
		while(true) {
			unsigned stage = p.getNextPathStageId();
			// Stage* s = p.stages[stage];
			// schedule to the highest priority stage with jobs
			// even if there is lock contention
			if(!queues[path][stage].empty()) {
				p.nextStageDecided = true;
				// if(debug) {
					// printf("Thread %d, path %d nextStageDecided = %d after setNextStage\n", threadId, path, p.nextStageDecided);
					// printf("Thread %d, path %d nextStage set to id: %d, name: %s\n", threadId, path, p.getNextPathStageId(), p.getNextStage()->getStageName().c_str());
				// }
				return;
			}
			if(p.isLowestPrio())
				break;
			else
				p.reducePriority();
		}
		// no jobs in any queue, set the job to be undecided
		p.resetNextStage();
		p.nextStageDecided = false;
	}

	// if(debug) {
		// printf("Thread %d, path %d nextStageDecided = %d after setNextStage\n", threadId, path, p.nextStageDecided);
	// }
}

// Job*
// Thread::getNextJob(unsigned path, unsigned stage) {

// 	Stage* stg = paths[path].stages[stage];
// 	Job* j = nullptr;
// 	if(stg->isNgxProc()) {
// 		auto itr = queues[path][stage].begin();
// 		while(itr != queues[path][stage].end()) {
// 			Job* curJob = *itr;
// 			unsigned cid = curJob->connId;
// 			if( (stg->connStates[cid].blocked && stg->connStates[cid].jid == curJob->id) ||
// 				!stg->connStates[cid].blocked ) {
// 				j = curJob;
// 				queues[path][stage].erase(itr);		
// 				break;
// 			}
// 			++itr;
// 		}
// 	} else {
// 		if(!queues[path][stage].empty()) {
// 			j = *(queues[path][stage].begin());
// 			queues[path][stage].pop_front();
// 		}
// 	}
// 	return j;
// }

Job*
Thread::getNextJob(unsigned path, unsigned stage) {
	Job* j = nullptr;
	if(!queues[path][stage].empty()) {
		j = *(queues[path][stage].begin());
		queues[path][stage].pop_front();
	}
	return j;
}

void
Thread::getJobByConn(unsigned path, unsigned stage, unsigned conn, std::list<Job*>& jList) {
	auto itr = queues[path][stage].begin();
	while(itr != queues[path][stage].end()) {
		Job* j = *itr;
		if(j->connId == conn) {
			jList.push_back(j);
			itr = queues[path][stage].erase(itr);
		} else
			++itr;
	}
}

void
Thread::getJobPerConn(unsigned path, unsigned stage, std::list<Job*>& jList) {
	std::vector<bool> conn(500);
	auto itr = queues[path][stage].begin();
	while(itr != queues[path][stage].end()) {
		Job* j = *itr;
		unsigned c = j->connId;
		assert(c < 500);

		if(conn[c] == false) {
			conn[c] = true;
			jList.push_back(j);
			itr = queues[path][stage].erase(itr);
		} else
			++itr;
	}
}

unsigned
Thread::jobNumByConn(unsigned path, unsigned stage, unsigned conn) {
	unsigned cnt = 0;
	auto itr = queues[path][stage].begin();
	while(itr != queues[path][stage].end()) {
		Job* j = *itr;
		if(j->connId == conn)
			++cnt;
		++itr;
	}
	return cnt;
}

unsigned
Thread::numConn(unsigned path, unsigned stage) {
	std::vector<bool> conn(500);
	unsigned ctr = 0;
	auto itr = queues[path][stage].begin();
	while(itr != queues[path][stage].end()) {
		Job* j = *itr;
		unsigned c = j->connId;
		assert(c < 500);
		// std::cout << "find conn : " << c << std::endl;

		if(conn[c] == false) {
			++ctr;
			conn[c] = true;
		}
		++itr;
	}
	return ctr;
}

// bool
// Thread::schedule() {
// 	assert(state != BLOCKED);
// 	schedSuc = false;
// 	Job* j = nullptr;

// 	for(unsigned pathId = 0; pathId < paths.size(); ++pathId) {
// 		// try to decide path's next stage if it is undecided
// 		if(!paths[pathId].nextStageDecided)
// 			setNextStage(pathId, true);
// 		else {
// 			// for ngxProc stage, try to reschedule to avoid deadlock
// 			Stage* s = paths[pathId].stages[paths[pathId].getNextPathStageId()];
// 			if(s->isNgxProc())
// 				setNextStage(pathId, true);
// 		}
		
// 		if (debug)
// 			printf("Thread: %d, path: %d nextStage decided to be %d\n", threadId, pathId, paths[pathId].getNextPathStageId());

// 		if(!paths[pathId].nextStageDecided)
// 			continue;
// 		Stage* s = paths[pathId].getNextStage();
// 		if(!s->criSecAvail())
// 			continue;

// 		unsigned stageId = paths[pathId].getNextPathStageId();
// 		if(queues[pathId][stageId].empty())
// 			continue;
// 		else if(s->isNgxProc()) {
// 			auto itr = queues[pathId][stageId].begin();
// 			while(itr != queues[pathId][stageId].end()) {
// 				j = *itr;
// 				unsigned cid = j->connId;
// 				if ( (s->connStates[cid].blocked && s->connStates[cid].jid == j->id) ||
// 					 !s->connStates[cid].blocked ) {
// 					nextPath = pathId;
// 					schedSuc = true;
// 					break;
// 				}
// 				++itr;
// 			}
// 		} else {
// 			Job* curJob = *queues[pathId][stageId].begin();
// 			if(j == nullptr || (j->waitReq() && curJob->waitResp()) 
// 				|| (j->waitResp() == curJob->waitResp() && curJob->time < j->time) ) {
// 				j = curJob;
// 				nextPath = pathId;
// 				schedSuc = true;
// 			}
// 		}
// 	}

// 	return schedSuc;
// }

bool
Thread::schedule() {
	assert(state != BLOCKED);
	schedSuc = false;
	Job* j = nullptr;

	for(unsigned pathId = 0; pathId < paths.size(); ++pathId) {
		// try to decide path's next stage if it is undecided
		if(!paths[pathId].nextStageDecided)
			setNextStage(pathId, true);
		else if (debug)
			printf("Thread: %d, path: %d nextStage decided to be %d\n", threadId, pathId, paths[pathId].getNextPathStageId());

		if(!paths[pathId].nextStageDecided)
			continue;
		Stage* s = paths[pathId].getNextStage();
		if(!s->criSecAvail())
			continue;

		unsigned stageId = paths[pathId].getNextPathStageId();
		if(queues[pathId][stageId].empty())
			continue;
		else {
			Job* curJob = *queues[pathId][stageId].begin();
			if(j == nullptr || (j->waitReq() && curJob->waitResp()) 
				|| (j->waitResp() == curJob->waitResp() && curJob->time < j->time) ) {
				j = curJob;
				nextPath = pathId;
				schedSuc = true;
			}
		}
	}

	return schedSuc;
}