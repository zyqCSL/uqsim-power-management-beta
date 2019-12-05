#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

#include "cluster_parser.hh"

const std::string ClusterParser::defServDir = "./architecture/default/microservice/";

ClusterParser::ClusterParser(const std::string& cluster_dir, double kqps, bool debug): 
	kqps(kqps), debug(debug) {
	clusterDir = cluster_dir;
	eq = new EventQueue();
	// read all micro service architecture into the map
	std::string servArchFile = clusterDir + "/microservice.json";
	std::string servArchDir = clusterDir + "/microservice/";
	struct stat path_stat;
	/*** read in all user specified service architecture ***/
	// have a single json file microservice.json in cluster arch directory
	if(stat(servArchFile.c_str(), &path_stat) == 0 && S_ISREG(path_stat.st_mode) != 0) {
		// micro-service spec file exists and it's a file
		Json::Reader reader;
		Json::Value services;
		std::ifstream sf(servArchFile);
		reader.parse(sf, services);
		for(unsigned i = 0; i < services.size(); ++i) {
			assert(services[i].isMember("service_name"));
			std::string servName = services[i]["service_name"].asString();
			// duplicate definition not allowed
			assert(servArch.find(servName) == servArch.end());
			servArch[servName] = services[i];
		}
		
	} else if(stat(servArchDir.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode) != 0) {
		// for each microservice, have a json file in cluster/microservice/ directory
		// check if spec diretory exists
		DIR* dir = opendir(servArchDir.c_str());
		if(dir == nullptr) {
			printf("Error: missing %s\n", servArchDir.c_str());
			assert(dir != nullptr);
		}

		assert(dir != nullptr);
		struct dirent* entry = readdir(dir);
		while(entry != nullptr) {
			std::string fname = std::string(entry->d_name);
			if(fname.find(".json") != std::string::npos) {
				std::string path = servArchDir + "/" + fname;
				Json::Reader reader;
				Json::Value service;
				std::ifstream sf(path);
				reader.parse(sf, service);
				assert(service.isMember("service_name"));
				std::string servName = service["service_name"].asString();
				assert(servArch.find(servName) == servArch.end());
				servArch[servName] = service;
			}
			entry = readdir(dir);
		}
	}

	// linux net stack model must exist
	if(servArch.find("net_stack") == servArch.end()) {
		// search default directory
		std::string servFname = "net_stack.json";
		DIR* dir = opendir(defServDir.c_str());
		if(dir == nullptr) {
			printf("Error: missing %s\n", servFname.c_str());
			assert(dir != nullptr);
		}

		struct dirent* entry = readdir(dir);
		bool found = false;
		while(entry != nullptr) {
			std::string fname = std::string(entry->d_name);
			if(fname == servFname) {
				Json::Reader reader;
				Json::Value servSpec;
				std::string fullPath = defServDir + "/" + servFname;
				reader.parse(fullPath, servSpec);
				// cache the spec
				servArch["net_stack"] = servSpec;
				found = true;
				break;
			}
			entry = readdir(dir);
		}

		if(!found) {
			printf("Error: missing NET_STACK model\n");
			exit(-1);
		}
	}
}

MicroService* 
ClusterParser::parsMicroServ(const std::string& servName, const std::string& servDomain, const std::string& instName, Json::Value& serv) {
	// add to name->id map
	assert(servNameIdMap.find(instName) == servNameIdMap.end());
	servNameIdMap[instName] = servId;

	assert(serv.isMember("type"));
	std::string type = serv["type"].asString();
	bool loadBal = (type == "load_balancer");
	assert(serv.isMember("bind_connection"));
	bool bindConn = serv["bind_connection"].asBool();

	// dvfs
	assert(serv.isMember("base_freq"));
	unsigned baseFreq = serv["base_freq"].asUInt();
	assert(serv.isMember("cur_freq"));
	unsigned curFreq = serv["cur_freq"].asUInt();

	MicroService* ms = nullptr;
	if(loadBal)
		ms = new LoadBalancer(servId, instName, servName, servDomain, bindConn, debug, eq);
	else if(servName == "net_stack" || servName.find("net_stack") != std::string::npos) {
		ms = new NetStack(servId, instName, servName, servDomain, debug, eq);
	} else
		ms = new MicroService(servId, instName, servName, servDomain, loadBal, bindConn, baseFreq, curFreq, debug, eq);
	// code paths
	std::vector<unsigned> pathProbDist;
	unsigned probSetNum = 0;
	assert(serv.isMember("paths"));
	for(unsigned i = 0; i < serv["paths"].size(); ++i) {
		Json::Value& pathSpec = serv["paths"][i];
		assert(pathSpec.isMember("code_path_id"));
		unsigned pathId = pathSpec["code_path_id"].asUInt();
		// find the probablity of the path here to maintain matching between path & prob
		if(pathSpec.isMember("probability")) {
			probSetNum += 1;
			pathProbDist.push_back(pathSpec["probability"].asUInt());
		}

		assert(pathSpec.isMember("num_stages"));
		unsigned numStages = pathSpec["num_stages"].asUInt();
		CodePath codePath(pathId, numStages);
		assert(pathSpec.isMember("stages"));
		for(unsigned j = 0; j < pathSpec["stages"].size(); ++j) {
			Json::Value& stageSpec = pathSpec["stages"][j];

			assert(stageSpec.isMember("path_stage_id"));
			unsigned pathStageId = stageSpec["path_stage_id"].asUInt();

			assert(stageSpec.isMember("stage_id"));
			unsigned stageId = stageSpec["stage_id"].asUInt();

			assert(stageSpec.isMember("blocking"));
			bool blocking = stageSpec["blocking"].asBool();

			assert(stageSpec.isMember("batching"));
			bool batching = stageSpec["batching"].asBool();

			assert(stageSpec.isMember("socket"));
			bool socket = stageSpec["socket"].asBool();

			assert(stageSpec.isMember("epoll"));
			bool epoll = stageSpec["epoll"].asBool();

			assert(stageSpec.isMember("ngx_proc"));
			bool ngx = stageSpec["ngx_proc"].asBool();

			assert(stageSpec.isMember("net"));
			bool net = stageSpec["net"].asBool();

			assert(stageSpec.isMember("critical_section"));
			bool criSec = stageSpec["critical_section"].asBool();

			// dvfs
			assert(stageSpec.isMember("scale_factor"));
			double scaleFactor = stageSpec["scale_factor"].asDouble();

			assert(stageSpec.isMember("stage_name"));
			std::string stageName = stageSpec["stage_name"].asString();
			unsigned thdLmt = 1;
			if(criSec) {
				assert(stageSpec.isMember("thread_limit"));
				thdLmt = stageSpec["thread_limit"].asUInt();
			}
			assert(stageSpec.isMember("chunk"));
			bool chunk = stageSpec["chunk"].asBool();
			bool _chunk = false;
			Stage* stage = new Stage(servId, stageId, pathStageId, stageName, pathId, blocking, batching,
				socket, epoll, ngx, _chunk, net, criSec, thdLmt,
				baseFreq, scaleFactor, debug);
			// set time model
			// recv time model
			assert(stageSpec.isMember("recv_time_model"));
			Json::Value& recvTmSpec = stageSpec["recv_time_model"];
			TimeModel* recvTm = nullptr;
			if(recvTmSpec["type"].asString() == "expo") {
				assert(recvTmSpec.isMember("latency"));
				Time latency = recvTmSpec["latency"].asUInt();
				if(servName == "mongodb" && stageName == "proc_cache_hit") {
					latency = latency * 5000/(1000 + kqps * 1000);
					std::cout << "tune mongodb latency = " << latency << std::endl;
				}

				if(servName == "mongo_io" && stageName == "disk_io") {
					unsigned qps = unsigned(qps * 1000);
					if(qps < 4000)
						latency = latency * 5000/(1000 + kqps * 1000);
					// if(qps < 1000)
					// 	latency = latency * 5000/(1000 + kqps * 1000);
					std::cout << "tune mongo_io latency = " << latency << std::endl;
				}

				recvTm = new ExpoTimeModel(latency);
			} else if (recvTmSpec["type"].asString() == "const") {
				assert(recvTmSpec.isMember("latency"));
				Time latency = recvTmSpec["latency"].asUInt();
				recvTm = new ConstTimeModel(latency);
			} else {
				printf("Error: Undefined TimeModel Type: %s\n", recvTmSpec["type"].asString().c_str());
				exit(1);
			}
			// resp time model
			TimeModel* respTm = nullptr;
			if(stageSpec.isMember("resp_time_model")) {
				Json::Value& respTmSpec = stageSpec["resp_time_model"];
				if(respTmSpec["type"].asString() == "expo") {
					// std::cout << "respTm type = const" << std::endl; 
					assert(respTmSpec.isMember("latency"));
					Time latency = respTmSpec["latency"].asUInt();
					respTm = new ExpoTimeModel(latency);
				} else if (respTmSpec["type"].asString() == "const") {
					// std::cout << "respTm type = expo" << std::endl; 
					assert(respTmSpec.isMember("latency"));
					Time latency = respTmSpec["latency"].asUInt();
					respTm = new ConstTimeModel(latency);
				} else {
					printf("Error: Undefined TimeModel Type: %s\n", respTmSpec["type"].asString().c_str());
					exit(1);
				}
			}
			stage->setTimeModel(recvTm, respTm);

			// set chunk processing
			if(chunk) {
				ChunkModel* cm = nullptr;
				Json::Value& chunkSpec = stageSpec["chunk_model"];
				if(chunkSpec["type"].asString() == "expo") {
					assert(chunkSpec.isMember("number"));
					double num = chunkSpec["number"].asUInt();
					cm = new ExpoChunkModel(num);
				} else if (chunkSpec["type"].asString() == "const") {
					assert(chunkSpec.isMember("number"));
					unsigned num = chunkSpec["number"].asUInt();
					cm = new ConstChunkModel(num);
				} else {
					printf("Error: Undefined ChunkModel Type: %s\n", chunkSpec["type"].asString().c_str());
					exit(1);
				}
				stage->setChunkModel(cm);
			}

			// set priority of stages within the path
			if(pathSpec.isMember("priority")) {
				std::vector<unsigned> priority;
				for(unsigned p = 0; p < pathSpec["priority"].size(); ++p)
					priority.push_back(pathSpec["priority"][p].asUInt());
				codePath.setPriority(priority);
			}

			// dvfs
			stage->setFreq(curFreq);

			// add stage to path
			codePath.addStage(stage);
		}

		// add path to microservice
		ms->addPath(codePath);
	}

	assert(probSetNum == 0 || probSetNum == serv["paths"].size());
	if(probSetNum != 0) 
		// set CodePath prob distribution
		ms->setPathDistr(pathProbDist);

	++servId;
	return ms;
}

MicroServPath
ClusterParser::parsServPath(Json::Value& servPath) {
	assert(servPath.isMember("entry"));
	unsigned entry = servPath["entry"].asUInt();

	assert(servPath.isMember("micro_service_path_id"));
	unsigned id = servPath["micro_service_path_id"].asUInt();
	MicroServPath path(id, entry);

	assert(servPath.isMember("nodes"));
	Json::Value& allNodeSpec = servPath["nodes"];
	// node->child map
	std::unordered_map<unsigned, std::vector<unsigned> > nodeIdChildsMap;
	// node->syncNum map
	std::unordered_map<unsigned, unsigned> nodeIdSyncNumMap;
	// first round
	for(unsigned i = 0; i < allNodeSpec.size(); ++i) {
		Json::Value& nodeSpec = allNodeSpec[i];
		// node content
		assert(nodeSpec.isMember("service_name"));
		std::string servName = nodeSpec["service_name"].asString();

		assert(nodeSpec.isMember("service_domain"));
		std::string servDomain = nodeSpec["service_domain"].asString();

		assert(nodeSpec.isMember("code_path"));
		int codePath = nodeSpec["code_path"].asInt();

		assert(nodeSpec.isMember("start_stage"));
		int startStg = nodeSpec["start_stage"].asInt();

		assert(nodeSpec.isMember("end_stage"));
		int endStg = nodeSpec["end_stage"].asInt();

		// node info
		assert(nodeSpec.isMember("node_id"));
		unsigned nodeId = nodeSpec["node_id"].asUInt();

		assert(nodeSpec.isMember("sync"));
		bool sync = nodeSpec["sync"].asBool();
		unsigned syncNodeId = 0;
		if(sync) {
			assert(nodeSpec.isMember("sync_node_id"));
			syncNodeId = nodeSpec["sync_node_id"].asUInt();
		}
		//childs
		if(nodeIdChildsMap.find(nodeId) != nodeIdChildsMap.end()) 
			std::cout << "repeated nodeId = " << nodeId << std::endl;
		assert(nodeIdChildsMap.find(nodeId) == nodeIdChildsMap.end());
		nodeIdChildsMap[nodeId] = std::vector<unsigned> ();
		Json::Value& childs = nodeSpec["childs"];
		for(unsigned j = 0; j < childs.size(); ++j) {
			unsigned childId = childs[j].asUInt();
			nodeIdChildsMap[nodeId].push_back(childId);
			// printf("node %d has child %d\n", nodeId, childId);

			// increment childNode's syncNum by 1
			if(nodeIdSyncNumMap.find(childId) == nodeIdSyncNumMap.end())
				nodeIdSyncNumMap[childId] = 1;
			else
				++nodeIdSyncNumMap[childId];
		}
		MicroServPathNode* node = new MicroServPathNode(servName, servDomain, codePath, startStg, endStg, id, nodeId, sync, syncNodeId);
		path.addNode(node);
	}
	// set childs
	for(auto ele: nodeIdChildsMap)
		path.setChilds(ele.first, ele.second);

	// set sync number
	for(auto ele: nodeIdSyncNumMap)
		path.setSyncNum(ele.first, ele.second);

	// set sync node
	path.setSyncNode();
	// path correctness check
	path.check();
	return path;
}

Cluster*
ClusterParser::parsCluster() {
	Cluster* cluster = new Cluster(eq);
	/*** machine spec ***/
	std::string machFname = clusterDir + "/machines.json";
	struct stat pathStat;
	if(stat(machFname.c_str(), &pathStat) == 0 && S_ISREG(pathStat.st_mode) != 0) {
		Json::Reader reader;
		Json::Value machines;
		std::ifstream mf(machFname);
		reader.parse(mf, machines);
		for(unsigned i = 0; i < machines.size(); ++i) {
			Json::Value& machine = machines[i];
			assert(machine.isMember("machine_id"));
			assert(machine.isMember("total_cores"));
			assert(machine.isMember("name"));

			Machine* mach = new Machine(machine["machine_id"].asUInt(), machine["name"].asString(), 
				machine["total_cores"].asUInt(), debug);
			cluster->addMachine(mach);
			// set up net
			std::string instName = "net_stack_" + std::to_string(machine["machine_id"].asUInt());
			// NetStack* net = dynamic_cast<NetStack*> (parsMicroServ("net_stack", "net_stack", instName, servArch["net_stack"]));
			NetStack* net = dynamic_cast<NetStack*> (parsMicroServ(instName, "net_stack", instName, servArch["net_stack"]));

			assert(net != nullptr);

			assert(machine.isMember("net_stack_sched"));
			Json::Value& netSched = machine["net_stack_sched"];
			if(netSched["type"].asString() == "CMT") {
				assert(netSched.isMember("num_threads"));
				assert(netSched.isMember("cores"));
				std::vector<unsigned> cid;

				auto& coreSpec = netSched["cores"];

				for(unsigned j = 0; j < coreSpec.size(); ++j) 
					cid.push_back(coreSpec[j].asUInt());
				mach->setNet(net, "CMT", netSched["num_threads"].asUInt(), cid);
				// set core affinity
				if(netSched.isMember("core_affinity")) {
					auto& coreAffSpec = netSched["core_affinity"];
					for(unsigned j = 0; j < coreAffSpec.size(); ++j) {
						auto& coreAffSpecSingle = coreAffSpec[j];
						assert(coreAffSpecSingle.isMember("thread"));
						assert(coreAffSpecSingle.isMember("cores"));

						unsigned tid = coreAffSpecSingle["thread"].asUInt();
						auto& coreList = coreAffSpecSingle["cores"];
						std::vector<Core*> cores;
						for(unsigned k = 0; k < coreList.size(); ++k) {
							unsigned cid = coreList[k].asUInt();
							if(cid >= mach->numCores()) {
								printf("Machine: %s netStack core_affinity: %d for thread %d out of bound\n",
									mach->getName().c_str(), cid, tid);
								exit(-1);
							}
							cores.push_back(mach->getCore(cid));
						}
						net->setCoreAffinity(tid, cores);
					}	
				}
			} else if(netSched["type"].asString() == "Simplified" || netSched["type"].asString() == "LinuxNetStack") {
				assert(netSched.isMember("num_queues"));
				assert(netSched.isMember("cores"));
				std::vector<unsigned> cid;

				auto& coreSpec = netSched["cores"];
				// std::cout << "net sched core size = " << coreSpec.size() << std::endl;

				for(unsigned j = 0; j < coreSpec.size(); ++j) {
					// std::cout << "core " << coreSpec[j].asUInt() << " assigned to net stack" << std::endl;
					cid.push_back(coreSpec[j].asUInt());
				}

				mach->setNet(net, netSched["type"].asString(), netSched["num_queues"].asUInt(), cid);
				// set core affinity
				if(netSched.isMember("core_affinity")) {
					auto& coreAffSpec = netSched["core_affinity"];
					for(unsigned j = 0; j < coreAffSpec.size(); ++j) {
						auto& coreAffSpecSingle = coreAffSpec[j];
						assert(coreAffSpecSingle.isMember("queue"));
						assert(coreAffSpecSingle.isMember("cores"));

						unsigned qid = netSched["core_affinity"][j]["queue"].asUInt();
						auto& coreList = coreAffSpecSingle["cores"];
						std::vector<Core*> cores;
						for(unsigned k = 0; k < coreList.size(); ++k) {
							unsigned cid = coreList[k].asUInt();
							if(cid >= mach->numCores()) {
								printf("Machine: %s netStack core_affinity: %d for queue %d out of bound\n",
									mach->getName().c_str(), cid, qid);
								exit(-1);
							}
							cores.push_back(mach->getCore(cid));
						}
						net->setCoreAffinity(qid, cores);
					}	
				}
			} else {
				printf("Error: Undefined microservice scheduler type: %s\n", netSched["type"].asString().c_str());
				exit(1);
			}
			cluster->addService( dynamic_cast<MicroService*> (net));

			// std::cout << "done net" << std::endl;
		}

		std::cout << "done machines.json" << std::endl;

	}

	/*** dependency graph spec  ***/
	std::string graphFname = clusterDir + "/graph.json";
	if(stat(graphFname.c_str(), &pathStat) == 0 && S_ISREG(pathStat.st_mode) != 0) {
		Json::Reader reader;
		Json::Value depdGraph;
		std::ifstream gf(graphFname);
		reader.parse(gf, depdGraph);
		// net latency & tcp
		assert(depdGraph.isMember("net_latency"));
		Time netLat = depdGraph["net_latency"].asUInt();
		cluster->setNetLat(netLat);

		// micro-service
		Json::Value& services = depdGraph["microservices"];
		for(unsigned i = 0; i < services.size(); ++i) {
			Json::Value& service = services[i];
			assert(service.isMember("service_name"));
			std::string servName = service["service_name"].asString();
			// std::cout << "add service " << servName << std::endl;

			assert(service.isMember("service_domain"));
			std::string servDomain = service["service_domain"].asString();

			assert(service.isMember("instance_name"));
			std::string instName = service["instance_name"].asString();

			assert(service.isMember("model_name"));
			std::string modelName = service["model_name"].asString();

			// find micro service arch spec
			MicroService* ms = nullptr;
			if(servArch.find(modelName) != servArch.end()) {
				ms = parsMicroServ(servName, servDomain, instName, servArch[modelName]);
				// ms->servName = servName;
			} else {
				std::string servFname = modelName + ".json";
				DIR* dir = opendir(defServDir.c_str());
				if(dir == nullptr) {
					printf("Error: missing %s\n", defServDir.c_str());
					assert(dir != nullptr);
				}
				struct dirent* entry = readdir(dir);
				while(entry != nullptr) {
					std::string fname = std::string(entry->d_name);
					if(fname == servFname) {
						Json::Value servSpec;
						std::string fullPath = defServDir + "/" + servFname;
						reader.parse(fullPath, servSpec);
						// cache the spec
						servArch[modelName] = servSpec;
						ms = parsMicroServ(servName, servDomain, instName, servSpec);
						// ms->servName = servName;
						break;
					}
					entry = readdir(dir);
				}
			}
			if(ms == nullptr) {
				printf("Error: missing specification for microservice: %s\n", servName.c_str());
				exit(1);
			}
			cluster->addService(ms);
			// deploy service on machine
			assert(service.isMember("machine_id"));
			unsigned mid = service["machine_id"].asUInt();
			Machine* targMach = cluster->getMachine(mid);
			targMach->addService(ms);
			// set up scheduler
			assert(service.isMember("scheduler"));
			Json::Value& schedSpec = service["scheduler"];

			if(schedSpec["type"].asString() == "CMT") {
				assert(schedSpec.isMember("num_threads"));
				unsigned numThreads = schedSpec["num_threads"].asUInt();

				std::vector<Core*> cores;
				auto& coreSpec = schedSpec["cores"];
				for(unsigned j = 0; j < coreSpec.size(); ++j) {
					unsigned cid = coreSpec[j].asUInt();
					if(cid >= targMach->numCores()) {
						printf("MicroService: %s, core: %d, out of bound for Machine: id-%d name-%s\n",
							targMach->getName().c_str(), cid, targMach->getId(), targMach->getName().c_str());
						exit(-1);
					}
					cores.push_back(targMach->getCore(cid));
				}
				ms->setSched("CMT", numThreads, cores);
				// set core affinity
				if(schedSpec.isMember("core_affinity")) {
					auto& coreAffSpec = schedSpec["core_affinity"];
					for(unsigned j = 0; j < coreAffSpec.size(); ++j) {
						auto& coreAffSpecSingle = coreAffSpec[j];
						assert(coreAffSpecSingle.isMember("thread"));
						assert(coreAffSpecSingle.isMember("cores"));

						unsigned tid = coreAffSpecSingle["thread"].asUInt();
						auto& coreList = coreAffSpecSingle["cores"];
						std::vector<Core*> cores;
						for(unsigned k = 0; k < coreList.size(); ++k) {
							unsigned cid = coreList[k].asUInt();
							if(cid >= targMach->numCores()) {
								printf("MicroService: %s, thread: %d, core: %d, out of bound for Machine: id-%d name-%s\n",
									targMach->getName().c_str(), tid, cid, targMach->getId(), targMach->getName().c_str());
								exit(-1);
							}
							cores.push_back(targMach->getCore(cid));
						}
						ms->setCoreAffinity(tid, cores);
					}	
				}
			} else if (schedSpec["type"].asString() == "Simplified") {
				assert(schedSpec.isMember("num_queues"));
				unsigned numQues = schedSpec["num_queues"].asUInt();

				assert(schedSpec.isMember("cores"));
				auto& coreSpec = schedSpec["cores"];
				std::vector<Core*> cores;

				for(unsigned j = 0; j < coreSpec.size(); ++j) {
					unsigned cid = coreSpec[j].asUInt();
					if(cid >= targMach->numCores()) {
						printf("MicroService: %s core: %d out of bound for Machine: id-%d name-%s\n",
							targMach->getName().c_str(), cid, targMach->getId(), targMach->getName().c_str());
						exit(-1);
					}
					cores.push_back(targMach->getCore(cid));
				}
				ms->setSched("Simplified", numQues, cores);
				// set core affinity
				if(schedSpec.isMember("core_affinity")) {
					auto& coreAffSpec = schedSpec["core_affinity"];
					for(unsigned j = 0; j < coreAffSpec.size(); ++j) {
						auto& coreAffSpecSingle = coreAffSpec[j];
						assert(coreAffSpecSingle.isMember("queue"));
						assert(coreAffSpecSingle.isMember("cores"));

						unsigned qid = coreAffSpecSingle["queue"].asUInt();
						auto& coreList = coreAffSpecSingle["cores"];
						std::vector<Core*> cores;
						for(unsigned k = 0; k < coreList.size(); ++k) {
							unsigned cid = coreList[k].asUInt();
							if(cid >= targMach->numCores()) {
								printf("MicroService: %s, queue: %d, core: %d, out of bound for Machine: id-%d name-%s\n",
									targMach->getName().c_str(), qid, cid, targMach->getId(), targMach->getName().c_str());
								exit(-1);
							}
							cores.push_back(targMach->getCore(cid));
						}
						ms->setCoreAffinity(qid, cores);
					}	
				}

			} else if (schedSpec["type"].asString() == "LinuxNetStack") {
				printf("Error: Setting LinuxNetStack for non-net service: %s\n", instName.c_str());
				exit(-1);
			} else {
				printf("Error: Undefined microservice scheduler type: %s\n", schedSpec["type"].asString().c_str());
				exit(-1);
			}
		}

		std::cout << "done placement" << std::endl;

		// edge
		Json::Value& edges = depdGraph["edges"];
		for(unsigned i = 0; i < edges.size(); ++i) {
			assert(edges[i].isMember("source"));
			std::string src = edges[i]["source"].asString();

			assert(edges[i].isMember("target"));
			std::string targ = edges[i]["target"].asString();

			assert(edges[i].isMember("biDirectional"));
			bool biDir = edges[i]["biDirectional"].asBool();
			cluster->addEdge(src, targ, biDir);
		}


		std::cout << "done edges" << std::endl;

		// set up connections among machines
		cluster->setupConn();
	} else {
		printf("Error: Missing graph.json\n");
		exit(1);
	}

	std::cout << "done graph.json" << std::endl;

	/*** microservice path spec ***/
	std::vector<MicroServPath> paths;
	std::vector<unsigned> pathDistr;
	std::string pathFname = clusterDir + "/path.json";
	if(stat(pathFname.c_str(), &pathStat) == 0 && S_ISREG(pathStat.st_mode) != 0) {
		Json::Reader reader;
		Json::Value allPathSpec;
		std::ifstream pf(pathFname);
		reader.parse(pf, allPathSpec);
		for(unsigned i = 0; i < allPathSpec.size(); ++i) {
			assert(allPathSpec[i].isMember("probability"));
			unsigned prob = allPathSpec[i]["probability"].asUInt();
			Json::Value& pathSpec = allPathSpec[i];
			MicroServPath path = parsServPath(pathSpec);
			paths.push_back(path);
			pathDistr.push_back(prob);
		}
		cluster->setServPath(paths);
		cluster->setServPathDistr(pathDistr);
	} else {
		printf("Error: Missing path.json\n");
		exit(1);
	}

	// std::cout << "after parsCluster clusterDir = " << clusterDir << std::endl;

	return cluster;
}

Client*
ClusterParser::parsClient(unsigned num_conn, Time net_lat, double kqps, bool debug) {
	std::string cliFname = clusterDir + "/client.json";

	// std::cout << "client.json path: " << cliFname << std::endl;

	struct stat pathStat;
	if(stat(cliFname.c_str(), &pathStat) == 0 && S_ISREG(pathStat.st_mode) != 0) {
		Json::Reader reader;
		Json::Value cliSpec;
		std::ifstream cf(cliFname);
		reader.parse(cf, cliSpec);

		assert(cliSpec.isMember("monitor_interval_sec"));
		Time monitor_interval = (Time) (cliSpec["monitor_interval_sec"].asDouble() * 1000000000);

		// assume both machines start from 2.6GHz
		unsigned init_memc_freq = 2600;
		unsigned init_ngx_freq = 2600;
		// first 0 represents 0 jobs
		Client* client = new Client(0, num_conn, net_lat, debug,
				monitor_interval);

		// assign epochs to cluster
		assert(cliSpec.isMember("epoch_end_sec"));
		Json::Value epoch_sec_spec = cliSpec["epoch_end_sec"];
		assert(cliSpec.isMember("epoch_qps"));
		Json::Value epoch_qps_spec = cliSpec["epoch_qps"];

		assert(epoch_qps_spec.size() == epoch_sec_spec.size());
		Time last_end = 0;
		for(unsigned i = 0; i < epoch_qps_spec.size(); ++i) {
			Time epoch_end = (Time) (epoch_sec_spec[i].asDouble() * 1000000000);
			uint64_t epoch_qps = epoch_qps_spec[i].asUInt();
			if(kqps != 0)
				epoch_qps = (unsigned)(kqps * 1000);
			assert(epoch_end > last_end);
			last_end = epoch_end;

			client->addEpoch(epoch_end, epoch_qps);
		}
		
		return client;
	} else {
		printf("Error: missing client.json\n");	
		exit(-1);
		return nullptr;
	}
}

