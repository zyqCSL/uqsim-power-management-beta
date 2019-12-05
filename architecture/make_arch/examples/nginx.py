import sys
import os
import json
import make_arch as march 

# nginx
def main():
	# req path
	recvTm = march.make_time_model("expo", [1.5])
	respTm = None
	epoll_stage = march.make_stage(stage_name = "epoll", pathId = 0, pathStageId = 0, stageId = 0, blocking = False, batching = True, socket = False, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("expo", [2.0])
	respTm = None
	socket_stage = march.make_stage(stage_name = "socket", pathId = 0, pathStageId = 1, stageId = 1, blocking = False, batching = True, socket = True, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("expo", [40.0])
	respTm = None
	req_stage = march.make_stage(stage_name = "proc_req", pathId = 0, pathStageId = 2, stageId = 2, blocking = False, batching = False, socket = False, 
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	req_path = march.make_code_path(pathId = 0, prob = None, stages=[epoll_stage, socket_stage, req_stage], priority = None)

	# mmc path
	recvTm = march.make_time_model("expo", [1.5])
	respTm = None
	epoll_stage = march.make_stage(stage_name = "epoll", pathId = 1, pathStageId = 0, stageId = 0, blocking = False, batching = True, socket = False, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("expo", [2.0])
	respTm = None
	socket_stage = march.make_stage(stage_name = "socket", pathId = 1, pathStageId = 1, stageId = 1, blocking = False, batching = True, socket = True, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("expo", [40.0])
	respTm = None
	mmc_stage = march.make_stage(stage_name = "proc_mmc", pathId = 1, pathStageId = 2, stageId = 3, blocking = False, batching = False, socket = False, 
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	mmc_path = march.make_code_path(pathId = 1, prob = None, stages=[epoll_stage, socket_stage, mmc_stage], priority = None)

	# php path
	recvTm = march.make_time_model("expo", [1.5])
	respTm = None
	epoll_stage = march.make_stage(stage_name = "epoll", pathId = 2, pathStageId = 0, stageId = 0, blocking = False, batching = True, socket = False, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("expo", [2.0])
	respTm = None
	socket_stage = march.make_stage(stage_name = "socket", pathId = 2, pathStageId = 1, stageId = 1, blocking = False, batching = True, socket = True, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	recvTm = march.make_time_model("expo", [20.0])
	respTm = None
	php_stage = march.make_stage(stage_name = "proc_php", pathId = 2, pathStageId = 2, stageId = 4, blocking = False, batching = False, socket = False, 
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	php_path = march.make_code_path(pathId = 2, prob = None, stages=[epoll_stage, socket_stage, php_stage], priority = None)


	# nginx
	nginx = march.make_micro_service(servType = "micro_service", servName = "nginx", bindConn = True, paths = [req_path, mmc_path, php_path])

	with open("./json/microservice/nginx.json", "w+") as f:
		json.dump(nginx, f, indent=2)


if __name__ == "__main__":
	main();