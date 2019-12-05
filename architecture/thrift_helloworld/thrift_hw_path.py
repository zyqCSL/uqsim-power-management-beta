import sys
import os
import json
import make_arch as march 


def main():
	# path 0: memcached cache hit
	nodeList = []

	node_0 = march.make_serv_path_node(servName = "thrift_hw", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 0, needSync = False, syncNodeId = None, childs = [1])
	nodeList.append(node_0)


	node_1 = march.make_serv_path_node(servName = "client", servDomain = "", codePath = -1, startStage = 0, endStage = -1, 
		nodeId = 1, needSync = False, syncNodeId = None, childs = [])
	nodeList.append(node_1)

	path = march.make_serv_path(pathId = 0, entry = 0, prob = 100, nodes = nodeList)


	paths = [path]

	with open("./json/path.json", "w+") as f:
		json.dump(paths, f, indent=2)

if __name__ == "__main__":
	main()