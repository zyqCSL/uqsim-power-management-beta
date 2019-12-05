import sys
import os
import json
import make_arch as march 

FANOUT = 50

def main():
	global FANOUT
	nodeList = []

	# first node is load balancer
	# childs include all nginx instances

	childs_list = range(1, FANOUT + 1)
	node = march.make_serv_path_node(servName = "load_balancer", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 0, needSync = False, syncNodeId = None, childs = childs_list)

	nodeList.append(node)
	node_cnt = 1

	# leaf_service
	for i in range(0, FANOUT):
		serv_name = "leaf_service_" + str(i)
		# child list should be the paired memcached
		childs_list = [FANOUT + 1]
		node = march.make_serv_path_node(servName = serv_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
			nodeId = i + 1, needSync = False, syncNodeId = None, childs = childs_list) 
		nodeList.append(node)
		node_cnt += 1

	# client finally
	node = march.make_serv_path_node(servName = "client", servDomain = "", codePath = -1, startStage = 0, endStage = -1, 
		nodeId = FANOUT + 1, needSync = False, syncNodeId = None, childs = [])
	nodeList.append(node)

	path = march.make_serv_path(pathId = 0, entry = 0, prob = 100, nodes = nodeList)

	paths = [path]

	with open("./json/path.json", "w+") as f:
		json.dump(paths, f, indent=2)

if __name__ == "__main__":
	main()