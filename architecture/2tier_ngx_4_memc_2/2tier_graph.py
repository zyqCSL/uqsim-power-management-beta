import sys
import os
import json
import make_arch as march 


def main():
	sched = march.make_service_sched("CMT", [4, [20,21,22,23]], None)
	nginx = march.make_serv_inst(servName = "nginx", servDomain = "", instName = "nginx", 
		modelName = "nginx", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [2, [20,21]], None)
	memcached = march.make_serv_inst(servName = "memcached", servDomain = "", instName = "memcached", 
		modelName = "memcached", sched = sched, machId = 1)

	services = [nginx, memcached]

	edge_0 = march.make_edge(src = "nginx", targ = "memcached", bidir = True)

	edges = [edge_0]

	graph = march.make_cluster(services = services, edges = edges, netLat = 65000)

	with open("./json/graph.json", "w+") as f:
		json.dump(graph, f, indent=2)

if __name__ == "__main__":
	main()

