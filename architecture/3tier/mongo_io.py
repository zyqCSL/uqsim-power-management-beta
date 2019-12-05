import sys
import os
import json
import make_arch as march 

# mongo_io
def main():
	# proc path
	recvTm = march.make_time_model("expo", [5000000])	# 5000 us
	respTm = None

	cmodel = march.make_chunk_model("expo", [2])

	proc_stage = march.make_stage(stage_name = "disk_io", pathId = 0, pathStageId = 0, stageId = 0, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, net = False, chunk = True, recvTm = recvTm, respTm = respTm, cm = cmodel, criSec = False, threadLimit = None,
		scaleFactor = 0.0)

	path = march.make_code_path(pathId = 0, prob = 100, stages=[proc_stage], priority = None)
	
	mongo_io = march.make_micro_service(servType = "micro_service", servName = "mongo_io", bindConn = False, paths = [path], baseFreq = 2600, curFreq = 2600)

	with open("./json/microservice/mongo_io.json", "w+") as f:
		json.dump(mongo_io, f, indent=2)


if __name__ == "__main__":
	main();