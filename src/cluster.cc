#include <time.h>

#include "cluster.hh"

Cluster::~Cluster() {
	srand(time(NULL));
	for(auto serv: services) 
		delete serv;
	for(auto ele: machineMap)
		delete ele.second;
}

unsigned
Cluster::getServId(const std::string& n) {
	assert(servNameIdMap.find(n) != servNameIdMap.end());
	return servNameIdMap[n];
}

MicroService*
Cluster::getService(const std::string& n) {
	unsigned index = getServId(n);
	return services[index];
}

void
Cluster::addService(MicroService* serv) {
	unsigned servId = services.size();
	services.push_back(serv);
	serv->setId(servId);
	servNameIdMap[serv->getName()] = servId;
	// std::pair<std::string, std::string> key = std::pair<std::string, std::string> (serv->getServName(), serv->getServDomain());
	std::string key = serv->getServName() + '_' + serv->getServDomain();
	if(servMap.find(key) == servMap.end())
		servMap[key] = std::vector<MicroService*>();
	servMap[key].push_back(serv);
}

void
Cluster::addEdge(std::string srcServ, std::string targServ, bool biDir) {
	assert(servNameIdMap.find(srcServ) != servNameIdMap.end());
	// std::cout << targServ << std::endl;
	assert(servNameIdMap.find(targServ) != servNameIdMap.end());
	MicroService* src = services[servNameIdMap[srcServ]];
	MicroService* targ = services[servNameIdMap[targServ]];
	src->addSendChn(targ);
	if(biDir)
		targ->addSendChn(src);
}

unsigned
Cluster::getNumService() {
	return services.size();
}

void 
Cluster::setServPath(std::vector<MicroServPath> path) {
	paths = path;
}

void
Cluster::setServPathDistr(std::vector<unsigned> dist) {
	assert(dist.size() == paths.size());
	unsigned accum = 0;
	for(unsigned prob: dist) {
		accum += prob;
		pathDistr.push_back(accum);
	}
	if(accum != 100) {
		printf("accum = %u\n", accum);
		printf("Error: all path probability do not sum up to 100\n");
		for(unsigned i = 0; i < dist.size(); ++i) {
			printf("Path %d probability: %d\n", i, dist[i]);
		}
		exit(1);
	}
	// assert(accum == 1.0);
}

void 
Cluster::addMachine(Machine* mac) {
	unsigned mid = mac->getId();
	assert(machineMap.find(mid) == machineMap.end());
	machineMap[mid] = mac;
}

Machine* 
Cluster::getMachine(unsigned mid) {
	auto itr = machineMap.find(mid);
	assert(itr != machineMap.end());
	return (*itr).second;
}

void
Cluster::setNetLat(Time lat) {
	for(auto m: machineMap)
		m.second->setNetLat(lat);
}

void
Cluster::setupConn() {
	unsigned i = 0;
	for(auto p1: machineMap) {
		Machine* m1 = p1.second;
		for(auto p2: machineMap) {
			Machine* m2 = p2.second;
			if(m1->getId() != m2->getId())	{
				m1->addConn(m2);
				m2->addConn(m1);
			}
		}
		i += 1;
		// if(i > machineMap.size()/2)
		// 	break;
	}
}

void
Cluster::enqueue(Job* j) {
	// select a path for this job
	unsigned prob = rand() % 100;
	unsigned i = 0;
	while(pathDistr[i] < prob)
		++i;
	j->setPathNode(paths[i].getEntry());
	MicroServPathNode* node = paths[i].getEntry();
	// std::pair<std::string, std::string> key = std::pair<std::string, std::string> (node->getServName(), node->getServDomain());
	std::string key = node->getServName() + '_' + node->getServDomain();
	assert(servMap.find(key) != servMap.end());
	unsigned pos = rand() % servMap[key].size();
	// set visited
	j->setServId(node->getServName(), node->getServDomain(), servMap[key][pos]->getId());
	j->netSend = false;
	j->targServId = servMap[key][pos]->getId();
	// printf("In Cluster::enqueue set job: %d  set targServId: %d\n", j->id, j->targServId);	
	j->srcServId = unsigned(-1);
	j->newPathNode = true;
	servMap[key][pos]->getNet()->enqueue(j);
}

Time
Cluster::nextEventTime(Time globalTime) {
	return eventQueue->nextEventTime();
	// std::cout << "cluster next event time = " << time << std::endl;
}

// modify this to cope with ClientRxThread
void
Cluster::run(Time globalTime) {
	// std::cout << "cluster new round" << std::endl;
	// eventQueue->show();
	Event* e = eventQueue->pop();
	if(e == nullptr)
		return;
	e->run(globalTime);
	if(e->del)
		delete e;
	// nextServ->run(globalTime);
}

bool
Cluster::jobLeft() {
	if(eventQueue->nextEventTime() != INVALID_TIME) {
		std::cout << "Deadlock Check: Event Queue not empty" << std::endl;
		return false;
	}
	for(auto serv: services) {
		if(serv->jobLeft())
			return true;
	}
	return false;
}

void
Cluster::setFreq(const std::string& serv_name, unsigned freq) {
	for(MicroService* serv: services) {
		// std::cout << serv->getName() << std::endl;
		if(serv->getName() == serv_name)
			serv->setFreq(freq);
	}
}

// rapl
void 
Cluster::decServFreq(Time time, const std::string& serv_name) {
	// return;
	auto itr = services.begin();
	while(itr != services.end()) {
		const std::string& name = (*itr)->getServName();
		if(name.find(serv_name) != std::string::npos) {
			Event* event = new Event(Event::EventType::DEC_FREQ);
			event->time = time;
			(*itr)->insertEvent(event);
		}
		++itr;
	}

	// auto itr = servNameIdMap.find(serv_name);
	// assert(itr != servNameIdMap.end());

	// unsigned serv_id = itr->second;
	// Event* event = new Event(Event::EventType::DEC_FREQ);
	// event->time = time;
	// services[serv_id]->insertEvent(event);
}

void 
Cluster::incServFreq(Time time, const std::string& serv_name) {
	// return;
	auto itr = services.begin();
	while(itr != services.end()) {
		const std::string& name = (*itr)->getServName();
		if(name.find(serv_name) != std::string::npos) {
			Event* event = new Event(Event::EventType::INC_FREQ);
			event->time = time;
			(*itr)->insertEvent(event);
		}
		++itr;
	}

	// auto itr = servNameIdMap.find(serv_name);
	// assert(itr != servNameIdMap.end());

	// unsigned serv_id = itr->second;
	// Event* event = new Event(Event::EventType::INC_FREQ);
	// event->time = time;
	// services[serv_id]->insertEvent(event);
}

void 
Cluster::incServFreqFull(Time time, const std::string& serv_name) {
	// return;

	auto itr = services.begin();
	while(itr != services.end()) {
		const std::string& name = (*itr)->getServName();
		if(name.find(serv_name) != std::string::npos) {
			Event* event = new Event(Event::EventType::INC_FULL_FREQ);
			event->time = time;
			(*itr)->insertEvent(event);
		}
		++itr;
	}

	// auto itr = servNameIdMap.find(serv_name);
	// assert(itr != servNameIdMap.end());

	// unsigned serv_id = itr->second;
	// Event* event = new Event(Event::EventType::INC_FULL_FREQ);
	// event->time = time;
	// services[serv_id]->insertEvent(event);
}

void
Cluster::showCpuUtil(Time time) {
	for(MicroService* serv: services) {
		double util = serv->getCpuUtil(time);
		std::cout << serv->getName() << " cpu util = " << util << std::endl;
	}
}

void
Cluster::getPerTierTail(std::unordered_map<std::string, Time>& lat_info) {
	for(MicroService* serv: services) {
		Time lat = serv->getTailLat();
		serv->clearRespTime();
		lat_info[serv->getName()] = lat;
	}
}

