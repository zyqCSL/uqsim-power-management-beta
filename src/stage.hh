#ifndef _STAGE_HH_
#define _STAGE_HH_

#include <vector>
#include <string>
#include <list>
#include <unordered_map>

#include "job.hh"
#include "time_model.hh"
#include "chunk_model.hh"

struct ConnState;
class Stage;

struct ConnState 
{
	bool valid;
	unsigned connId;
	bool blocked;
	// job id that blocks this connection
	uint64_t jid;

	ConnState();
};

class Stage
{
	protected:
		unsigned servId;
		// stage id within the module
		unsigned stageId;
		// stage id within the path
		unsigned pathStageId;

		std::string stageName;

		// the path this stage belongs to
		unsigned pathId;

		Stage* nextStage;

		// if a job waiting for i/o will block downstream jobs
		bool blocking;
		// batch all requests in queues
		bool batching;
		// if this stage is socket read, batch on connection granularity
		bool socket;
		// if this stage is epoll or not
		bool epoll;

		// if is nginx's processing stage
		bool ngxProc;
		// if processing at his modules is splitted into multiple chunks
		bool chunk;
		// if request issued from this stage goes through network
		bool net;

		/*** critical section ***/
		bool criSec;
		// # threads allowed in the critical section
		unsigned thdLmt;
		// # threads currenty in the critical section
		unsigned* thdCnt;

		/*** timing model ***/
		TimeModel* reqProcTm;
		TimeModel* respProcTm;

		// for batching stages
		// map #batched jobs to tms
		std::unordered_map<unsigned, TimeModel*> procTmMap;
		// for sock read requests
		std::vector<unsigned> readBytes;
		std::vector<Time> baselineReadLatency;
		std::vector<Time> readLatency;
		unsigned packetSize;

		// for ngx proc time test
		std::vector<Time> baseline_ngx_proc;
		std::vector<Time> ngx_proc;

		std::vector<Time> baseline_ngx_proc_resp;
		std::vector<Time> ngx_proc_resp;

		ChunkModel* cm;

		bool debug;

		// for dvfs scaling
		// part of time that scales linearly with ratio of 1, wrt frequency
		double scaleFactor;
		// the baseline frequency that determines default speed
		unsigned baseFreq;
		unsigned curFreq;

	public:
		// connection stage tracking
		unsigned numConn;
		std::vector<ConnState> connStates;

	public:
		Stage(unsigned servId, unsigned stageId, unsigned pstgId, const std::string& stgName, unsigned pathId, 
			 bool blk, bool batch, bool sock, bool epoll, bool ngx, bool chk, bool net, bool cri_sec, unsigned thd_lmt,
			 unsigned base_freq, double scale_factor, bool debug);

		virtual ~Stage();

		unsigned getStageId();
		unsigned getPathStageId();
		void setPathStageId(unsigned id);
		const std::string& getStageName();

		bool isBlocking();

		bool isBatching();
		bool isSocket();
		bool isEpoll();
		bool isNgxProc();

		bool isCriSec();
		void setCriSec(unsigned* num_thds);
		bool criSecAvail();
		unsigned* getThdCnt();
		unsigned getThdLmt();

		void setTimeModel(TimeModel* req_tm, TimeModel* resp_tm);

		void setChunkModel(ChunkModel* c);
		void setNextStage(Stage* s);

		// for dvfs
		// right now setFreq must be called before simulation starts
		void setFreq(unsigned freq);

		void incFullFreq();
		void incFreq();
		void decFreq();

		// queue is the input queue of this stage
		// @queue: input queue
		// @pendingQ: jobs to be sent to next stage
		// @sendQ jobs to be sent to outside micro services
		// @return the processing time of the job
		virtual Time run(Time globalTime, Job* j, Time proctime);
		virtual Time getProcTime(int batchedJobs, bool ngx_proc_already);
};

#endif
