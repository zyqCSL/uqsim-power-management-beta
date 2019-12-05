import sys
import os
import json
import make_arch as march 

FANOUT = 50

def main():
	global FANOUT
	nodeList = []

	node_cnt = 0
	ngx_1st_start_node = 1
	memc_start_node = FANOUT + 1
	ngx_2nd_start_node = FANOUT*2 + 1
	client_dst_node = FANOUT*3 + 1

	# first node is load balancer
	# childs include all nginx instances

	childs_list = range(1, FANOUT + 1)
	node = march.make_serv_path_node(servName = "load_balancer", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 0, needSync = False, syncNodeId = None, childs = childs_list)

	nodeList.append(node)
	node_cnt = 1

	# then nginx
	for i in range(0, FANOUT):
		serv_name = "nginx_" + str(i)
		# child list should be the paired memcached
		childs_list = [i + memc_start_node]
		node = march.make_serv_path_node(servName = serv_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
			nodeId = i + ngx_1st_start_node, needSync = False, syncNodeId = None, childs = childs_list) 
		nodeList.append(node)
		node_cnt += 1


	# then memcached
	for i in range(0, FANOUT):
		serv_name = "memcached_" + str(i)
		# child list should be paired nginx
		childs_list = [i + ngx_2nd_start_node]
		node = march.make_serv_path_node(servName = serv_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
			nodeId = i + memc_start_node, needSync = False, syncNodeId = None, childs = childs_list) 
		nodeList.append(node)
		node_cnt += 1

	# then nginx
	for i in range(0, FANOUT):
		serv_name = "nginx_" + str(i)
		# child list should be client
		childs_list = [client_dst_node]
		node = march.make_serv_path_node(servName = serv_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
			nodeId = i + ngx_2nd_start_node, needSync = False, syncNodeId = None, childs = childs_list) 
		nodeList.append(node)
		node_cnt += 1

	# client finally
	node = march.make_serv_path_node(servName = "client", servDomain = "", codePath = -1, startStage = 0, endStage = -1, 
		nodeId = client_dst_node, needSync = False, syncNodeId = None, childs = [])
	nodeList.append(node)

	path = march.make_serv_path(pathId = 0, entry = 0, prob = 1.0, nodes = nodeList)

	paths = [path]

	with open("./json/path.json", "w+") as f:
		json.dump(paths, f, indent=2)

if __name__ == "__main__":
	main()