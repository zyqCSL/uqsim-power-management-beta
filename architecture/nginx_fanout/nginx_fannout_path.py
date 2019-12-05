import sys
import os
import json
import make_arch as march 

FANOUT = 5

def main():
	global FANOUT
	# path 0: memcached cache hit
	nodeList = []

	node_0 = march.make_serv_path_node(servName = "nginx_frontend", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 0, needSync = False, syncNodeId = None, childs = range(1, FANOUT + 1))
	nodeList.append(node_0)

	for i in range(1, FANOUT + 1):
		serv_name = "nginx_leaf_" + str(i - 1)
		node = march.make_serv_path_node(servName = serv_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
			nodeId = i, needSync = False, syncNodeId = None, childs = [FANOUT + 1])
		nodeList.append(node)

		# node_1 = march.make_serv_path_node(servName = "memcached", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		# 	nodeId = 1, needSync = False, syncNodeId = None, childs = [2])
		# nodeList.append(node_1)

	# should be a different nginx path (currently still set to 0 to prevent deadlock issues)
	node_2 = march.make_serv_path_node(servName = "nginx_frontend", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = FANOUT + 1, needSync = False, syncNodeId = None, childs = [FANOUT + 2])
	nodeList.append(node_2)

	# last node returns to client

	node_3 = march.make_serv_path_node(servName = "client", servDomain = "", codePath = -1, startStage = 0, endStage = -1, 
		nodeId = FANOUT + 2, needSync = False, syncNodeId = None, childs = [])
	nodeList.append(node_3)

	path = march.make_serv_path(pathId = 0, entry = 0, prob = 100, nodes = nodeList)


	paths = [path]

	with open("./json/path.json", "w+") as f:
		json.dump(paths, f, indent=2)

if __name__ == "__main__":
	main()