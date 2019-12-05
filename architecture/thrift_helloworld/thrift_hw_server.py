import sys
import os
import json
import make_arch as march 

# thrift_hw
def main():
	# req path
	recvTm = march.make_time_model("expo", [1500])
	respTm = None
	proc_stage = march.make_stage(stage_name = "thrift_proc", pathId = 0, pathStageId = 0, stageId = 0, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False,  net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None, 
		scaleFactor = 0.55, useHist = True)

	req_path = march.make_code_path(pathId = 0, prob = 100, stages=[proc_stage], priority = None)

	# thrift_hw
	thrift_hw = march.make_micro_service(servType = "micro_service", servName = "thrift_hw", bindConn = True, paths = [req_path], 
		baseFreq = 2600, curFreq = 2600)

	with open("./json/microservice/thrift_hw.json", "w+") as f:
		json.dump(thrift_hw, f, indent=2)


if __name__ == "__main__":
	main();