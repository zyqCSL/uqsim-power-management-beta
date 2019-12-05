import sys
import os
import json
import make_arch as march 


def main():
	# path 0: memcached cache hit
	nodeList = []
	node_0 = march.make_serv_path_node(servName = "php", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 0, needSync = False, syncNodeId = None, childs = [])
	nodeList.append(node_0)

	node_1 = march.make_serv_path_node(servName = "movieID", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 1, needSync = False, syncNodeId = None, childs = [])

	node_2 = march.make_serv_path_node(servName = "text", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 2, needSync = False, syncNodeId = None, childs = [])

	node_3 = march.make_serv_path_node(servName = "uniqueID", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 3, needSync = False, syncNodeId = None, childs = [])

	node_4 = march.make_serv_path_node(servName = "AssignRating", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 4, needSync = False, syncNodeId = None, childs = [])

	node_5 = march.make_serv_path_node(servName = "ComposeReview", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 5, needSync = False, syncNodeId = None, childs = [])

	node_6 = march.make_serv_path_node(servName = "MovieReviewDB", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 6, needSync = False, syncNodeId = None, childs = [])

	node_7 = march.make_serv_path_node(servName = "ReviewStorage", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 7, needSync = False, syncNodeId = None, childs = [])

	node_8 = march.make_serv_path_node(servName = "UserReviewDB", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 8, needSync = False, syncNodeId = None, childs = [])



	############################ old 2tier starts ###########################################
	node_0 = march.make_serv_path_node(servName = "nginx", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 0, needSync = False, syncNodeId = None, childs = [1])
	nodeList.append(node_0)

	node_1 = march.make_serv_path_node(servName = "memcached", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 1, needSync = False, syncNodeId = None, childs = [2])
	nodeList.append(node_1)

	# should be a different nginx path
	node_2 = march.make_serv_path_node(servName = "nginx", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = 2, needSync = False, syncNodeId = None, childs = [3])
	nodeList.append(node_2)

	node_3 = march.make_serv_path_node(servName = "client", servDomain = "", codePath = -1, startStage = 0, endStage = -1, 
		nodeId = 3, needSync = False, syncNodeId = None, childs = [])
	nodeList.append(node_3)

	path_memc_hit = march.make_serv_path(pathId = 0, entry = 0, prob = 1.0, nodes = nodeList)


	paths = [path_memc_hit]

	############################ old 2tier ends ###########################################

	with open("./json/path.json", "w+") as f:
		json.dump(paths, f, indent=2)

if __name__ == "__main__":
	main()