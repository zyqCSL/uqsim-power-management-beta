import sys
import os
import json
import make_arch as march 

FANOUT = 5
NGX_THREAD = 4

def main():
	global NGX_THREAD
	services = []
	edges = []
	# sched = march.make_service_sched("CMT", [8, [20,21,22,23,24,25,26,27]], None)
	sched = march.make_service_sched("CMT", [NGX_THREAD, range(20, 20 + NGX_THREAD)], None)
	nginx = march.make_serv_inst(servName = "nginx_frontend", servDomain = "", instName = "nginx_frontend", 
		modelName = "nginx_frontend", sched = sched, machId = 0)
	services.append(nginx)

	for i in range(1, FANOUT + 1):
		sched = march.make_service_sched("CMT", [NGX_THREAD, range(20, 20 + NGX_THREAD)], None)
		inst_name = "nginx_leaf_" + str(i - 1)
		nginx = march.make_serv_inst(servName = inst_name, servDomain = "", instName = inst_name,
			modelName = "nginx_file_server", sched = sched, machId = i)
		services.append(nginx)

	# sched = march.make_service_sched("CMT", [4, [20,21,22,23]], None)
	# memcached = march.make_serv_inst(servName = "memcached", servDomain = "", instName = "memcached", sched = sched, machId = 1)

	for i in range(1, FANOUT + 1):
		edge = march.make_edge(src = "nginx_frontend", targ = "nginx_leaf_" + str(i - 1), bidir = True)
		edges.append(edge)

	graph = march.make_cluster(services = services, edges = edges, netLat = 65000)

	with open("./json/graph.json", "w+") as f:
		json.dump(graph, f, indent=2)

if __name__ == "__main__":
	main()

