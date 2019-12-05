#include <stdlib.h>

#include "machine.hh"

/******************* Machine ********************/
Machine::Machine(unsigned id, const std::string& name, unsigned cores, bool debug):
	id(id), name(name), totalCores(cores), netStack(nullptr), debug(debug)
{
	corePool = new Core*[totalCores];
	for(unsigned i = 0; i < totalCores; ++i)
		corePool[i] = new Core(i);
	dummyClient = new ClientRx();
}

Machine::~Machine() {
	// delete tcp is done in cluster
	for(unsigned i = 0; i < totalCores; ++i)
		delete corePool[i];
	delete corePool;
	delete dummyClient;
}

unsigned
Machine::getId() {
	return id;
}

const std::string&
Machine::getName() {
	return name;
}

unsigned 
Machine::numCores() {
	return totalCores;
}

void 
Machine::setNet(NetStack* net_stack, const std::string& schedType, unsigned num_ques, std::vector<unsigned> cid) {
	netStack = net_stack;
	std::vector<Core*> cores;
	for(auto i: cid) {
		if(i >= totalCores) {
			printf("Machine: %s netStack core spec: %d out of bound\n", name.c_str(), i);
			exit(-1);
		}
		cores.push_back(getCore(i));
	}
	netStack->setSched(schedType, num_ques, cores);

	assert(dummyClient != nullptr);
	net_stack->dummyClient = dummyClient;
}

NetStack*
Machine::getNet() {
	return netStack;
}

void
Machine::setNetLat(Time lat) {
	assert(netStack != nullptr);
	netStack->setNetLat(lat);
}

void
Machine::addService(MicroService* serv) {
	services.push_back(serv);
	serv->setNet(netStack);
	netStack->addLocalServ(serv);

	assert(dummyClient != nullptr);
	serv->dummyClient = dummyClient;
}

std::vector<MicroService*>
Machine::getServices() {
	return services;
}

// TODO: more flexbile implementation for service discovery which enables service transfer
void
Machine::addConn(Machine* mac) {
	if(connectedMachines.find(mac->getId()) == connectedMachines.end())
		connectedMachines.insert(mac->getId());
	else
		return;
	for(auto serv: mac->getServices()) 
		netStack->addConn(serv->getId(), mac->getNet());
}

Core*
Machine::getCore(unsigned cid) {
	assert(cid < totalCores);
	return corePool[cid];
}