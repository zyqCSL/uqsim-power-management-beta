import sys
import os
import json
import make_arch as march

def main():
	node_0 = march.make_serv_path_node(servName="memcached", servDomain="", 
		codePath=0, startStage=0, endStage=-1, nodeId=0, needSync=False, syncNodeId=None, childs=[1])
	node_1 = march.make_serv_path_node(servName = "client", servDomain = "", 
		codePath = -1, startStage = 0, endStage = -1, 
		nodeId = 1, needSync = False, syncNodeId = None, childs = [])
	nodeList = [node_0, node_1]
	memc_read_only_path = march.make_serv_path(pathId=0, entry=0, prob=1.0, nodes=nodeList)
	paths = [memc_read_only_path]
	with open("/home/zhangyanqi/cornell/SAIL/microSSim/architecture/memcached/path.json", "w+") as f:
		json.dump(paths, f, indent=2)

if __name__ == "__main__":
	main()