import sys
import os
import json
import make_arch as march 


def main():
	# path 0: memcached cache hit
	nodeList = []

	node_0 = march.make_serv_path_node(servName = "nginx", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 0, needSync = False, syncNodeId = None, childs = [1])
	nodeList.append(node_0)

	node_1 = march.make_serv_path_node(servName = "memcached", servDomain = "", codePath = -1, startStage = 0, endStage = -1, 
		nodeId = 1, needSync = False, syncNodeId = None, childs = [2])
	nodeList.append(node_1)

	node_2 = march.make_serv_path_node(servName = "nginx", servDomain = "", codePath = 1, startStage = 0, endStage = -1, 
		nodeId = 2, needSync = False, syncNodeId = None, childs = [3])
	nodeList.append(node_2)

	node_3 = march.make_serv_path_node(servName = "client", servDomain = "", codePath = -1, startStage = 0, endStage = -1, 
		nodeId = 3, needSync = False, syncNodeId = None, childs = [])
	nodeList.append(node_3)

	path_memc_hit = march.make_serv_path(pathId = 0, entry = 0, prob = 0.5, nodes = nodeList)

	# path 1: memcached miss & mongodb hit
	nodeList = [node_0, node_1, node_2]
	# php_fcgi_req
	node_3 = march.make_serv_path_node(servName = "php", servDomain = "", codePath = 0, startStage = 0, endStage = 0,
		nodeId = 3, needSync = True, syncNodeId = 5, childs = [4])
	nodeList.append(node_3)

	node_4 = march.make_serv_path_node(servName = "php_io", servDomain = "", codePath = 0, startStage = 0, endStage = -1,
		nodeId = 4, needSync = False, syncNodeId = None, childs = [5])
	nodeList.append(node_4)

	# php_fopen
	node_5 = march.make_serv_path_node(servName = "php", servDomain = "", codePath = 0, startStage = 0, endStage = 1,
		nodeId = 5, needSync = True, syncNodeId = 7, childs = [6])
	nodeList.append(node_5)

	node_6 = march.make_serv_path_node(servName = "php_io", servDomain = "", codePath = 1, startStage = 0, endStage = -1,
		nodeId = 6, needSync = False, syncNodeId = None, childs = [7])
	nodeList.append(node_6)

	# php_fput
	node_7 = march.make_serv_path_node(servName = "php", servDomain = "", codePath = 0, startStage = 1, endStage = 2,
		nodeId = 7, needSync = True, syncNodeId = 9, childs = [8])
	nodeList.append(node_7)

	node_8 = march.make_serv_path_node(servName = "mongodb", servDomain = "", codePath = 0, startStage = 0, endStage = -1,
		nodeId = 8, needSync = False, syncNodeId = None, childs = [9])
	nodeList.append(node_8)

	# php_find_done
	node_9 = march.make_serv_path_node(servName = "php", servDomain = "", codePath = 0, startStage = 2, endStage = 3,
		nodeId = 9, needSync = True, syncNodeId = 11, childs = [10])
	nodeList.append(node_9)

	node_10 = march.make_serv_path_node(servName = "mongodb", servDomain = "", codePath = 0, startStage = 0, endStage = -1,
		nodeId = 10, needSync = False, syncNodeId = None, childs = [11])
	nodeList.append(node_10)

	# php_get_bytes
	node_11 = march.make_serv_path_node(servName = "php", servDomain = "", codePath = 0, startStage = 3, endStage = 4,
		nodeId = 11, needSync = True, syncNodeId = 13, childs = [12])
	nodeList.append(node_11)

	node_12 = march.make_serv_path_node(servName = "memcached", servDomain = "", codePath = 1, startStage = 0, endStage = -1,
		nodeId = 12, needSync = False, syncNodeId = None, childs = [13])
	nodeList.append(node_12)

	# php_mmc_store
	node_13 = march.make_serv_path_node(servName = "php", servDomain = "", codePath = 0, startStage = 4, endStage = -1,
		nodeId = 13, needSync = False, syncNodeId = None, childs = [14])
	nodeList.append(node_13)

	node_14 = march.make_serv_path_node(servName = "nginx", servDomain = "", codePath = 2, startStage = 0, endStage = -1,
		nodeId = 14, needSync = False, syncNodeId = None, childs = [15])
	nodeList.append(node_14)

	node_15 = march.make_serv_path_node(servName = "client", servDomain = "", codePath = -1, startStage = 0, endStage = -1, 
		nodeId = 15, needSync = False, syncNodeId = None, childs = [])
	nodeList.append(node_15)

	path_memc_miss_mongo_hit = march.make_serv_path(pathId = 1, entry = 0, prob = 0.25, nodes = nodeList)


	# path 2: memcached miss & mongodb miss
	nodeList = []
	
	# first access to mongo
	node_8 = march.make_serv_path_node(servName = "mongodb", servDomain = "", codePath = 1, startStage = 0, endStage = 1,
		nodeId = 8, needSync = True, syncNodeId = 19, childs = [18])

	node_18 = march.make_serv_path_node(servName = "mongo_io", servDomain = "", codePath = 0, startStage = 0, endStage = -1,
		nodeId = 18, needSync = False, syncNodeId = None, childs = [19])

	node_19 = march.make_serv_path_node(servName = "mongodb", servDomain = "", codePath = 1, startStage = 1, endStage = -1,
		nodeId = 19, needSync = False, syncNodeId = None, childs = [9])

	# second access to mongo, should also access mongo_io, here just for testing internal chunking
	node_10 = march.make_serv_path_node(servName = "mongodb", servDomain = "", codePath = 1, startStage = 0, endStage = -1,
		nodeId = 10, needSync = False, syncNodeId = None, childs = [11])

	nodeList = [node_0, node_1, node_2, node_3, node_4, node_5, node_6, node_7, node_8, node_9, node_10, node_11, node_12,
		node_13, node_14, node_15, node_18, node_19]

	path_memc_miss_mongo_miss = march.make_serv_path(pathId = 2, entry = 0, prob = 0.25, nodes = nodeList)

	paths = [path_memc_hit, path_memc_miss_mongo_hit, path_memc_miss_mongo_miss]

	with open("./json/path.json", "w+") as f:
		json.dump(paths, f, indent=2)

if __name__ == "__main__":
	main()