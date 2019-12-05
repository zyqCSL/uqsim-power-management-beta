#ifndef _CLUSTER_HH_
#define _CLUSTER_HH_

#include "machine.hh"
#include "micro_service.hh"
#include "micro_service_path.hh"

class Cluster
{
	private:
		// maps micro service name to its index
		std::unordered_map<std::string, unsigned> servNameIdMap;

		std::unordered_map<std::string, std::vector<MicroService*>> servMap;
		// maps machine id to machine
		std::unordered_map<unsigned, Machine*> machineMap;
		unsigned id;
		// microservice paths
		std::vector<MicroServPath> paths;
		std::vector<unsigned> pathDistr;

		EventQueue* eventQueue;

	public:	
		std::vector<MicroService*> services;

	public:
		Cluster(EventQueue* eq): eventQueue(eq) {}
		virtual ~Cluster();

		unsigned getServId(const std::string& n);
		MicroService* getService(const std::string& n);
		
		// micro service
		void addService(MicroService* serv);
		void addEdge(std::string srcServ, std::string targServ, bool biDir);
		unsigned getNumService();
		// micro service path
		void setServPath(std::vector<MicroServPath> path);
		void setServPathDistr(std::vector<unsigned> dist);

		// machines
		void addMachine(Machine* mac);
		Machine* getMachine(unsigned mid);
		void setNetLat(Time lat);
		void setupConn();

		void enqueue(Job* j);

		Time nextEventTime(Time globalTime);

		void run(Time globalTime);

		bool jobLeft();

		// dvfs
		void setFreq(const std::string& serv_name, unsigned freq);
		void decServFreq(Time time, const std::string& serv_name);
		void incServFreq(Time time, const std::string& serv_name);
		void incServFreqFull(Time time, const std::string& serv_name);

		void showCpuUtil(Time time);
		void getPerTierTail(std::unordered_map<std::string, Time>& lat_info);
};

#endif