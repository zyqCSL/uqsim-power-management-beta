#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <iterator>
#include <math.h>

#include "stage.hh"

/***************** ConnState **********************/
ConnState::ConnState(): valid(false), connId(0), blocked(false), jid(0) {}

/***************** Stage **********************/
Stage::Stage(unsigned servId, unsigned stageId, unsigned pstgId, const std::string& stgName, unsigned pathId, 
			 bool blk, bool batch, bool sock, bool epoll, bool ngx, bool chk, bool net, bool cri_sec, unsigned thd_lmt,
			 unsigned base_freq, double scale_factor, bool debug): 
			 servId(servId), stageId(stageId), pathStageId(pstgId), stageName(stgName), pathId(pathId), nextStage(nullptr), 
			 blocking(blk), batching(batch), socket(sock), epoll(epoll), ngxProc(ngx), chunk(chk), net(net), criSec(cri_sec), thdLmt(thd_lmt), thdCnt(nullptr),
			 reqProcTm(nullptr), respProcTm(nullptr), cm(nullptr),  debug(debug), scaleFactor(scale_factor), baseFreq(base_freq), curFreq(base_freq) {
	if(blocking)
		assert(!batching);
	if(socket)
		assert(batching);
	// currently chunk stage does not support batching
	if(chunk)
		assert(!batching);

	// for sock read stages
	if(isSocket()) {
		packetSize = 7;
		// unsigned bytes[] = {53, 54, 108, 162, 324, 594, 648, 864, 1404, 1512, 
		// 	1620, 1998, 2052, 2538, 2970, 3348, 32754, 131072};

		// // in nanoseconds
		// Time lat[] = {3200, 3500, 7000, 8000, 10000, 11000,  12000, 13000, 14000, 15000, 
		// 	16000, 17000, 18500, 19500, 21500, 22500, 200000, 550000};


		unsigned bytes[] = {35, 70, 270, 786, 1836, 2268};

		// in nanoseconds
		Time lat[] = {800, 1600, 3000, 5400, 11600, 26000};

		readBytes = std::vector<unsigned> (std::begin(bytes), std::end(bytes));
		readLatency = std::vector<Time> (std::begin(lat), std::end(lat));

		baselineReadLatency = std::vector<Time> (readLatency);

		assert(readBytes.size() == readLatency.size());
	}

	if(isNgxProc())
		srand(time(NULL));
	// epoll proc time is implemented in getProcTime

	// ngx proc stage time
	ngx_proc.resize(100);
	ngx_proc.resize(100);
	for(int i = 0; i <= 4; ++i)
		ngx_proc[i] = i * 1000 + 17000;

	for(int i = 5; i <= 7; ++i)
		ngx_proc[i] = 22000;

	for(int i = 8; i <= 10; ++i)
		ngx_proc[i] = 23000;

	for(int i = 11; i <= 13; ++i)
		ngx_proc[i] = 24000;

	for(int i = 14; i <= 18; ++i)
		ngx_proc[i] = 25000;

	for(int i = 19; i <= 24; ++i)
		ngx_proc[i] = 26000;

	for(int i = 25; i <= 31; ++i)
		ngx_proc[i] = 27000;

	for(int i = 32; i <= 39; ++i)
		ngx_proc[i] = 28000;

	for(int i = 40; i <= 48; ++i)
		ngx_proc[i] = 29000;

	for(int i = 49; i <= 56; ++i)
		ngx_proc[i] = 30000;

	for(int i = 57; i <= 64; ++i)
		ngx_proc[i] = 31000;

	for(int i = 65; i <= 71; ++i)
		ngx_proc[i] = 32000;

	for(int i = 72; i <= 78; ++i)
		ngx_proc[i] = 33000;

	for(int i = 79; i <= 83; ++i)
		ngx_proc[i] = 34000;

	for(int i = 84; i <= 87; ++i)
		ngx_proc[i] = 35000;

	for(int i = 88; i <= 90; ++i)
		ngx_proc[i] = 36000;

	for(int i = 91; i <= 92; ++i)
		ngx_proc[i] = 37000;

	for(int i = 93; i <= 94; ++i)
		ngx_proc[i] = 38000;

	ngx_proc[95] = 39000;
	ngx_proc[96] = 40000;
	ngx_proc[97] = 43000;
	ngx_proc[98] = 48000;
	ngx_proc[99] = 100000;

	for(unsigned i = 0; i < ngx_proc.size(); ++i)
		// 0.65 is for 8-process nginx
		// change 0.65 to 0.64 when simulating 4-process nginx
		ngx_proc[i] = Time(ngx_proc[i] * 0.65);

	baseline_ngx_proc = std::vector<Time> (ngx_proc);

	ngx_proc_resp = std::vector<Time>(ngx_proc);
	baseline_ngx_proc_resp = std::vector<Time>(ngx_proc);

	assert(ngx_proc_resp.size() == ngx_proc.size());

	// connection states
	numConn = 500;
	connStates.resize(numConn);
	for(unsigned i = 0; i < numConn; ++i)
		connStates[i] = ConnState();

	// std::cout << "curFreq " << curFreq << " in stage " << stageName << std::endl;
}

Stage::~Stage() {
	delete reqProcTm;
	delete respProcTm;
	delete cm;

	for(auto itr = procTmMap.begin(); itr != procTmMap.end(); ++itr) {
		delete itr->second;
	}
}

unsigned
Stage::getStageId() {
	return stageId;
}

unsigned
Stage::getPathStageId() {
	return pathStageId;
}

const std::string&
Stage::getStageName() {
	return stageName;
}

void
Stage::setPathStageId(unsigned id) {
	pathStageId = id;
}

bool 
Stage::isBlocking() {
	return blocking;
}

bool
Stage::isBatching() {
	return batching;
}

bool
Stage::isSocket() {
	return socket;
}

bool
Stage::isEpoll() {
	return epoll;
}

bool
Stage::isNgxProc() {
	return ngxProc;
}

bool
Stage::isCriSec() {
	return criSec;
}

void
Stage::setCriSec(unsigned* num_thds) {
	thdCnt = num_thds;
}

bool 
Stage::criSecAvail() {
	if(!criSec)
		return true;
	else
		return (*thdCnt > 0);
}

unsigned*
Stage::getThdCnt() {
	assert(criSec);
	assert(thdCnt != nullptr);
	return thdCnt;
}

unsigned 
Stage::getThdLmt() {
	return thdLmt;
}

void
Stage::setTimeModel(TimeModel* req_tm, TimeModel* resp_tm) {
	reqProcTm = req_tm;
	respProcTm = resp_tm;
}

void
Stage::setChunkModel(ChunkModel* c) {
	cm = c;
}

void 
Stage::setNextStage(Stage* s) {
	nextStage = s;
}

void 
Stage::setFreq(unsigned freq) {
	if(freq == curFreq) {
		// std::cout << "In Stage " << stageName << " freq already equals " << freq << ", " << stageName << std::endl;
		return;
	}

	// std::cout << "In Stage " << stageName << " freq set to " << freq << ", " << stageName << std::endl;

	curFreq = freq;
	if(isSocket()) {
		for(unsigned i = 0; i < readLatency.size(); ++i)
			readLatency[i] = (Time)(1.0 - scaleFactor + scaleFactor * (double)baseFreq/(double)curFreq)*baselineReadLatency[i];
	} else if(isNgxProc()) {
		for(unsigned i = 0; i < ngx_proc.size(); ++i)
			ngx_proc[i] = (Time)(0.6 + 0.4 * (double)baseFreq/(double)curFreq)*baseline_ngx_proc[i];
		for(unsigned i = 0; i < ngx_proc_resp.size(); ++i)
			ngx_proc_resp[i] = (Time)(1.0 * (double)baseFreq/(double)curFreq)*baseline_ngx_proc_resp[i];
	} else if(isEpoll()) {
		// dealt with in getProcTime
		return;
	} else {
		// normal stage
		assert(reqProcTm != nullptr);
		reqProcTm->setFreq(baseFreq, curFreq, scaleFactor);
	}
}

void
Stage::incFullFreq() {
	setFreq(baseFreq);
}

void
Stage::incFreq() {
	if(curFreq == baseFreq)
		return;
	assert(curFreq + 200 <= baseFreq);
	setFreq(curFreq + 200);
}

void
Stage::decFreq() {
	if(curFreq == 1200)
		return;
	unsigned freq = curFreq - 200;
	assert(freq >= 1200);
	setFreq(freq);
}

Time
Stage::run(Time globalTime, Job* j, Time proc_time) {

	if(!j->in_nic) {
		assert(j->syncDone());
		assert(j->getStage() == pathStageId);
	}

	unsigned connId = j->connId;

	// if(!connStates[j->connId].valid) {
	// 	connStates[j->connId].connId  = connId;
	// 	connStates[j->connId].valid   = true;
	// 	connStates[j->connId].blocked = false;
	// }

	// // cannot execute a job from a blocked connection execept the blocking job
	// // only model connection blocking for ngx proc stage
	if(isNgxProc()) {
		j->ngxProc = true;
		// if(connStates[connId].blocked == false) {
		// 	// recv job from client
		// 	connStates[connId].blocked = true;
		// 	connStates[connId].jid = j->id;
		// 	if(debug)
		// 		std::cout << "in nginx proc, conn:" << connId << " blocked by job:" << j->id << std::endl;
		// } else {
		// 	// recv job resp downstream services
		// 	assert(j->id == connStates[connId].jid);
		// 	connStates[connId].blocked = false;
		// 	if(debug)
		// 		std::cout << "in nginx proc, conn:" << connId << " un-blocked by job (resp):" << j->id << std::endl;
		// }
	}

	// // lock contention
	// assert(criSecAvail());
	// if(criSec)
	// 	*thdCnt -= 1;

	// don't consider chunk processing
	// if(chunk) {
	// 	// it is possible that a job passed a chunking stage without setting chunk
	// 	if(!j->chunkSet() && j->waitReq()) {
	// 		assert(cm != nullptr);
	// 		unsigned chunkTimes = cm->chunkNum();
	// 		if(chunkTimes > 1) {
	// 			bool chunkInternal = j->setChunk(chunkTimes);
	// 			if(chunkInternal)
	// 				j->decChunkNum();
	// 			if(debug)
	// 				printf("In Stage::run() job: %lu, chunkNum set to %lu\n", j->id, j->getChunkNum());
	// 		} else if(debug) 
	// 			printf("In Stage::run() job: %lu, ignore chunk since chunkNum = 0\n", j->id);
	// 	} else if(j->waitResp() && j->chunkSet()) {
	// 		j->decChunkNum();
	// 		if(j->chunkDone()) {
	// 			j->clearChunk();
	// 			if(debug)
	// 				printf("In Stage::run() job: %lu chunk process complete\n", j->id);
	// 		}
	// 	}
	// } else
	// 	assert(!j->chunkSet());

	// if(!j->in_nic) {
	assert(j->time <= globalTime);
	j->time = globalTime;
	// }

	if(debug)
		printf("In Stage::run() in stage: %s, path: %u, pathStageId: %u, stageId: %u, jid: %lu starts running at time: %lu ns\n", 
			stageName.c_str(), pathId, pathStageId, stageId, j->id, globalTime);

	// printf("In Stage::run() in stage: %s, path: %lu, pathStageId: %lu, stageId: %lu, jid: %lu starts running at time: %.3f\n", 
	// 		stageName.c_str(), pathId, pathStageId, stageId, j->id, globalTime);

	Time procTime = INVALID_TIME;
	// if(proc_time != INVALID_TIME)
	// 	procTime = proc_time;
	// else if(j->in_nic || j->waitReq()) {
	// 	// std::cout << "request proc" << std::endl;
	// 	assert(reqProcTm != nullptr);
	// 	procTime = reqProcTm->lat();
	// } else {
	// 	// std::cout << "resp proc" << std::endl;
	// 	assert(respProcTm != nullptr);
	// 	procTime = respProcTm->lat();
	// }

	if(proc_time != INVALID_TIME)
		procTime = proc_time;
	else {
		// std::cout << "resp proc" << std::endl;
		assert(reqProcTm != nullptr);
		if(cm != nullptr) {
			procTime = 0;
			unsigned chunk_num = cm->chunkNum();
			while(chunk_num != 0) {
				procTime += reqProcTm->lat();
				--chunk_num;
			}
		} else 
			procTime = reqProcTm->lat();
	}

	if(debug)
		printf("processing time set to %lu ns\n", procTime);

	j->time += procTime;

	if(debug) 
		printf("job: %lu exe should end at %lu ns\n", j->id, j->time);

	// thread blocking done in thread block
	// if(blocking && j->needSync())
	// 	j->blockThread();

	j->tailStage = (nextStage == nullptr);
	j->net = net;

	// std::cout << "job processing at stage ends" << std::endl;

	return procTime;
}

// only used for batching stages
Time 
Stage::getProcTime(int batchedJobs, bool ngx_proc_already) {
	assert(isBatching() || isNgxProc());
	if(isBatching())
		assert(batchedJobs > 0);

	// currently only deal with socket stage
	unsigned key = 0;
	unsigned readTime = 1;
	if(!isSocket() && !isEpoll() && !isNgxProc() ) {
		if(cm == nullptr)
			return reqProcTm->lat();
		else {
			Time procTime = 0;
			unsigned chunk_num = cm->chunkNum();
			while(chunk_num != 0) {
				procTime += reqProcTm->lat();
				--chunk_num;
			}
			return procTime;
		}
	} else if(isEpoll()) {
		// std::cout << "in epoll getProcTime" << std::endl;
		Time time = round(0.38*(batchedJobs - 7) + 3.8) * 1000;
		if(curFreq != baseFreq)
			time = (Time)(time*(1.0 - scaleFactor + scaleFactor * (double)baseFreq/(double)curFreq));
		auto itr = procTmMap.find(time);
		if(itr == procTmMap.end()) {
			// std::cout << "new tm" << std::endl;
			procTmMap[time] = new ExpoTimeModel(time);
		}
		return procTmMap[time]->lat();

	} else if (isNgxProc()) {
		int num = rand() % 100;
		if(!ngx_proc_already)
			return (Time)(ngx_proc[num]);
		else
			return (Time)(ngx_proc_resp[num]);
	} else {

		unsigned bytes = batchedJobs * packetSize;

		// std::cout << "sock read bytes = " << bytes << std::endl;

		unsigned i;
		for(i = 0; i < readBytes.size(); ++i) {
			if(readBytes[i] >= bytes) {
				key = readBytes[i];
				break;
			}
		}

		if(i == readBytes.size()) {
			--i;
			key = readBytes[readBytes.size() - 1];
			readTime = bytes/key;
			if(readTime == 0)
				readTime = 1;
		}

		// approximate
		assert(key != 0);
		auto itr = procTmMap.find(key);
		if(itr == procTmMap.end()) {
			// std::cout << "new tm" << std::endl;
			procTmMap[key] = new ExpoTimeModel(readLatency[i]);
		}

		Time total = 0;
		while(readTime != 0) {
			// std::cout << "proc once" << std::endl;
			total += procTmMap[key]->lat();
			--readTime;
		}

		// std::cout << "sock read time = " << total << std::endl;

		return total;
	}
}
