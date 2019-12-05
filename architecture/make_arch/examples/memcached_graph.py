import sys
import os
import json
import make_arch as march 


def main():
	sched = march.make_service_sched("CMT", [4, [20,21,22,23]], None)
	memcached = march.make_serv_inst(servName = "memcached", servDomain = "", instName = "memcached", sched = sched, machId = 0)

	services = [memcached]

	edges = []

	graph = march.make_cluster(services = services, edges = edges, netLat = 30.0)

	with open("../../memcached/graph.json", "w+") as f:
		json.dump(graph, f, indent=2)

if __name__ == "__main__":
	main()

