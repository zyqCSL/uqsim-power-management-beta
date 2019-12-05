import sys
import os
import json
import make_arch as march 

# mongo_io
def main():
	# proc path
	recvTm = march.make_time_model("expo", [8500.0])
	respTm = None
	proc_stage = march.make_stage(stage_name = "proc", pathId = 0, pathStageId = 0, stageId = 0, blocking = True, batching = False, socket = False, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	path = march.make_code_path(pathId = 0, prob = 1.0, stages=[proc_stage], priority = None)
	
	mongo_io = march.make_micro_service(servType = "micro_service", servName = "mongo_io", bindConn = False, paths = [path])

	with open("./json/microservice/mongo_io.json", "w+") as f:
		json.dump(mongo_io, f, indent=2)


if __name__ == "__main__":
	main();