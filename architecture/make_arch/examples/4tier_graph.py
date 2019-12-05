import sys
import os
import json
import make_arch as march 


def main():
	sched = march.make_service_sched("CMT", [2, [20,21]], None)
	nginx = march.make_serv_inst(servName = "nginx", servDomain = "", instName = "nginx", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [4, [20,21,22,23]], None)
	php = march.make_serv_inst(servName = "php", servDomain = "", instName = "php", sched = sched, machId = 1)

	sched = march.make_service_sched("CMT", [1, [24]], None)
	php_io = march.make_serv_inst(servName = "php_io", servDomain = "", instName = "php_io", sched = sched, machId = 1)

	sched = march.make_service_sched("CMT", [4, [20,21,22,23]], None)
	memcached = march.make_serv_inst(servName = "memcached", servDomain = "", instName = "memcached", sched = sched, machId = 2)

	sched = march.make_service_sched("CMT", [4, [20,21,22,23]], None)
	mongodb = march.make_serv_inst(servName = "mongodb", servDomain = "", instName = "mongodb", sched = sched, machId = 3)

	sched = march.make_service_sched("CMT", [1, [24]], None)
	mongodb_io = march.make_serv_inst(servName = "mongo_io", servDomain = "", instName = "mongo_io", sched = sched, machId = 3)

	services = [nginx, php, php_io, memcached, mongodb, mongodb_io]

	edge_0 = march.make_edge(src = "nginx", targ = "memcached", bidir = True)
	edge_1 = march.make_edge(src = "nginx", targ = "php", bidir = True)
	edge_2 = march.make_edge(src = "php", targ = "mongodb", bidir = True)
	edge_3 = march.make_edge(src = "php", targ = "memcached", bidir = True)
	edge_4 = march.make_edge(src = "php", targ = "php_io", bidir = True)
	edge_5 = march.make_edge(src = "mongodb", targ = "mongo_io", bidir = True)

	edges = [edge_0, edge_1, edge_2, edge_3, edge_4, edge_5]

	graph = march.make_cluster(services = services, edges = edges, netLat = 30.0)

	with open("./json/graph.json", "w+") as f:
		json.dump(graph, f, indent=2)

if __name__ == "__main__":
	main()

