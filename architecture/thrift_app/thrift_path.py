import sys
import os
import json
import make_arch as march 

def generate_memc_hit_path(service_name, start_node_id, back_node_id, node_list):
	memc_name = service_name + "_memcached"
	node_0 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id, needSync = False, syncNodeId = None, childs = [start_node_id + 1])

	node_1 = march.make_serv_path_node(servName = memc_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 1, needSync = False, syncNodeId = None, childs = [start_node_id + 2])

	node_2 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 2, needSync = False, syncNodeId = None, childs = [back_node_id]) 

	node_list += [node_0, node_1, node_2]

	return start_node_id + 2


def generate_memc_miss_mongo_hit_path(service_name, start_node_id, back_node_id, node_list):
	memc_name = service_name + "_memcached"
	mongo_name = service_name + "_mongodb"
	node_0 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id, needSync = False, syncNodeId = None, childs = [start_node_id + 1])

	node_1 = march.make_serv_path_node(servName = memc_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 1, needSync = False, syncNodeId = None, childs = [start_node_id + 2])

	node_2 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 2, needSync = False, syncNodeId = None, childs = [start_node_id + 3])

	node_3 = march.make_serv_path_node(servName = mongo_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 3, needSync = False, syncNodeId = None, childs = [start_node_id + 4])

	node_4 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 4, needSync = False, syncNodeId = None, childs = [start_node_id + 5])

	node_5 = march.make_serv_path_node(servName = memc_name, servDomain = "", codePath = 1, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 5, needSync = False, syncNodeId = None, childs = [start_node_id + 6])

	node_6 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 6, needSync = False, syncNodeId = None, childs = [back_node_id])

	node_list += [node_0, node_1, node_2, node_3, node_4, node_5, node_6]

	return start_node_id + 6

def generate_memc_miss_mongo_miss_path(service_name, start_node_id, back_node_id, node_list):
	memc_name = service_name + "_memcached"
	mongo_name = service_name + "_mongodb"
	mongo_io_name = service_name + "_mongodb_io"
	node_0 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id, needSync = False, syncNodeId = None, childs = [start_node_id + 1])

	node_1 = march.make_serv_path_node(servName = memc_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 1, needSync = False, syncNodeId = None, childs = [start_node_id + 2])

	node_2 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 2, needSync = False, syncNodeId = None, childs = [start_node_id + 3])

	node_3 = march.make_serv_path_node(servName = mongo_name, servDomain = "", codePath = 1, startStage = 0, endStage = 1, 
		nodeId = start_node_id + 3, needSync = True, syncNodeId = 5 + start_node_id, childs = [start_node_id + 4])

	node_4 = march.make_serv_path_node(servName = mongo_io_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 4, needSync = False, syncNodeId = None, childs = [start_node_id + 5])

	node_5 = march.make_serv_path_node(servName = mongo_name, servDomain = "", codePath = 1, startStage = 2, endStage = -1, 
		nodeId = start_node_id + 5, needSync = False, syncNodeId = None, childs = [start_node_id + 6])
	
	node_6 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 6, needSync = False, syncNodeId = None, childs = [start_node_id + 7])

	node_7 = march.make_serv_path_node(servName = memc_name, servDomain = "", codePath = 1, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 7, needSync = False, syncNodeId = None, childs = [start_node_id + 8])

	node_8 = march.make_serv_path_node(servName = service_name, servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
		nodeId = start_node_id + 8, needSync = False, syncNodeId = None, childs = [back_node_id])
	
	node_list += [node_0, node_1, node_2, node_3, node_4, node_5, node_6, node_7, node_8]

	return start_node_id + 8

def main():
	user_path  = [0]
	tweet_path = [0]
	file_path  = [0]

	func = [generate_memc_hit_path, generate_memc_miss_mongo_hit_path, generate_memc_miss_mongo_miss_path]

	user_prob  = [1.0]
	tweet_prob = [1.0]
	file_prob  = [1.0]

	path_id = 0

	paths = []

	for user in user_path:
		for tweet in tweet_path:
			for file in file_path:
				back_to_compose_1st_node_id = 30
				back_to_compose_2nd_node_id = 50
				start_node_id = 0
				node_list = []
				start_child_0 = func[user]("user", 1, back_to_compose_1st_node_id, node_list)
				func[tweet]("tweet", start_child_0 + 1, back_to_compose_1st_node_id, node_list)

				start_node = march.make_serv_path_node(servName = "compose", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
					nodeId = 0, needSync = False, syncNodeId = None, childs = [1, start_child_0 + 1])

				node_list = [start_node] + node_list

				cur_node_id = back_to_compose_1st_node_id
				compose_sync_node = march.make_serv_path_node(servName = "compose", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
					nodeId = cur_node_id, needSync = False, syncNodeId = None, childs = [cur_node_id + 1])
				node_list.append(compose_sync_node)

				func[file]("file", cur_node_id + 1, back_to_compose_2nd_node_id, node_list)

				cur_node_id = back_to_compose_2nd_node_id
				compose_final_node = march.make_serv_path_node(servName = "compose", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
					nodeId = cur_node_id, needSync = False, syncNodeId = None, childs = [cur_node_id + 1])

				client_node = march.make_serv_path_node(servName = "client", servDomain = "", codePath = 0, startStage = 0, endStage = -1, 
					nodeId = cur_node_id + 1, needSync = False, syncNodeId = None, childs = [])

				node_list += [compose_final_node, client_node]
				path_prob = 100.0 * user_prob[user] * tweet_prob[tweet] * file_prob[file]
				path = march.make_serv_path(pathId = path_id, entry = 0, prob = path_prob, nodes = node_list)

				for node in node_list:
					print 'node id ', str(node["node_id"]), ', service: ', node["service_name"], ", next node id: ", node["childs"]

				print "path ", path_id, ", prob ", path_prob, " done\n"

				path_id += 1

				paths += [path]

	with open("./json/path.json", "w+") as f:
		json.dump(paths, f, indent=2)

if __name__ == "__main__":
	main()