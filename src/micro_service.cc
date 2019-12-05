#include "micro_service.hh"
#include <iostream>
#include <stdio.h>
#include <stdlib.h> 
#include <time.h>  
#include <algorithm>     

/************** MicroService ***************/
MicroService::MicroService(unsigned id, const std::string& inst_name, const std::string& serv_name, 
	const std::string& serv_func, bool lb, bool bindcon, unsigned base_freq, unsigned cur_freq, bool debug, EventQueue* eq): 
	id(id), instName(inst_name), servName(serv_name), servDomain(serv_func), loadBal(lb), bindConn(bindcon), 
	debug(debug), scheduler(nullptr), eventQueue(eq), netStack(nullptr),
	baseFreq(base_freq), curFreq(cur_freq) {
		dummyClient = nullptr;
		nextEventDummy = false;
		common = true;

	// if(serv_name == "mongodb") {
	// 	std::cout << "mongodb located" << std::endl;
	// 	this->debug = true;
	// }
	// srand(time(NULL)); // done in cluster
	// std::cout << "inst_name = " << instName << std::endl;
}

MicroService::~MicroService() {
	delete scheduler;
	for(auto& path: paths) {
		for(unsigned i = 0; i < path.numStages; ++i)
			delete path.stages[i];
	}
	for(auto lock: criSect)
		delete lock.second;
}

void
MicroService::setId(unsigned idx) {
	id = idx;
}

unsigned
MicroService::getId() {
	return id;
}

const std::string& 
MicroService::getName() {
	return instName;
}

const std::string& 
MicroService::getServName() {
	return servName;
}

const std::string& 
MicroService::getServDomain() {
	return servDomain;
}

bool
MicroService::isLoadBal() {
	return loadBal;
}

void 
MicroService::setSched(const std::string& type, unsigned numThdOrQue, std::vector<Core*> coreList) {
	// std::cout << "in set numCores = " << numCores << std::endl;
	// TODO: create sched here
	if(type == "CMT")
		scheduler = new CMTScheduler(id, paths, criSect, bindConn, numThdOrQue, coreList, debug);
	else if(type == "Simplified") {
		scheduler = new SimpScheduler(id, paths, bindConn, numThdOrQue, coreList, debug);
	} else if(type == "LinuxNetStack") {
		scheduler = new LinuxNetScheduler(id, paths, bindConn, numThdOrQue, coreList, debug);
	} else {
		printf("No matching microservice scheduler");
		exit(-1);
	}
}

void
MicroService::setCoreAffinity(unsigned tid, std::vector<Core*> cores) {
	assert(scheduler != nullptr);
	scheduler->setCoreAffinity(tid, cores);
}

void
MicroService::addPath(const CodePath& p) {
	paths.push_back(p);
	// deal with critical sections (only model global locks)
	for(auto stage: p.stages) {
		if(stage->isCriSec()) {
			unsigned stgId = stage->getStageId();
			if(criSect.find(stgId) == criSect.end()) {
				unsigned* lock = new unsigned;
				*lock = stage->getThdLmt();
				criSect[stgId] = lock;
				stage->setCriSec(lock);
			} else {
				assert(stage->getThdLmt() == *(criSect[stgId]));
				stage->setCriSec(criSect[stgId]);
			}
		}
	}
}

void 
MicroService::setPathDistr(const std::vector<unsigned> dist) {
	assert(dist.size() == paths.size());
	unsigned s = 0;
	for(auto prob: dist) {
		s += prob;
		pathDist.push_back(s);
	}
	assert(s == 100);
}

unsigned 
MicroService::decidePath() {
	if(paths.size() == 1)
		return 0;
	unsigned p = rand() % 100;
	unsigned path;
	for(path = 0; path < pathDist.size(); ++path) {
		if(pathDist[path] >= p)
			break;
	}
	assert(path != pathDist.size());
	std::cout << "path decided to be " << path << std::endl;
	return path;
}

// channels
void
MicroService::addSendChn(MicroService* s) {
	std::string key = s->getServName() + '_' + s->getServDomain();
	if(sendChn.find(key) == sendChn.end())
		sendChn[key] = std::vector<MicroService*>();
	sendChn[key].push_back(s);
}

void 
MicroService::setNet(NetStack* net_serv) {
	assert(net_serv != nullptr);
	netStack = net_serv;
}

NetStack*
MicroService::getNet() {
	return netStack;
}

/*** event handling ***/
void
MicroService::insertEvent(Event* e) {
	assert(e != nullptr);
	if(debug)
		printf("In MicroService::insertEvent, microservice id: %u, instName: %s, inserted Event: %s\n", 
			id, instName.c_str(), e->present().c_str());

	// printf("after insertEvent event_cnt = %lu\n", event_cnt);
	// insert call back
	e->registerCallBack(std::bind(&MicroService::eventHandler, this, e, std::placeholders::_1));
	eventQueue->enqueue(e);
	// printf("after insertEvent eventQueue.size() = %lu\n", eventQueue.size());
}

bool
MicroService::eventHandler(Event* event, Time globalTime) {
	assert(event->time <= globalTime);
	if(debug)
		printf("In MicroService::eventHandler, microservice id: %u, instName: %s, Event: %s\n", 
			id, instName.c_str(), event->present().c_str());

	// dvfs event
	if(event->type == Event::EventType::INC_FULL_FREQ) {
		event->del = true;
		curFreq = baseFreq;
		scheduler->incFullFreq();
		printf("RAPL: In MicroService instName: %s, id: %u, freq increased to full %uMHz\n", 
			instName.c_str(), id, baseFreq);
		
		return true;
	} else if(event->type == Event::EventType::INC_FREQ) {
		event->del = true;
		curFreq += 200;
		assert(curFreq <= 2600);
		scheduler->setFreq(curFreq);
		assert(curFreq <= baseFreq);
		printf("RAPL: In MicroService instName: %s, id: %u, freq increased to %uMHz\n", 
			instName.c_str(), id, curFreq);

		return true;
	} else if(event->type == Event::EventType::DEC_FREQ) {
		event->del = true;
		curFreq -= 200;
		assert(curFreq >= 1200);
		scheduler->setFreq(curFreq);
		printf("RAPL: In MicroService instName: %s, id: %u, freq decreased to %uMHz\n", 
			instName.c_str(), id, curFreq);

		return true;
	} else if(event->type == Event::EventType::SET_FREQ) {
		assert(event->freq >= 1200);
		assert(event->freq <= 2600);
		scheduler->setFreq(event->freq);
		event->del = true;
		curFreq = event->freq;
		printf("RAPL: In MicroService instName: %s, id: %u, freq set to %uMHz\n", 
			instName.c_str(), id, curFreq);

		return true;
	}

	// for queue length
	// printf("In MicroService::eventHandler, microservice id: %d, instName: %s, Event: %s\n", 
	// 		id, instName.c_str(), event->present().c_str());

	if(event->type == Event::EventType::COMPUT_COMPLETE) {
		bool servComplDecided = false;
		// Attention! send event's time is different from job's time which is arriving time	
		for(auto job: event->jobList) {
			if(job->lastStage() || job->tailStage) {
				// need to enter a new path node
				std::list<Job*> sendL;
				std::list<Job*> pendL;
				bool servCompl = false;

				job->leavePathNode(sendL, pendL, servCompl);
				if(!servComplDecided) {
					event->servCompl = servCompl;
					servComplDecided = true;
				}

				if(servCompl)
					resp_time.push_back(job->time - job->enq_time );

				if(job->del)
					delete job;
				// pending jobs
				auto itr = pendL.begin();
				while(itr != pendL.end()) {
					Job* j = *itr;
					// job is leaving current path node, so every child should enter new node
					j->enterPathNode();
					if(!j->syncDone()) {
						itr = pendL.erase(itr);
						delete j;
					} else
						++itr;
				}
				// merge pendL into event->pendList
				// dispatch is done in scheduler (to consider event scheduling)
				for(auto j: pendL)
					event->pendList.push_back(j);

				// send jobs
				for(auto j: sendL)
					send(j);
			} else {
				// not leaving current path node or stage, just move to next stage
				job->incStage();
				event->pendList.push_back(job);
			}
		}
		event->jobList.clear();
	} else if(event->type == Event::EventType::JOB_RECV) {
		auto itr = event->jobList.begin();
		while(itr != event->jobList.end()) {
			Job* j = *itr;
			assert(j->newPathNode);
			assert(j->getServName() == servName && j->getServDomain() == servDomain);
			j->enterPathNode();
			// decide code path
			if(j->getCodePath() == -1) {
				assert(pathDist.size() != 0);
				j->setCodePath(decidePath());
			}

			if(!j->syncDone()) {
				itr = event->jobList.erase(itr);
				delete j;
			} else
				++itr;
		}

		if(event->jobList.empty()) {
			event->del = true;
			if(debug)
				printf("Event handled\n\n");
			return true;
		}
	}

	// deliver event to service scheduler
	std::list<Event*> eventList;
	scheduler->eventHandler(event, globalTime, eventList);
	for(auto itr: eventList)
		insertEvent(itr);
	if(debug)
		printf("Event handled\n\n");
	return true;
}

void
MicroService::enqueue(Job* j) {
	if(debug) {
		printf("In MicroService::enqueue microservice id: %u, instName: %s, enqueue job: %lu at time: %lu ns\n", id, instName.c_str(), j->id, j->time);
	}
	Event* event = new Event(Event::EventType::JOB_RECV);
	event->time = j->time;
	j->enq_time = j->time;
	event->jobList.push_back(j);
	insertEvent(event);
	if(debug) {
		printf("Job enqueued\n\n");
	}
}

// void
// MicroService::run(double globalTime) {
// 	assert(!eventQueue.empty());
// 	auto itr = eventQueue.begin();
// 	Event* event = *itr;
// 	// erase the event here to avoid polluion of itr in eventHandler
// 	eventQueue.erase(itr);
// 	eventHandler(event, globalTime);
// 	// delete event done in scheduler
// }

bool
MicroService::jobLeft() {
	if(debug)
		std::cout << "check " << servName << " deadlock" << std::endl;

	bool left = scheduler->jobLeft();
	if(left)
		printf("In microservice: instName: %s, id %u, job left\n", instName.c_str(), id);
	// dummyClient
	if(!left && common) {
		left = dummyClient->jobLeft();
		if(left)
			printf("In microservice dummyClient: instName: %s, id %u, job left\n", instName.c_str(), id);
	}
	return left;
}

double
MicroService::getCpuUtil(Time elapsed_time) {
	return scheduler->getCpuUtil(elapsed_time);
}

Time
MicroService::getTailLat() {
	if(resp_time.empty())
		return INVALID_TIME;
	std::sort(resp_time.begin(), resp_time.end());
	return resp_time[unsigned(0.99 * resp_time.size())];
	// resp_time.clear();
}

void
MicroService::clearRespTime() {
	resp_time.clear();
}

void 
MicroService::setFreq(unsigned freq) {
	assert(freq >= 1200);
	assert(freq <= 2600);
	curFreq = freq;
	printf("MicroService instName: %s, id: %u, freq set to %uMHz\n", 
		instName.c_str(), id, curFreq);
	scheduler->setFreq(curFreq);
	std::cout << std::endl;
}

void
MicroService::send(Job* j) {
	assert(netStack != nullptr);
	j->srcServId = id;
	j->netSend = true;
	// can't send to itself
	assert(!(j->getServName() == servName && j->getServDomain() == servDomain));
	// std::array<std::string, 2> key = {j->getServName(), j->getServDomain()};
	std::string key = j->getServName() + '_' + j->getServDomain();

	if(j->getServName() == "client") {
		// TODO: when deleting all copies of the same job, should inform client or cluster that this job is done
		// delete j;
		j->net = true;
		netStack->enqueue(j);
	} else if(sendChn[key].size() == 1 && sendChn[key][0]->isLoadBal()) {
		j->targServId = sendChn[key][0]->getId();
		if(j->net)
			// send to network
			netStack->enqueue(j);	
		else
			// send to target without going throught network, disk i/o e.g.
			sendChn[key][0]->enqueue(j);	
	} else {
		unsigned targServ = 0;
		bool visited = j->getServId(j->getServName(), j->getServDomain(), targServ);
		if(visited) {
			// find a matching service
			bool suc = false;
			for(auto serv: sendChn[key]) {
				assert(!serv->isLoadBal());
				if(serv->getId() == targServ) {
					j->targServId = targServ;
					if(j->net)
						netStack->enqueue(j);
					else
						serv->enqueue(j);
					suc = true;
					break;
				}
			}
			assert(suc);
		} else {
			// random select a target service
			unsigned pos = rand() % sendChn[key].size();
			MicroService* s = sendChn[key][pos];
			j->setServId(j->getServName(), j->getServDomain(),s->getId());
			j->targServId = s->getId();
			if(j->net)
				netStack->enqueue(j);
			else
				s->enqueue(j);	
		}
	}
}


/************** NetStack ***************/
// instance name is set by machine
NetStack::NetStack(unsigned id, const std::string& instName, const std::string& servName, const std::string& servDomain, bool debug, EventQueue* eq):
	MicroService(id, instName, servName, servDomain, false, true, 2600, 2600, debug, eq) {
		common = false;
		netLat = INVALID_TIME;
}

void
NetStack::addConn(unsigned serv_id, NetStack* net_serv) {
	// std::cout << "conn serv_id = " << serv_id << " added" << std::endl;
	// assert(connections.find(serv_id) == connections.end());
	connections[serv_id] = net_serv;
}

void
NetStack::setLocalServ(std::unordered_map<unsigned, MicroService*> local_serv) {
	localServ = local_serv;
}

void 
NetStack::addLocalServ(MicroService* serv) {
	assert(localServ.find(serv->getId()) == localServ.end());
	localServ[serv->getId()] = serv;
}

void
NetStack::setNetLat(Time lat) {
	netLat = lat;
}

void
NetStack::setNet(NetStack* s) {
	printf("Error: setting NetStack for NetStack\n");
	exit(1);
}

NetStack*
NetStack::getNet() {
	printf("Error: getting NetStack for NetStack\n");
	exit(1);
}

void 
NetStack::addSendChn(MicroService* s) {
	return;
}

void 
NetStack::enqueue(Job* j) {
	if(debug) {
		printf("In NetStack::enqueue microservice id: %u, instName: %s, enqueue job: %lu at time: %lu ns\n", id, instName.c_str(), j->id, j->time);
	}
	j->in_nic = true;
	unsigned dummy;
	if(j->getServId(servName, servDomain, dummy) == false)
		j->setServId(servName, servDomain, id);

	// currently only implements tcp
	if(j->netSend) {
		if(j->getServName() == "client")
			j->net_code_path = 1;
		else if(localServ.find(j->targServId) != localServ.end())
			// code path 0: network send to localhost, not going through nic
			j->net_code_path = 0;
		else
			// code path 1: network send to remote host
			j->net_code_path = 1;
	} else {
		if(localServ.find(j->srcServId) != localServ.end())
			// code path 2: network recv from localhost, not going through nic
			j->net_code_path = 2;
		else
			// code path 3: network recv from remote host
			j->net_code_path = 3;
	}

	Event* event = new Event(Event::EventType::JOB_RECV);
	event->time = j->time;
	event->jobList.push_back(j);
	insertEvent(event);
}

void
NetStack::insertEvent(Event* e) {
	assert(e != nullptr);
	if(debug)
		printf("In NetStack::insertEvent, microservice id: %u, instName: %s, inserted Event: %s\n", 
			id, instName.c_str(), e->present().c_str());

	// printf("after insertEvent event_cnt = %lu\n", event_cnt);
	// insert call back
	e->registerCallBack(std::bind(&NetStack::eventHandler, this, e, std::placeholders::_1));
	eventQueue->enqueue(e);
	// printf("after insertEvent eventQueue.size() = %lu\n", eventQueue.size());
}

bool
NetStack::eventHandler(Event* event, Time globalTime) {
	assert(event->time <= globalTime);
	if(debug)
		printf("In NetStack::eventHandler, microservice id: %u, instName: %s, Event: %s\n", 
			id, instName.c_str(), event->present().c_str());

	// dvfs event
	if(event->type == Event::EventType::INC_FULL_FREQ) {
		event->del = true;
		curFreq = baseFreq;
		scheduler->incFullFreq();
		printf("RAPL: In MicroService instName: %s, id: %u, freq increased to full %uMHz\n", 
			instName.c_str(), id, baseFreq);
		
		return true;
	} else if(event->type == Event::EventType::INC_FREQ) {
		event->del = true;
		curFreq += 200;
		assert(curFreq <= 2600);
		scheduler->setFreq(curFreq);
		assert(curFreq <= baseFreq);
		printf("RAPL: In MicroService instName: %s, id: %u, freq increased to %uMHz\n", 
			instName.c_str(), id, curFreq);

		return true;
	} else if(event->type == Event::EventType::DEC_FREQ) {
		event->del = true;
		curFreq -= 200;
		assert(curFreq >= 1200);
		scheduler->setFreq(curFreq);
		printf("RAPL: In MicroService instName: %s, id: %u, freq decreased to %uMHz\n", 
			instName.c_str(), id, curFreq);

		return true;
	} else if(event->type == Event::EventType::SET_FREQ) {
		assert(event->freq >= 1200);
		assert(event->freq <= 2600);
		scheduler->setFreq(event->freq);
		event->del = true;
		curFreq = event->freq;
		printf("RAPL: In MicroService instName: %s, id: %u, freq set to %uMHz\n", 
			instName.c_str(), id, curFreq);

		return true;
	}

	// for queue length
	// printf("In MicroService::eventHandler, microservice id: %d, instName: %s, Event: %s\n", 
	// 		id, instName.c_str(), event->present().c_str());

	if(event->type == Event::EventType::COMPUT_COMPLETE) {
		// std::cout << "COMPUT_COMPLETE" << std::endl;
		// Attention! send event's time is different from job's time which is arriving time	
		for(auto job: event->jobList)
			send(job);
		event->jobList.clear();
		// event->del = true;
		std::list<Event*> eventList;
		scheduler->eventHandler(event, globalTime, eventList);
		for(auto itr: eventList)
			insertEvent(itr);
		if(debug)
			printf("Event handled\n\n");
		// return true;
	} else if(event->type == Event::EventType::JOB_RECV) {
		// std::cout << "JOB_RECV" << std::endl;
		std::list<Event*> eventList;
		scheduler->eventHandler(event, globalTime, eventList);
		for(auto itr: eventList)
			insertEvent(itr);
		if(debug)
			printf("Event handled\n\n");
		return true;
	}
	
	return true;
	// deliver event to service scheduler
	
}

void
NetStack::send(Job* j) {
	assert(netLat >= 0);
	j->in_nic = false;
	// TODO: should actually sent to client
	if(j->getServName() == "client") {
		j->time += netLat;
		// assert(dummyClient != nullptr);
		// dummyClient->enqueue(j);
		delete j;
		return;
	}

	// network should not change the original src and targ fields of job
	if(j->netSend) {
		if(debug)
			printf("In NetStack::send microservice id: %u, instName: %s, send job: %lu at time: %lu ns\n", id, instName.c_str(), j->id, j->time);
		// should always be network recv after a network send
		j->netSend = false;
		auto itr = localServ.find(j->targServId);
		if(itr != localServ.end())
			(*itr).second->enqueue(j);
		else {
			// std::cout << "targServId = " << j->targServId << std::endl;
			j->time += netLat;
			auto itrc = connections.find(j->targServId);
			assert(itrc != connections.end());
			(*itrc).second->enqueue(j);
		}
	} else {
		if(debug)
			printf("In NetStack::send microservice id: %u, instName: %s, recv job: %lu at time: %lu ns\n", id, instName.c_str(), j->id, j->time);
		auto itr = localServ.find(j->targServId);
		if(debug && itr == localServ.end()) {
			printf("job: %lu targServId: %u not found\n", j->id, j->targServId);
			printf("Local services for this netStack are:\n");
			for(auto ele: localServ)
				printf("serv id: %u, instName: %s\n", ele.second->getId(), ele.second->getName().c_str());
		}

		assert(itr != localServ.end());
		(*itr).second->enqueue(j);
	}
}


/************** LoadBalancer ***************/
LoadBalancer::LoadBalancer(unsigned id, const std::string& inst_name, const std::string& serv_name, 
		const std::string& serv_func, bool bindcon, bool debug, EventQueue* eq):
		MicroService(id, inst_name, serv_name, serv_func, true, bindcon, 2600, 2600, debug, eq) {
	nextServ = 0;
	common = false;
}

void
LoadBalancer::addSendChn(MicroService* s) {
	assert(s->getServName() == servName);
	assert(s->getServDomain() == servDomain);
	services.push_back(s);
}

void
LoadBalancer::enqueue(Job* j) {
	if(debug) {
		printf("In LoadBalancer::enqueue microservice id: %u, instName: %s, enqueue job: %lu at time: %lu ns\n", id, instName.c_str(), j->id, j->time);
	}

	assert(j->getServName() == servName);
	assert(j->getServDomain() == servDomain);
	// randomly select a code path
	int codePath = -1;
	MicroServPathNode* oriNode = j->getPathNode();
	// fake a new micro serv path node specific to the network service 
	// should travel through all stages without communicating with outside microservices
	MicroServPathNode* tempNode = new MicroServPathNode(servName, servDomain, codePath, 0, -1, 
		oriNode->getPathId(), oriNode->getNodeId(), false, 0);
	tempNode->setTemp();
	std::vector<MicroServPathNode*> childs;
	childs.push_back(oriNode);
	tempNode->setChilds(childs);
	j->setPathNode(tempNode);

	Event* event = new Event(Event::EventType::JOB_RECV);
	event->time = j->time;
	event->jobList.push_back(j);
	insertEvent(event);
}

void
LoadBalancer::send(Job* j) {
	assert(netStack != nullptr);
	// since load balancer is implict, srcServId is not here
	unsigned targServ = 0;
	bool visited = j->getServId(j->getServName(), j->getServDomain(), targServ);
	if(visited) {
		// find a matching service
		bool suc = false;
		unsigned pos = 0;
		for(auto serv: services) {
			if(serv->getId() == targServ) {
				if(pos == nextServ)
					nextServ = (nextServ + 1) % services.size();

				j->targServId = targServ;
				j->netSend = true;
				if(j->net)
					netStack->enqueue(j);
				else
					serv->enqueue(j);
				suc = true;
				break;
			} else
				++pos;
		}
		assert(suc);
	} else {
		// random select a target service
		MicroService* s = services[nextServ];
		nextServ = (nextServ + 1) % services.size();
		j->setServId(j->getServName(), j->getServDomain(), s->getId());
		j->targServId = s->getId();
		j->netSend = true;
		if(j->net)
			netStack->enqueue(j);
		else
			s->enqueue(j);	
	}
}


/*************************************/
ClientRxThread::ClientRxThread(): cpuNextAvailTime(0.0) {
	proc_tm = new ExpoTimeModel(40);
	epoll_tm = new ExpoTimeModel(15);
}

ClientRxThread::~ClientRxThread() {
	delete proc_tm;
	delete epoll_tm;
}

bool 
ClientRxThread::hasConn(unsigned conn) {
	if(find(connIds.begin(), connIds.end(), conn) == connIds.end())
		return false;
	else
		return true;
}

void
ClientRxThread::enqueue(Job* j) {
	if(std::find(connIds.begin(), connIds.end(), j->connId) == connIds.end()) {
		connIds.push_back(j->connId);
		connJobList[j->connId] = std::list<Job*> ();
	}

	std::list<Job*>& queue = connJobList[j->connId];
	auto itr = queue.end();

	if(itr == queue.begin())
		queue.push_back(j);
	else {
		--itr;
		bool suc = false;
		while(itr != queue.begin()) {
			if( (*itr)->time < j->time) {
				++itr;
				queue.insert(itr, j);
				suc = true;
				break;
			}
			--itr;
		}
		if(!suc) {
			if((*itr)->time > j->time )
				queue.insert(itr, j);
			else {
				++itr;
				queue.insert(itr, j);
			}
		}
	}

}

void
ClientRxThread::getJob(std::list<Job*>& jobList, Time time) {
	auto itr = connJobList.begin();
	while(itr != connJobList.end()) {
		std::list<Job*>& q = itr->second;
		while(!q.empty()) {
			Job* j = *(q.begin());
			if(j->time < time) {
				q.pop_front();
				jobList.push_back(j);
			} else
				break;
		}
		++itr;
	}
}

void
ClientRxThread::execute(Time time) {
	// std::cout << "dummy global time = " << time << std::endl;

	Time acc_time = time + epoll_tm->lat();
	if(cpuNextAvailTime > time)
		return;
	auto itr = connJobList.begin();
	bool executed = false;
	while(itr != connJobList.end()) {
		std::list<Job*>& q = itr->second;
		while(!q.empty()) {
			Job* j = *(q.begin());

			// std::cout << "jid = " << j->id << ", job time = " << j->time << std::endl;

			if(j->time <= time) {
				executed = true;
				q.pop_front();
				acc_time += proc_tm->lat();
				j->time = acc_time;
				delete j;
			} else
				break;
		}
		++itr;
	}

	// assert(executed);
	if(executed)
		cpuNextAvailTime = acc_time;

	// std::cout << "dummy execute done" << std::endl;
}

bool
ClientRxThread::jobLeft() {
	auto itr = connJobList.begin();
	while(itr != connJobList.end()) {
		std::list<Job*>& q = itr->second;
		if(!q.empty())
			return true;
		++itr;
	}
	return false;
}

ClientRx::ClientRx() {
	numThreads = 16;
	for(unsigned i = 0; i < numThreads; ++i) {
		// std::cout << "dummy init 1" << std::endl;

		ClientRxThread* t = new ClientRxThread();
		threads.push_back(t);
		// std::cout << "dummy init 2" << std::endl;
	}
	nextThread = 0;

	// std::cout << "dummy initiated" << std::endl;
}

void
ClientRx::enqueue(Job* j) {
	// std::cout << "enqueue jid " << j->id << " at time " << j->time << std::endl;

	for(unsigned i = 0; i < numThreads; ++i) {
		if(threads[i]->hasConn(j->connId)) {
			threads[i]->enqueue(j);
			return;
		}
	}

	threads[nextThread]->enqueue(j);
	nextThread = (nextThread + 1)%numThreads;
}

void
ClientRx::execute(Time time) {
	for(unsigned i = 0; i < numThreads; ++i)
		threads[i]->execute(time);
}

bool
ClientRx::jobLeft() {
	for(unsigned i = 0; i < numThreads; ++i) {
		bool left = threads[i]->jobLeft();
		if(left)
			return true;
	}
	return false;
}

