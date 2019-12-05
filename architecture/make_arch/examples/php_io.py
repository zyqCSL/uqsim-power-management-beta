import sys
import os
import json
import make_arch as march 

# php_io
def main():
	# fopen path
	recvTm = march.make_time_model("expo", [3.0])
	respTm = None
	fopen_stage = march.make_stage(stage_name = "fopen", pathId = 0, pathStageId = 0, stageId = 0, blocking = False, batching = False, socket = False, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	fopen_path = march.make_code_path(pathId = 0, prob = None, stages=[fopen_stage], priority = None)

	# fput path
	recvTm = march.make_time_model("expo", [1.0])
	respTm = None
	fput_stage = march.make_stage(stage_name = "fput", pathId = 1, pathStageId = 0, stageId = 1, blocking = False, batching = False, socket = False, 
		net = False, chunk = False, recvTm = recvTm, respTm = respTm, cm = None, criSec = False, threadLimit = None)

	fput_path = march.make_code_path(pathId = 1, prob = None, stages=[fput_stage], priority = None)

	php_io = march.make_micro_service(servType = "micro_service", servName = "php_io", bindConn = False, paths = [fopen_path, fput_path])

	with open("./json/microservice/php_io.json", "w+") as f:
		json.dump(php_io, f, indent=2)


if __name__ == "__main__":
	main();