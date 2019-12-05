import sys
import os
import json
import make_arch as march 

# memcached
def main():
	# codepath 0: tcp send to localhost
	recvTm = march.make_time_model("expo", [1500])
	respTm = None
	proc_send_local = march.make_stage(stage_name = "proc_send_local", pathId = 0, pathStageId = 0, stageId = 0, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None,
		scaleFactor = 0.0)
	send_local = march.make_code_path(pathId = 0, prob = None, stages=[proc_send_local], priority = None)

	# codepath 1: tcp send to remote host
	recvTm = march.make_time_model("expo", [1000])
	respTm = None
	proc_send_remote = march.make_stage(stage_name = "proc_send_remote", pathId = 1, pathStageId = 0, stageId = 1, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None, 
		scaleFactor = 0.0)
	send_remote = march.make_code_path(pathId = 1, prob = None, stages=[proc_send_remote], priority = None)

	# codepath 2: tcp recv from local host
	recvTm = march.make_time_model("expo", [1500])
	respTm = None
	proc_recv_local = march.make_stage(stage_name = "proc_recv_local", pathId = 2, pathStageId = 0, stageId = 2, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None, 
		scaleFactor = 0.0)
	recv_local = march.make_code_path(pathId = 2, prob = None, stages=[proc_recv_local], priority = None)

	# codepath 3: tcp recv from remote host
	recvTm = march.make_time_model("expo", [4000])
	respTm = None
	proc_recv_remote = march.make_stage(stage_name = "proc_recv_remote", pathId = 3, pathStageId = 0, stageId = 3, blocking = False, batching = False, socket = False, 
		epoll = False, ngx = False, net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None, 
		scaleFactor = 0.0)
	recv_remote = march.make_code_path(pathId = 3, prob = None, stages=[proc_recv_remote], priority = None)

	# tcp
	tcp = march.make_micro_service(servType = "micro_service", servName = "net_stack", bindConn = True, paths = [send_local, send_remote, recv_local, recv_remote],
		baseFreq = 2600, curFreq = 2600)

	with open("./json/microservice/net_stack.json", "w+") as f:
		json.dump(tcp, f, indent=2)


if __name__ == "__main__":
	main();