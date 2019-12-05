import sys
import os
import json
import make_arch as march 

# memcached
def main():
	# a dummy stage with 0 processing time
	# used mainly for synchronization
	recvTm = march.make_time_model("const", [0])
	respTm = None
	proc = march.make_stage(stage_name = "proc_load_bal", pathId = 0, pathStageId = 0, stageId = 0, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None,
		scaleFactor = 0.0)
	path = march.make_code_path(pathId = 0, prob = 1.0, stages=[proc], priority = None)


	# load balancer
	ldb = march.make_micro_service(servType = "micro_service", servName = "load_balancer", bindConn = True, paths = [path],
		baseFreq = 2.6, curFreq = 2.6)

	with open("./json/microservice/load_balancer.json", "w+") as f:
		json.dump(ldb, f, indent=2)


if __name__ == "__main__":
	main();