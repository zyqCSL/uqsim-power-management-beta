#ifndef _MICRO_SERVICE_H_
#define _MICRO_SERVICE_H_

#include <string>
#include <algorithm>
#include <stdio.h>
#include <array>
#include <vector>
#include <unordered_map>
#include <functional>

#include "micro_service_sched.hh"

class ClientRx;

// class LoadBalancer;
class MicroService;
class NetStack;

class MicroService
{
	protected:
		/** micro-service property **/
		// service instance id
		unsigned id;
		std::string instName;
		// service name (memcached e.g.)
		std::string servName;
		std::string servDomain;

		// if this micro-service is a load balancer
		bool loadBal;

		bool bindConn;
		bool debug;
		// maps (service name, service domain) to connected service instances
		std::unordered_map<std::string, std::vector<MicroService*> > sendChn;

		MicroServScheduler* scheduler;

		std::vector<CodePath> paths;

		/** event handling (for simulation) **/
		EventQueue* eventQueue;

		// probability/dist of going into different paths (precent)
		std::vector<unsigned> pathDist;

		// critical sections
		std::unordered_map<unsigned, unsigned*> criSect;

		// network
		NetStack* netStack;

		unsigned baseFreq;
		unsigned curFreq;


		std::vector<Time> resp_time;

	public:
		bool common;

		bool nextEventDummy;

		ClientRx* dummyClient;

		MicroService(unsigned id, const std::string& inst_name, const std::string& serv_name, 
			const std::string& serv_func, bool lb, bool bindcon,
			unsigned base_freq, unsigned cur_freq, bool debug, EventQueue* eq);
		virtual ~MicroService();

		/** micro service properties **/
		void setId(unsigned id);
		unsigned getId();
		const std::string& getName();
		const std::string& getServName();
		const std::string& getServDomain();
		bool isLoadBal();

		void addPath(const CodePath& p);
		void setPathDistr(const std::vector<unsigned> dist);
		unsigned decidePath();

		virtual void setSched(const std::string& type, unsigned numThdOrQue, std::vector<Core*> coreList);		
		virtual void setCoreAffinity(unsigned tid, std::vector<Core*> cores);
		virtual void addSendChn(MicroService* s);

		virtual void setNet(NetStack* serv);
		virtual NetStack* getNet();

		/** event handling **/
		virtual void insertEvent(Event* e);
		// return if event handling is successful
		virtual bool eventHandler(Event* event, Time globalTime);

		// first enqueue and then dispatch
		virtual void enqueue(Job* j);

		/** deadlock check**/
		// if there are any event left in queue unprocessed
		virtual bool jobLeft();

		double getCpuUtil(Time elapsed_time);
		Time getTailLat();
		void clearRespTime();

		void setFreq(unsigned freq);

	protected:
		virtual void send(Job* j);
};

/*** entire network is modeled as a service and is not part of micro service path ***/
/**
 * TODO: should be splitted into two parts, nic and linux network stack should be modeled differently
 */
class NetStack: public MicroService 
{
	protected:
		// maps each microservice service instance id to the tcp instance 
		// of the machine on which it's deployed
		std::unordered_map<unsigned, NetStack*> connections;
		// micro-services that are on the same service as this tcp
		std::unordered_map<unsigned, MicroService*> localServ;

		Time netLat;

	public:


		NetStack(unsigned id, const std::string& instName, const std::string& servName,
			 const std::string& servDomain, bool debug, EventQueue* eq);
		~NetStack() {}
		void addConn(unsigned serv_id, NetStack* tcp);	
		void setLocalServ(std::unordered_map<unsigned, MicroService*> local_serv);
		void addLocalServ(MicroService* serv);
		void setNetLat(Time lat);

		// derived
		void addSendChn(MicroService* s) override;
		void setNet(NetStack* s) override;
		NetStack* getNet() override;
		void enqueue(Job* j) override;

		// return if event handling is successful
		void insertEvent(Event* e) override;
		bool eventHandler(Event* event, Time globalTime) override;
	protected:
		void send(Job* j) override;
};

/*** tcp service is not part of micro service path ***/
// LoadBalancer should have the same ServName and servDomain as the services it's balancing
class LoadBalancer: public MicroService
{
	protected:
		// for round robin load balancing
		unsigned nextServ;
		std::vector<MicroService*> services;

	public:
		LoadBalancer(unsigned id, const std::string& inst_name, const std::string& serv_name, 
			const std::string& serv_func, bool bindcon, bool debug, EventQueue* eq);
		~LoadBalancer() {}

		// derived
		void addSendChn(MicroService* s) override;
		void enqueue(Job* j) override;
	protected:
		void send(Job* j) override;
};


/********** dumyClient *********/
// dumpy client receiver on this machine
class ClientRxThread
{
	public:
		std::vector<unsigned> connIds;
		std::unordered_map<unsigned, std::list<Job*> > connJobList;
		Time cpuNextAvailTime;

		ClientRxThread();
		~ClientRxThread();

		bool hasConn(unsigned conn);
		void enqueue(Job* j);
		void getJob(std::list<Job*>& jobList, Time time);

		void execute(Time time);

		bool jobLeft();

		ExpoTimeModel* proc_tm;
		ExpoTimeModel* epoll_tm;

};


class ClientRx{
	public:
		std::vector<ClientRxThread*> threads;
		unsigned numThreads;

		unsigned nextThread;

		ClientRx();

		void enqueue(Job* j);

		void execute(Time time);

		bool jobLeft();
};

#endif