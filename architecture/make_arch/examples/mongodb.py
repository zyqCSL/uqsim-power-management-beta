import sys
import os
import json
import make_arch as march 

# mongodb
def main():
	# cache hit path
	recvTm = march.make_time_model("expo", [75.0])
	respTm = None
	mem_hit_stage = march.make_stage(stage_name = "mem_access_hit", pathId = 0, pathStageId = 0, stageId = 0, blocking = False, batching = False, socket = False, 
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	cache_hit_path = march.make_code_path(pathId = 0, prob = None, stages=[mem_hit_stage], priority = None)

	# cache miss path
	recvTm = march.make_time_model("expo", [100.0])
	respTm = march.make_time_model("const", [10.0])
	mem_miss_stage = march.make_stage(stage_name = "mem_access_miss", pathId = 1, pathStageId = 0, stageId = 0, blocking = False, batching = False, socket = False, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("const", [0.0])
	respTm = march.make_time_model("expo", [100.0])
	chkm = march.make_chunk_model("expo", [3])
	io_stage = march.make_stage(stage_name = "io", pathId = 1, pathStageId = 1, stageId = 1, blocking = True, batching = False, socket = False, 
		net = False, chunk = True, recvTm = recvTm, respTm = respTm, cm = chkm, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("expo", [80.0])
	respTm = None
	mem_fill_stage = march.make_stage(stage_name = "mem_fill", pathId = 1, pathStageId = 2, stageId = 2, blocking = False, batching = False, socket = False, 
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	cache_miss_path = march.make_code_path(pathId = 1, prob = None, stages=[mem_miss_stage, io_stage, mem_fill_stage], priority = None)


	# mongodb
	mongodb = march.make_micro_service(servType = "micro_service", servName = "mongodb", bindConn = True, paths = [cache_hit_path, cache_miss_path])

	with open("./json/microservice/mongodb.json", "w+") as f:
		json.dump(mongodb, f, indent=2)


if __name__ == "__main__":
	main();