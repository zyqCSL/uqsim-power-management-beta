import sys
import os
import json
import make_arch as march 


def main():
	sched = march.make_service_sched("CMT", [1, [20]], None)
	compose = march.make_serv_inst(servName = "compose", servDomain = "", instName = "compose", 
		modelName = "compose", sched = sched, machId = 0)

	# user service
	sched = march.make_service_sched("CMT", [1, [31]], None)
	user = march.make_serv_inst(servName = "user", servDomain = "", instName = "user", 
		modelName = "user", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [1, [32]], None)
	user_memcached = march.make_serv_inst(servName = "user_memcached", servDomain = "", instName = "user_memcached", 
		modelName = "memcached", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [1, [34]], None)
	user_mongodb = march.make_serv_inst(servName = "user_mongodb", servDomain = "", instName = "user_mongodb", 
		modelName = "mongodb", sched = sched, machId = 0)

	user_mongodb_io = march.make_serv_inst(servName = "user_mongodb_io", servDomain = "", instName = "user_mongodb_io", 
		modelName = "mongo_io", sched = sched, machId = 0)

	# tweet service
	sched = march.make_service_sched("CMT", [1, [41]], None)
	tweet = march.make_serv_inst(servName = "tweet", servDomain = "", instName = "tweet", 
		modelName = "tweet", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [1, [42]], None)
	tweet_memcached = march.make_serv_inst(servName = "tweet_memcached", servDomain = "", instName = "tweet_memcached", 
		modelName = "memcached", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [1, [43]], None)
	tweet_mongodb = march.make_serv_inst(servName = "tweet_mongodb", servDomain = "", instName = "tweet_mongodb", 
		modelName = "mongodb", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [1, [44]], None)
	tweet_mongodb_io = march.make_serv_inst(servName = "tweet_mongodb_io", servDomain = "", instName = "tweet_mongodb_io", 
		modelName = "mongo_io", sched = sched, machId = 0)

	# file service
	sched = march.make_service_sched("CMT", [1, [51]], None)
	file = march.make_serv_inst(servName = "file", servDomain = "", instName = "file", 
		modelName = "file", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [1, [52]], None)
	file_memcached = march.make_serv_inst(servName = "file_memcached", servDomain = "", instName = "file_memcached", 
		modelName = "memcached", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [1, [53]], None)
	file_mongodb = march.make_serv_inst(servName = "file_mongodb", servDomain = "", instName = "file_mongodb", 
		modelName = "mongodb", sched = sched, machId = 0)

	sched = march.make_service_sched("CMT", [1, [54]], None)
	file_mongodb_io = march.make_serv_inst(servName = "file_mongodb_io", servDomain = "", instName = "file_mongodb_io", 
		modelName = "mongo_io", sched = sched, machId = 0)

	services = [compose,
				user, user_memcached, user_mongodb, user_mongodb_io,
				tweet, tweet_memcached, tweet_mongodb, tweet_mongodb_io,
				file, file_memcached, file_mongodb, file_mongodb_io]

	edges = []
	# compose
	edges.append( march.make_edge(src = "compose", targ = "user", bidir = True) )
	edges.append( march.make_edge(src = "compose", targ = "tweet", bidir = True) )
	edges.append( march.make_edge(src = "compose", targ = "file", bidir = True) )
	# user
	edges.append( march.make_edge(src = "user", targ = "user_memcached", bidir = True) )
	edges.append( march.make_edge(src = "user", targ = "user_mongodb", bidir = True) )
	edges.append( march.make_edge(src = "user_mongodb", targ = "user_mongodb_io", bidir = True) )
	# tweet
	edges.append( march.make_edge(src = "tweet", targ = "tweet_memcached", bidir = True) )
	edges.append( march.make_edge(src = "tweet", targ = "tweet_mongodb", bidir = True) )
	edges.append( march.make_edge(src = "tweet_mongodb", targ = "tweet_mongodb_io", bidir = True) )
	# file
	edges.append( march.make_edge(src = "file", targ = "file_memcached", bidir = True) )
	edges.append( march.make_edge(src = "file", targ = "file_mongodb", bidir = True) )
	edges.append( march.make_edge(src = "file_mongodb", targ = "file_mongodb_io", bidir = True) )



	graph = march.make_cluster(services = services, edges = edges, netLat = 65000)

	with open("./json/graph.json", "w+") as f:
		json.dump(graph, f, indent=2)

if __name__ == "__main__":
	main()

