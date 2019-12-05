import sys
import os
import json
import make_arch as march 


def main():
	sched = march.make_service_sched("CMT", [8, [20,21,22,23,24,25,26,27]], None)
	nginx = march.make_serv_inst(servName = "nginx", servDomain = "", instName = "nginx", 
		modelName = "nginx", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [4, [20,21,22,23]], None)
	memcached = march.make_serv_inst(servName = "memcached", servDomain = "", instName = "memcached", 
		modelName = "memcached", sched = sched, machId = 1)

	sched = march.make_service_sched("CMT", [300, [20,21,22,23]], None)
	mongodb = march.make_serv_inst(servName = "mongodb", servDomain = "", instName = "mongodb", 
		modelName = "mongodb", sched = sched, machId = 2)

	sched = march.make_service_sched("Simplified", [1, [24]], None)
	mongo_io = march.make_serv_inst(servName = "mongo_io", servDomain = "", instName = "mongo_io",
	 modelName = "mongo_io", sched = sched, machId = 2)

	services = [nginx, memcached, mongodb, mongo_io]

	edge_0 = march.make_edge(src = "nginx", targ = "memcached", bidir = True)
	edge_1 = march.make_edge(src = "nginx", targ = "mongodb", bidir = True)
	edge_2 = march.make_edge(src = "mongodb", targ = "mongo_io", bidir = True)

	edges = [edge_0, edge_1, edge_2]

	graph = march.make_cluster(services = services, edges = edges, netLat = 65000)

	with open("./json/graph.json", "w+") as f:
		json.dump(graph, f, indent=2)

if __name__ == "__main__":
	main()

