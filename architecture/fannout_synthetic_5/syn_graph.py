import sys
import os
import json
import make_arch as march 

FANOUT = 5

def main():
	global FANOUT
	services = []

	# load balancer
	load_bal_machine_id = 0
	sched = march.make_service_sched("CMT", [1, [20]], None)
	load_bal = march.make_serv_inst(servName = "load_balancer", servDomain = "", instName = "load_balancer", modelName = "load_balancer", sched = sched, machId = load_bal_machine_id)
	services.append(load_bal)

	# leaf_service
	leaf_machine_id = 1
	for i in range(0, FANOUT):
		sched = march.make_service_sched("CMT", [1, [20]], None)
		serv_name = 'leaf_service_' + str(i)
		leaf_service = march.make_serv_inst(servName = serv_name, servDomain = "", instName = serv_name, modelName = "leaf_service", sched = sched, machId = leaf_machine_id)
		leaf_machine_id += 1
		services.append(leaf_service)

	edges = []
	# edge between load_balancer & each leaf service
	for i in range(0, FANOUT):
		src_name = "load_balancer"
		targ_name = "leaf_service_" + str(i)
		edge = march.make_edge(src = src_name, targ = targ_name, bidir = True)
		edges.append(edge)

	graph = march.make_cluster(services = services, edges = edges, netLat = 65000)

	with open("./json/graph.json", "w+") as f:
		json.dump(graph, f, indent=2)

if __name__ == "__main__":
	main()

