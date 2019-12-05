import sys
import os
import json
import make_arch as march 

# mongodb
def main():
	# sock recv
	recvTm = march.make_time_model("expo", [3000])
	respTm = None
	recv_stage = march.make_stage(stage_name = "sock_recv", pathId = 0, pathStageId = 0, stageId = 0, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, scaleFactor = 0.0,
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	# cache hit path
	recvTm = march.make_time_model("expo", [75000])
	respTm = None
	cache_hit_stage = march.make_stage(stage_name = "proc_cache_hit", pathId = 0, pathStageId = 1, stageId = 1, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, scaleFactor = 0.0,
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	cache_hit_path = march.make_code_path(pathId = 0, prob = None, stages=[recv_stage, cache_hit_stage], priority = None)

	# cache miss path
	recvTm = march.make_time_model("expo", [3000])
	respTm = None
	recv_stage = march.make_stage(stage_name = "sock_recv", pathId = 1, pathStageId = 0, stageId = 2, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, scaleFactor = 0.0,
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("expo", [100000])
	mem_miss_stage = march.make_stage(stage_name = "assemble_cache_miss", pathId = 1, pathStageId = 1, stageId = 3, blocking = True, batching = False, socket = False, 
		epoll = False, ngx = False, scaleFactor = 0.0,
		net = False, chunk = False, recvTm = recvTm, respTm = None, cm = None, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("expo", [100000])
	io_stage = march.make_stage(stage_name = "proc_cache_miss", pathId = 1, pathStageId = 2, stageId = 4, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, scaleFactor = 0.0,
		net = True, chunk = False, recvTm = recvTm, respTm = None, cm = None, criSec = False, threadLimit = None)

	# recvTm = march.make_time_model("expo", [80.0])
	# respTm = None
	# mem_fill_stage = march.make_stage(stage_name = "mem_fill", pathId = 1, pathStageId = 2, stageId = 2, blocking = False, batching = False, socket = False, 
	# 	net = True, chunk = False, recvTm = recvTm, respTm = None, cm = None, criSec = False, threadLimit = None)

	cache_miss_path = march.make_code_path(pathId = 1, prob = None, stages=[recv_stage, mem_miss_stage, io_stage], priority = None)


	# mongodb
	mongodb = march.make_micro_service(servType = "micro_service", servName = "mongodb", bindConn = True, paths = [cache_hit_path, cache_miss_path], 
										baseFreq = 2600, curFreq = 2600)

	with open("./json/microservice/mongodb.json", "w+") as f:
		json.dump(mongodb, f, indent=2)


if __name__ == "__main__":
	main();