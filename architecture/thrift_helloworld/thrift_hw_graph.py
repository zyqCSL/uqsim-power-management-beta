import sys
import os
import json
import make_arch as march 


def main():
	sched = march.make_service_sched("CMT", [50, [20]], None)
	thrift_hw = march.make_serv_inst(servName = "thrift_hw", servDomain = "", instName = "thrift_hw", 
		modelName = "thrift_hw", sched = sched, machId = 0)

	services = [thrift_hw]

	edges = []

	graph = march.make_cluster(services = services, edges = edges, netLat = 65000)

	with open("./json/graph.json", "w+") as f:
		json.dump(graph, f, indent=2)

if __name__ == "__main__":
	main()

