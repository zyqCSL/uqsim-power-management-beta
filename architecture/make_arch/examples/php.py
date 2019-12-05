import sys
import os
import json
import make_arch as march 

# php
def main():
	# read path
	# fcgi_req, goes to php_io
	recvTm = march.make_time_model("expo", [40.0])
	respTm = march.make_time_model("expo", [40.0])
	fcgi_req_stage = march.make_stage(stage_name = "php_fcgi_req", pathId = 0, pathStageId = 0, stageId = 0, blocking = True, batching = False, socket = False, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	# fopen, goes to php_io
	recvTm = march.make_time_model("expo", [48.0])
	respTm = march.make_time_model("expo", [48.0])
	fopen_stage = march.make_stage(stage_name = "php_fopen_req", pathId = 0, pathStageId = 1, stageId = 1, blocking = True, batching = False, socket = False, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	# fput, goes to mongo
	recvTm = march.make_time_model("expo", [40.0])
	respTm = march.make_time_model("expo", [40.0])
	fput_stage = march.make_stage(stage_name = "php_fput", pathId = 0, pathStageId = 2, stageId = 2, blocking = True, batching = False, socket = False, 
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	# find_done, goes to mongo
	recvTm = march.make_time_model("expo", [40.0])
	respTm = march.make_time_model("expo", [40.0])
	find_done_stage = march.make_stage(stage_name = "php_find_done", pathId = 0, pathStageId = 3, stageId = 3, blocking = True, batching = False, socket = False, 
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	# get_bytes, goes to memcached
	recvTm = march.make_time_model("expo", [40.0])
	respTm = march.make_time_model("expo", [40.0])
	get_bytes_stage = march.make_stage(stage_name = "php_get_bytes", pathId = 0, pathStageId = 4, stageId = 4, blocking = True, batching = False, socket = False, 
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	# mmc_store, goes to nginx
	recvTm = march.make_time_model("expo", [40.0])
	respTm = march.make_time_model("expo", [40.0])
	mmc_store_stage = march.make_stage(stage_name = "php_mmc_store", pathId = 0, pathStageId = 5, stageId = 5, blocking = True, batching = False, socket = False, 
		net = True, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	path = march.make_code_path(pathId = 0, prob = 1.0, 
		stages=[fcgi_req_stage, fopen_stage, fput_stage, find_done_stage, get_bytes_stage, mmc_store_stage], priority = None)


	php = march.make_micro_service(servType = "micro_service", servName = "php", bindConn = False, paths = [path])


	with open("./json/microservice/php.json", "w+") as f:
		json.dump(php, f, indent=2)


if __name__ == "__main__":
	main();