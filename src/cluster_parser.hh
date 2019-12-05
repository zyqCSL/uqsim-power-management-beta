#ifndef __CLUSTER_PARSER_HH__
#define __CLUSTER_PARSER_HH__

#include <string>

#include "../jsoncpp/json/json.h"
#include "cluster.hh"

// change name to parser
#include "client.hh"

class ClusterParser 
{
	private:
		// micro service configuration
		std::unordered_map<std::string, Json::Value> servArch;
		// map instance name to service id
		std::unordered_map<std::string, unsigned> servNameIdMap;
		// default directory that contains default configuration of micro services
		static const std::string defServDir;
		// directory that contains cluster configuration
		std::string clusterDir;
		unsigned servId;
		bool debug;
		EventQueue* eq;
		double kqps;

	public:
		ClusterParser(const std::string& cluster_dir, double kqps, bool debug);

	private:
		MicroService* parsMicroServ(const std::string& servName, const std::string& servFunc, const std::string& instName, Json::Value& serv);
		MicroServPath parsServPath(Json::Value& servPath);

	public:
		Cluster* parsCluster();
		Client* parsClient(unsigned num_conn, Time net_lat, double kqps, bool debug);
};



#endif