import sys
import os
import json
import make_arch as march 

FANOUT = 50

def main():
	global FANOUT
	services = []
	# nginx
	ngx_init_machine_id = 0
	sched = march.make_service_sched("CMT", [8, [20,21,22,23,24,25,26,27]], None)
	for i in range(0, FANOUT):
		ngx_name = "nginx_" + str(i)
		nginx = march.make_serv_inst(servName = ngx_name, servDomain = "", instName = ngx_name, modelName = "nginx", sched = sched, machId = i + ngx_init_machine_id)
		services.append(nginx)

	# memcached
	memc_init_machine_id = FANOUT
	sched = march.make_service_sched("CMT", [4, [20,21,22,23]], None)
	for i in range(0, FANOUT):
		memc_name = "memcached_" + str(i)
		memcached = march.make_serv_inst(servName = memc_name, servDomain = "", instName = memc_name, modelName = "memcached", sched = sched, machId = i + memc_init_machine_id)
		services.append(memcached)

	# load balancer
	load_bal_machine_id = 2*FANOUT
	sched = march.make_service_sched("CMT", [1, [20]], None)
	load_bal = march.make_serv_inst(servName = "load_balancer", servDomain = "", instName = "load_balancer", modelName = "load_balancer", sched = sched, machId = load_bal_machine_id)
	services.append(load_bal)

	edges = []
	# edge between load_balancer & each ngx
	for i in range(0, FANOUT):
		src_name = "load_balancer"
		targ_name = "nginx_" + str(i)
		edge = march.make_edge(src = src_name, targ = targ_name, bidir = True)
		edges.append(edge)

	# edge between each ngx, memc pair
	for i in range(0, FANOUT):
		src_name = "nginx_" + str(i)
		targ_name = "memcached_" + str(i)
		edge = march.make_edge(src = src_name, targ = targ_name, bidir = True)
		edges.append(edge)

	graph = march.make_cluster(services = services, edges = edges, netLat = 65.0)

	with open("./json/graph.json", "w+") as f:
		json.dump(graph, f, indent=2)

if __name__ == "__main__":
	main()

