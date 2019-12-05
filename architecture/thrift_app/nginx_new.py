import sys
import os
import json
import make_arch as march 

# nginx
def main():
	# req path
	recvTm = march.make_time_model("expo", [1500])
	respTm = None
	epoll_stage = march.make_stage(stage_name = "epoll", pathId = 0, pathStageId = 0, stageId = 0, blocking = False, batching = True, socket = False, 
		epoll = True, ngx = False,  net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None, 
		scaleFactor = 0.55, useHist = True)

	recvTm = march.make_time_model("expo", [19000])
	respTm = None
	proc_req_stage = march.make_stage(stage_name = "ngx_proc", pathId = 0, pathStageId = 1, stageId = 1, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = True,  net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None, 
		scaleFactor = 0.4, useHist = True)


	req_path = march.make_code_path(pathId = 0, prob = 100, stages=[epoll_stage, proc_req_stage], priority = None)

	# resp path

	recvTm = march.make_time_model("expo", [19000])
	respTm = None
	proc_resp_stage = march.make_stage(stage_name = "ngx_proc", pathId = 0, pathStageId = 1, stageId = 1, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = True,  net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None, 
		scaleFactor = 1.0, useHist = True)

	resp_path = march.make_code_path(pathId = 1, prob = None, stages=[epoll_stage, proc_resp_stage], priority = None)

	# nginx
	nginx = march.make_micro_service(servType = "micro_service", servName = "nginx", bindConn = True, paths = [req_path], 
		baseFreq = 2600, curFreq = 2600)

	with open("./json/microservice/nginx.json", "w+") as f:
		json.dump(nginx, f, indent=2)


if __name__ == "__main__":
	main();