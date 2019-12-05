#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <stdio.h>

#include "cluster_parser.hh"
#include "cluster.hh"
#include "client.hh"

std::string sched_dec_file = "./sched_test/sched_decision.txt";
std::string stats_output_file = "./sched_test/simulated_stats.txt";

// return true if decision is ready
bool get_sched_decision(int cur_round, std::unordered_map<std::string, unsigned>& freq_set) {
	std::ifstream dec_file(sched_dec_file);
	if(!dec_file) {
		// std::cout << "cannot open " << sched_dec_file << std::endl;
		return false;
	}
	char buffer[256];
	unsigned memc = 0;
	unsigned ngx = 0;
	int sched_round = -1;
	while(!dec_file.eof()) {
		dec_file.getline(buffer, 256);
		sscanf(buffer, "nginx: %u; memcached: %u; cur_round: %d", &ngx, &memc, &sched_round);
	}
	dec_file.close();
	if(sched_round != cur_round) {
		// printf("sched_round = %d, cur_round = %d\n", sched_round, cur_round);
		return false;
	} else {
		freq_set["nginx"] = ngx;
		freq_set["memcached"] = memc;
		return true;
	}
}

void output_stats(int cur_round, Time end2end_tail, Time ngx_tail, Time memc_tail, uint64_t cur_qps) {
	std::ofstream stats_file(stats_output_file);
	if(!stats_file) {
		std::cout << "cannot open " << sched_dec_file << std::endl;
		exit(-1);
	}

	std::cout << "output stats" << std::endl;
	stats_file << "end2end: " << end2end_tail << "; nginx: " << ngx_tail 
		<< "; memcached :" << memc_tail << "; cur_qps: " << cur_qps  << "; cur_round: " << cur_round << std::endl;
	stats_file.close();
	std::cout << "end2end: " << end2end_tail << "; nginx: " << ngx_tail 
		<< "; memcached :" << memc_tail << "; cur_qps: " << cur_qps  << "; cur_round: " << cur_round << std::endl;
}

int main(int argc, char* argv[]) {
	assert(argc >= 4);
	std::string clusterDir = std::string(argv[1]);
	unsigned numConn = atoi(argv[2]); 
	Time netLat = atoi(argv[3]);

	// std::string raplPolicy = std::string(argv[4]);
	// std::string raplFallbackPolicy = std::string(argv[5]);

	// client config
	std::string cliTm = std::string(argv[4]);
	// define load in terms of rps
	// convert it into us
	// double avgCliLat = atof(argv[6]);

	double kqps = atof(argv[5]);
	std::cout << "user given qps = " << (unsigned)(kqps*1000) << std::endl;

	// set up files
	FILE* f = fopen(stats_output_file.c_str(), "w+");
	if(f == nullptr) {
		std::cout << "Cannot create " << stats_output_file << std::endl;
		exit(-1);
	}
	fclose(f);

	f = fopen(sched_dec_file.c_str(), "w+");
	if(f == nullptr) {
		std::cout << "Cannot create " << sched_dec_file << std::endl;
		exit(-1);
	}
	fclose(f);

	// unsigned ngx_freq = atoi(argv[6]);
	// unsigned memc_freq = atoi(argv[7]);

	// debug
	bool debug = false;
	if(argc == 7 && std::string(argv[6]) == "debug")
		debug = true;

	ClusterParser* parser = new ClusterParser(clusterDir, kqps, debug);

	std::cout << "before parsing" << std::endl;

	Cluster* cluster = parser->parsCluster();
	// delete parser;

	// cluster->setFreq("nginx", ngx_freq);
	// cluster->setFreq("memcached", memc_freq);

	std::cout << "after parsing" << std::endl;

	// set up client
	// Client* client = new Client( numConn, netLat, debug);

	// Client* client = parser->parsClient(raplPolicy, raplFallbackPolicy, numConn, netLat, debug);
	Client* client = parser->parsClient(numConn, netLat, kqps, debug);
	// client->numTotal = 1;


	TimeModel* tm = nullptr;
	if(cliTm == "expo") {
		tm = new ExpoTimeModel(1);
	} else if (cliTm == "const") {
		tm = new ConstTimeModel(1);
	} else {
		printf("Error: Undefined time model for client\n");
		exit(1);
	}
	client->setTimeModel(tm);

	client->setEntry(cluster);
	client->setTimeArray();

	std::cout << "simulation starts..." << std::endl;
	unsigned cur_round = 0;

	// simulation
	Time globalTime = 0;
	while(true) {
		Time nextClient = client->nextEventTime();
		Time nextCluster = cluster->nextEventTime(globalTime);

		if(nextCluster == INVALID_TIME && client->isAllJobIssued()) {
			// all jobs have finished processing
			// only perf monitoring events are left
			break;
		}

		if(debug) {
			printf("nextClient = %f ms\n", nextClient/1000000.0);
			printf("nextCluster = %f ms\n", nextCluster/1000000.0);
		}

		if(nextClient == INVALID_TIME && nextCluster == INVALID_TIME)
			break;
		else if(nextCluster == INVALID_TIME) {
			// std::cout << "cluster invalid time" << std::endl;
			// std::cout << "before client run" << std::endl;
			// std::cout << "globalTime = " << globalTime << std::endl;

			assert(globalTime <= nextClient);
			globalTime = nextClient;

			if(client->needSched(globalTime)) {
				std::cout << "client make decision at " << nextClient/1000000000.0 << "s, cur_round = "
						<< cur_round << std::endl;

				std::unordered_map<std::string, Time> lat_info;
				cluster->getPerTierTail(lat_info);
				Time end2end_tail = client->getTailLat();
				client->clearRespTime();
				uint64_t cur_qps = client->getCurQps();
				output_stats(cur_round, end2end_tail, lat_info["nginx"], lat_info["memcached"], cur_qps);

				std::unordered_map<std::string, unsigned> freq_set;
				while(!get_sched_decision(cur_round, freq_set));
				cur_round += 1;
				std::cout << "decision made nginx = " << freq_set["nginx"] << 
					", memcached = " << freq_set["memcached"] << std::endl << std::endl; 
				cluster->setFreq("nginx", freq_set["nginx"]);
				cluster->setFreq("memcached", freq_set["memcached"]);
				client->setLastMonitorTime(globalTime);
			}
			
			client->run(globalTime);
			// std::cout << "client run" << std::endl << std::endl;
		} else if(nextClient == INVALID_TIME) {
			// std::cout << "client invalid time" << std::endl;
			// std::cout << "before cluster run" << std::endl;
			// std::cout << "globalTime = " << globalTime << std::endl;

			assert(globalTime <= nextCluster);
			globalTime = nextCluster;

			if(client->needSched(globalTime)) {
				std::cout << "client make decision at " << nextClient/1000000000.0 << "s, cur_round = "
						<< cur_round << std::endl;

				std::unordered_map<std::string, Time> lat_info;
				cluster->getPerTierTail(lat_info);
				Time end2end_tail = client->getTailLat();
				client->clearRespTime();
				uint64_t cur_qps = client->getCurQps();
				output_stats(cur_round, end2end_tail, lat_info["nginx"], lat_info["memcached"], cur_qps);

				std::unordered_map<std::string, unsigned> freq_set;
				while(!get_sched_decision(cur_round, freq_set));
				cur_round += 1;
				std::cout << "decision made nginx = " << freq_set["nginx"] << 
					", memcached = " << freq_set["memcached"] << std::endl << std::endl; 
				cluster->setFreq("nginx", freq_set["nginx"]);
				cluster->setFreq("memcached", freq_set["memcached"]);
				client->setLastMonitorTime(globalTime);
			}
			
			cluster->run(globalTime);
			// std::cout << "cluster run" << std::endl << std::endl;
		} else {
			// std::cout << "cluster and client valid time " << std::endl;
			// std::cout << "globalTime = " << globalTime << std::endl;
			assert(globalTime <= nextClient);
			assert(globalTime <= nextCluster);
			if(nextClient < nextCluster) {
				globalTime = nextClient;
				// check if needs scheduling
				if(client->needSched(globalTime)) {
					std::cout << "client make decision at " << nextClient/1000000000.0 << "s, cur_round = "
						<< cur_round << std::endl;

					std::unordered_map<std::string, Time> lat_info;
					cluster->getPerTierTail(lat_info);
					Time end2end_tail = client->getTailLat();
					client->clearRespTime();
					uint64_t cur_qps = client->getCurQps();
					output_stats(cur_round, end2end_tail, lat_info["nginx"], lat_info["memcached"], cur_qps);

					std::unordered_map<std::string, unsigned> freq_set;
					while(!get_sched_decision(cur_round, freq_set));
					cur_round += 1;
					std::cout << "decision made nginx = " << freq_set["nginx"] << 
						", memcached = " << freq_set["memcached"] << std::endl << std::endl; 
					cluster->setFreq("nginx", freq_set["nginx"]);
					cluster->setFreq("memcached", freq_set["memcached"]);
					client->setLastMonitorTime(globalTime);
				}
				// std::cout << "before client run" << std::endl;
				client->run(globalTime);
				// std::cout << "client run" << std::endl << std::endl;
			} else {
				globalTime = nextCluster;
				// std::cout << "before cluster run" << std::endl;
				cluster->run(globalTime);
				// std::cout << "cluster run" << std::endl << std::endl;
			}
		}
	}

	// check for deadlock
	bool deadlock = cluster->jobLeft();
	assert(!deadlock);

	client->show();
	cluster->showCpuUtil(globalTime);

	delete client;
	delete cluster;
	delete parser;
	return 0;
}