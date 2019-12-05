#ifndef _JOB_HH_
#define _JOB_HH_

#include <string>
#include <assert.h>
#include <limits>
#include <unordered_map>
#include <vector>
#include <list>
#include <utility>
#include <iostream>
#include <cstdint>

#include "util.hh"
#include "micro_service_path.hh"

class Job;
class JobRecord;
class JobTimeRecords;

class JobTimeRecords
{
	protected:
		std::vector<Time> jobComplTime;

	public:
		JobTimeRecords(unsigned init_size);
		~JobTimeRecords() {}

		// add 
		void jobCompl(Time time);
		// return 99% tail latency
		Time getTailLat();
		Time getAvgLat();

		void clear();
};

class Job
{
	private:
		unsigned numServices;
		// maps the microservice id that this job goes through
		// to all job records corresponding to the service
		std::unordered_map<unsigned, std::list<JobRecord*>>* records;
		// maps (serv_name, serv_func) in path to micro-service index in the depedency graph
		// std::unordered_map<std::pair<std::string, std::string>, unsigned>* servMap;
		std::unordered_map<std::string, unsigned>* servMap;
		JobRecord* curRecord;
		// counter for dereferencing records
		int* cnt;
		// shared time record the for job
		Time* finalTime;
		bool* complete;
		// the node (in micro service path) the job is currently in
		MicroServPathNode* pathNode;

		bool debug;

		JobTimeRecords* complRecords;

	public:
		// # of micro-service instances in the group
		// unsigned numServices;
		uint64_t id;
		// connection id
		unsigned connId;
		unsigned net_code_path;
		bool in_nic;

		Time startTime;
		Time time;
		Time enq_time;

		// if this job is entering a new pathNode
		bool newPathNode;
		// garbage collection
		bool del;
		// if this job reaches tail(end) stage of the path
		bool tailStage;
		// if this job needs to go through network
		bool net;
		// used to determine if a network recv is from localhost or remote host
		unsigned srcServId;
		// tell network stack the target of the job
		unsigned targServId;
		// if this job is a transmit or a receive
		bool netSend;

		bool ngxProc;

		std::string trace;

		Job(uint64_t id, unsigned connid, JobTimeRecords* time_records, MicroServPathNode* node, Time initime, Time* ftime,
			bool* complete, bool debug);
		Job(Job* j);
		~Job();

		/** time keeping **/
		// return time job's first enqueued into micro-service
		Time getEnqTime();
		
		/**** intra-micro service and CodePath ****/
		unsigned getThread();
		int getCodePath();
		void setCodePath(int code_path);
		// thread related
		bool threadAssigned();
		void assignThread(unsigned thread_id);
		void blockThread();
		bool unblockThread();

		/*** micro-service path ***/
		std::string getServName();
		std::string getServDomain();
		void setServId(std::string serv_name, std::string serv_func, unsigned servId);
		bool getServId(std::string serv_name, std::string serv_func, unsigned& servId);
		MicroServPathNode* getPathNode();
		void setPathNode(MicroServPathNode* node);
		// if job has reached the last stage (in codePath) specified by current pathNode
		// must be called after finishing all operations at current stage
		// including decrementing chunkNum
		bool lastStage();
		unsigned getStage();
		void setStage(unsigned s);
		// update record when job is not leaving current path node and current stage
		void incStage();

	private:
		/*** synchronization & JobRecord ***/
		void createRecord();
		// update record when entering next path node but not leaving the current micro-service
		void updateRecord(MicroServPathNode* nextNode);
		// try to match job to an existing record
		bool matchRecord();
		void clearRecord();
	public:
		void recv();
		void enterPathNode();
		// servComplete indicate if the job has finished all computation at the current micro-service
		void leavePathNode(std::list<Job*>& sendQ, std::list<Job*>& pendQ, bool& servCompl);
		bool syncDone();
		bool needSync();
		// status of job
		bool waitReq();
		bool waitResp();

		// chunk processing
		// should be called on entering a chunk stage for the first time
		// return true if the job is chunking on a internal stage of current micro service
		bool setChunk(unsigned chkNum);
		// return true if this job is chunking on an internal stage
		// must be called after chunkSet()
		bool chunkInternal();
		bool chunkDone();
		void decChunkNum();
		bool chunkSet();
		unsigned getChunkNum();
		void clearChunk();
};

// with path specified, it is still useful to keep JobRecord to keep information
// of synchronization, thread blocking and safety check that a job completely traversed all stages
// in its specified codePath
class JobRecord
{	
	friend class Job;
	public:
		enum class State {WAIT_REQ, WAIT_RESP};
	private:
		/***** micro service path ******/
		// index of micro service path
		unsigned microServPath;
		// index of micro service path node that this job is/will be in at this micro service 
		unsigned pathNode;
		// # req/resp to be synchronized
		unsigned syncNum;
		
		/*** machine/OS ***/
		bool threadAssigned;
		unsigned threadId;

		/*** processing status ***/
		// micro service id
		unsigned servId;
		// there could be nodes not specifying CodePath
		int codePath;
		unsigned stage;
		// time that job first enters this micro-service
		Time initTime;
		// time that last synchronized input arrives
		Time time;
		State stat;

		/**** chunk processing ****/
		bool chunk;
		// how many times the job needs to be processed at this stage
		unsigned chunkNum;
		MicroServPathNode* chunkSendNode;

		/**** thread blocking ****/
		// if thread is blocked by this job
		bool threadBlocked;

	public:
		JobRecord(unsigned path_id, unsigned node_id, unsigned serv_id, unsigned sync_num, Time initime);

	private:
		void assignThread(unsigned thread_id);
};

#endif