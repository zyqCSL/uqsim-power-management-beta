# # # # # # # # # #
# micro-service
# # # # # # # # # #
def make_chunk_model(kind, params):
	cm = {}
	assert type(kind) is str
	if kind == "expo":
		cm["type"] = kind
		cm["number"] = params[0]
	elif kind == "const":
		cm["type"] = kind
		cm["number"] = params[0]
	else:
		print "invalid chunk model type"
		exit(1)

	return cm

def make_time_model(kind, params):
	tm = {}
	assert type(kind) is str
	if kind == "expo":
		tm["type"] = kind
		tm["latency"] = params[0]
	elif kind == "const":
		tm["type"] = kind
		tm["latency"] = params[0]
	else:
		print "invalid scheduler type"
		exit(1)

	return tm


def make_stage(stage_name, pathId, pathStageId, stageId, blocking, batching, socket, epoll, ngx, net, chunk, recvTm, respTm, cm, 
	criSec, threadLimit):
	stage = {}

	assert type(stage_name) is str
	stage["stage_name"] = stage_name

	assert type(pathId) is int
	stage["code_path_id"] = pathId

	assert type(pathStageId) is int
	stage["path_stage_id"] = pathStageId

	assert type(stageId) is int
	stage["stage_id"] = stageId

	assert type(blocking) is bool
	stage["blocking"] = blocking

	assert type(batching) is bool
	stage["batching"] = batching

	assert type(socket) is bool
	stage["socket"] = socket

	assert type(epoll) is bool
	stage["epoll"] = epoll

	assert type(ngx) is bool
	stage["ngx_proc"] = ngx

	assert type(net) is bool
	stage["net"] = net

	assert type(chunk) is bool
	stage["chunk"] = chunk

	assert type(recvTm) is dict
	stage["recv_time_model"] = recvTm

	if respTm is not None:
		assert type(respTm) is dict
		stage["resp_time_model"] = respTm

	if cm != None:
		assert type(cm) is dict
		stage["chunk_model"] = cm

	assert type(criSec) is bool
	stage["critical_section"] = criSec

	if threadLimit is None:
		assert criSec is False
	else:
		assert type(threadLimit) is int
		stage["thread_limit"] = threadLimit

	return stage

def make_code_path(pathId, prob, stages, priority):
	path = {}
	assert type(pathId) is int
	path["code_path_id"] = pathId

	if prob is not None:
		assert type(prob) is float
		path["probability"] = prob

	assert type(stages) is list
	path["stages"] = stages
	path["num_stages"] = len(stages)

	if priority is not None:
		assert type(priority) is list
		path["priority"] = priority
		assert len(stages) == len(priority)

	return path

def make_CMT_core_aff(thread, cores):
	aff = {}
	aff["thread"] = thread
	assert type(cores) is list
	aff["cores"] = cores
	return aff

def make_Simp_core_aff(queue, cores):
	aff = {}
	aff["queue"] = queue
	assert type(cores) is list
	aff["cores"] = cores
	return aff

def make_service_sched(kind, params, coreAffinity):
	sched = {}
	sched["type"] = kind
	if kind == "CMT":
		assert type(params[0]) is int
		sched["num_threads"] = params[0]
		assert type(params[1]) is list
		sched["cores"] = params[1]
		if coreAffinity is not None:
			assert type(coreAffinity) is dict
			sched["core_affinity"] = coreAffinity

	elif kind == "Simplified" or kind == "LinuxNetStack":
		assert type(params[0]) is int
		sched["num_queues"] = params[0]
		assert type(params[1]) is list
		sched["cores"] = params[1]
		if coreAffinity is not None:
			assert type(coreAffinity) is list
			sched["core_affinity"] = coreAffinity

	else:
		print "Invalid scheduler type"
		exit(1)

	return sched

def make_micro_service(servType, servName, bindConn, paths):
	service = {}
	assert type(servType) is str
	service["type"] = servType

	assert type(servType) is str
	service["service_name"] = servName

	assert type(bindConn) is bool
	service["bind_connection"] = bindConn

	assert type(paths) is list
	service["paths"] = paths

	# assert type(sched) is dict
	# service["scheduler"] = sched

	return service

# # # # # # # # # #
# micro-service path
# # # # # # # # # #
def make_serv_path_node(servName, servDomain, codePath, startStage, endStage, nodeId, needSync, syncNodeId, childs):
	node = {}
	
	assert type(servName) is str
	node["service_name"] = servName

	assert type(servDomain) is str
	node["service_domain"] = servDomain

	assert type(codePath) is int
	node["code_path"] = codePath

	assert type(startStage) is int
	node["start_stage"] = startStage

	assert type(endStage) is int
	node["end_stage"] = endStage

	assert type(nodeId) is int
	node["node_id"] = nodeId

	assert type(needSync) is bool
	node["sync"] = needSync

	if syncNodeId is None:
		assert needSync is False
	else:
		assert needSync
		assert type(syncNodeId) is int
		node["sync_node_id"] = syncNodeId

	assert type(childs) is list
	node["childs"] = childs

	return node


def make_serv_path(pathId, entry, prob, nodes):
	path = {}

	assert type(pathId) is int
	path["micro_service_path_id"] = pathId

	assert type(entry) is int
	path["entry"] = entry

	assert type(prob) is float
	path["probability"] = prob

	assert type(nodes) is list
	path["nodes"] = nodes

	return path


# # # # # # # # # #
# dependency graph
# # # # # # # # # #
# make a micro-service instance in cluster
def make_serv_inst(servName, servDomain, instName, sched, machId):
	servInst = {}

	assert type(servName) is str
	servInst["service_name"] = servName

	assert type(servDomain) is str
	servInst["service_domain"] = servDomain

	assert type(instName) is str
	servInst["instance_name"] = instName

	assert type(sched) is dict
	servInst["scheduler"] = sched

	assert type(machId) is int
	servInst["machine_id"] = machId

	return servInst


def make_edge(src, targ, bidir):
	edge = {}

	assert type(src) is str
	edge["source"] = src

	assert type(targ) is str
	edge["target"] = targ

	assert type(bidir) is bool
	edge["biDirectional"] = bidir

	return edge

def make_cluster(services, edges, netLat):
	cluster = {}

	assert type(services) is list
	cluster["microservices"] = services

	assert type(edges) is list
	cluster["edges"] = edges

	assert type(netLat) is float
	cluster["net_latency"] = netLat

	return cluster

# # # # # # # # # #
# machines
# # # # # # # # # #
# def make_machine(mid, name, cores, threads, netSched):
def make_machine(mid, name, cores, netSched):
	machine = {}
	assert type(mid) is int
	machine["machine_id"] = mid

	assert type(name) is str
	machine["name"] = name

	assert type(cores) is int
	machine["total_cores"] = cores

	# assert type(threads) is int
	# machine["total_threads"] = threads

	assert type(netSched) is dict
	machine["net_stack_sched"] = netSched

	return machine