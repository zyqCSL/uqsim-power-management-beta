#ifndef __MACHINE_HH__
#define __MACHINE_HH__

#include <vector>
#include <set>
#include <string>

#include <stdio.h>      /* printf, fopen */
#include <stdlib.h>     /* exit, EXIT_FAILURE */

#include "core.hh"
#include "micro_service.hh"

class Machine;

class Machine
{
	private:
		unsigned id;
		std::string name;
		// total resource
		unsigned totalCores;
		Core** corePool;

		// tcp service
		NetStack* netStack;
		std::vector<MicroService*> services;
		bool debug;
		std::set<unsigned> connectedMachines;

	public:
		ClientRx* dummyClient;

		Machine(unsigned id, const std::string& name, unsigned cores, bool debug);
		~Machine();

		unsigned getId();
		const std::string& getName();
		unsigned numCores();

		void setNet(NetStack* net_stack, const std::string& schedType, unsigned num_queues, std::vector<unsigned> cid);
		NetStack* getNet();
		void setNetLat(Time lat);

		void addService(MicroService* serv);
		std::vector<MicroService*> getServices();

		void addConn(Machine* mac);

		Core* getCore(unsigned cid);
};

#endif