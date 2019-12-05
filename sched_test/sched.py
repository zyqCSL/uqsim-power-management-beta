import os
import subprocess
import sys
import time
from socket import SOCK_STREAM, socket, AF_INET, SOL_SOCKET, SO_REUSEADDR
import time
import random
import json
import collections
import numpy as np

# NGX_COMM_PORT = 40001
# MEMC_COMM_PORT = 40002

IP_ADDR = {}
IP_ADDR["ath1"]     = "1.2.8.4"
IP_ADDR["ath2"]     = "1.2.8.5"
IP_ADDR["ath3"]     = "1.2.8.6"
IP_ADDR["ath4"]     = "1.2.8.7"
IP_ADDR["ath5"]     = "1.2.8.8"
IP_ADDR["ath8"]     = "1.2.8.9"

HIGHEST_FREQ = {}		# MHz
LOWEST_FREQ  = {}
FREQ_RESO	 = {}
for serv in IP_ADDR:
	if serv != "ath8":
		HIGHEST_FREQ[serv] = 2600
		LOWEST_FREQ[serv]  = 1200
		FREQ_RESO[serv]	 = 200
	else:
		HIGHEST_FREQ[serv] = 2200
		LOWEST_FREQ[serv]  = 1200
		FREQ_RESO[serv]	 = 100

CUR_ROUND			= 0		# hand shaking with simulator
CONFIG 			    = './sched_config.txt'
STATS_FILE 		    = './simulated_stats.txt'
DEC_FILE			= './sched_decision.txt'
RUN_TIME			= 60	# seconds
TOTAL_ROUND			= 60	# tototal rounds to run (RUN_TIME/INTERVAL)
INTERVAL 			= 1		# seconds
TRACE_LIMIT 		= 200
PORTS 			    = {}	# indexed by server
SOCKS 			    = {}
APP_SERVER 		    = {}
APP_CORE_NUM		= {}
SERVER_APP			= {}
APP_FREQ_VEC 	    = {}

# THRESHOLD           = 0.8
# one threshold record per load interval

SCHED_STATE			 = 'EXPLORE'	
QOS_SLOT_NUM		 = 20
QOS_SLOT_STEP		 = -1
POINT_STATIC_CYCLE	 = 20			# stay in a point for # cycles before switching to other points
POINT_STATIC_COUNTER = 0		

CUR_OPT_POINT		 = None
VIOL_POINT			 = None

OBSERVE_CYCLE 		 = 5			# cycles to observe if qos violation is caused after a slow down decision
OBSERVE_COUNTER		 = 0

ONLINE_GRAPHS		 = []
CUR_GRAPH_IDX		 = -1
OBSERVE_GRAPH_IDX	 = -1			# the corresponding graph at which current observe action starts
VIOL_GRAPH_IDX		 = -1			# the graph that causes qos violation

CUR_QPS				 = 0
QPS_EXPERIENCE	 	 = []			# load ranges we have seen after choosing a target point or start observing 
QPS_RANGE_STEP		 = 5000

def get_qps_range():
	global CUR_QPS
	global QPS_RANGE_STEP
	if CUR_QPS == 0:
		return 0
	elif CUR_QPS % QPS_RANGE_STEP == 0:
		return CUR_QPS
	else:
		CUR_QPS = (int(CUR_QPS/QPS_RANGE_STEP) + 1) * QPS_RANGE_STEP
		print 'CUR_QPS (ranged) = ', CUR_QPS
		return CUR_QPS

class GraphPoint:
	def __init__(self, end2end_lat, lat_vec):
		self.end2end_lat  	  = end2end_lat
		self.lat_vec 	  	  = lat_vec

	# if current point dominates graph_point
	def dominate(self, graph_point):
		dominate = True
		for key in self.lat_vec:
			dominate &= self.lat_vec[key] > graph_point.lat_vec[key]
			if not dominate:
				return False
		return True

	def equal(self, graph_point):
		eq = True
		for key in self.lat_vec:
			eq &= (format(self.lat_vec[key], '.2f') == format(graph_point.lat_vec[key], '.2f'))
			if not eq:
				return False
		return True

	def show(self):
		string = 'end2end: ' + str(self.end2end_lat) + '\t' + '\t'.join( (key + ':' + str(self.lat_vec[key])) for key in self.lat_vec)
		return string

# currently only evaluate on the granularity of graph, not point
class OnlineGraph:
	def __init__(self, min_lat, max_lat):
		self.graph			  		  = []		# optimistic as long as QoS is met
		self.min_lat				  = min_lat
		self.max_lat 				  = max_lat
		self.viol_stats 			  = {}
		self.viol_stats['run_time']   = 0
		self.viol_stats['total_viol'] = 0
		self.probability			  = 0		# the probability of using this graph for scheduling
		self.witnessed				  = False

	def insert(self, new_point):
		new_graph = []
		insert = True
		for point in self.graph:
			if point.dominate(new_point):
				insert = False
				break
			elif not new_point.dominate(point):
				new_graph.append(point)
		if insert:
			self.graph = new_graph
			self.graph.append(new_point)

	def delete(self, vic_point):
		new_graph = []
		for point in self.graph:
			if not point.equal(vic_point):
				new_graph.append(point)
		self.graph = new_graph

	def get_optimal_point(self, cur_lat_vec):
		assert len(self.graph) > 0
		global CUR_OPT_POINT
		min_val = -1.0
		# print 'cur_lat_vec = ', cur_lat_vec
		for point in self.graph:
			cur_val = 0.0
			for key in point.lat_vec:
				cur_val += point.lat_vec[key]/cur_lat_vec[key]

			if min_val < 0 or cur_val < min_val:
				min_val = cur_val
				CUR_OPT_POINT = point
		print 'CUR_OPT_POINT: ', CUR_OPT_POINT.show()
		return CUR_OPT_POINT

	def show_graph(self):
		print 'graph for end2end_tail in [', self.min_lat, ', ', self.max_lat, ']:' 
		for point in self.graph:
			print point.show()

def init_online_graphs(target):
	global ONLINE_GRAPHS
	global QOS_SLOT_NUM
	global QOS_SLOT_STEP
	QOS_SLOT_STEP = float(target)/float(QOS_SLOT_NUM)	
	print 'QOS_SLOT_STEP = ', QOS_SLOT_STEP
	i = 0
	lower_bound = 0.0
	upper_bound = 0.0
	while i < QOS_SLOT_NUM:
		lower_bound = upper_bound
		upper_bound += QOS_SLOT_STEP
		ONLINE_GRAPHS.append(OnlineGraph(lower_bound, upper_bound))
		i += 1

def get_graph_idx(end2end_lat):
	global QOS_SLOT_NUM
	global QOS_SLOT_STEP
	idx = int(end2end_lat/QOS_SLOT_STEP)
	assert idx < QOS_SLOT_NUM
	return idx

def insert_new_point(new_point):
	global ONLINE_GRAPHS
	global QOS_SLOT_STEP
	print 'insert_new_point'
	idx = get_graph_idx(new_point.end2end_lat)
	print 'inserted idx = ', idx
	explore = False
	if not ONLINE_GRAPHS[idx].witnessed:
		ONLINE_GRAPHS[idx].witnessed = True
		check_idx = idx + 1
		explore = True
		while check_idx < QOS_SLOT_NUM:
			explore &= (not ONLINE_GRAPHS[check_idx].witnessed)
			check_idx += 1
		# set the probability to closest one down
		lower_idx = idx - 1
		while lower_idx >= 0:
			print 'test explore in insert_new_point, lower_idx = ', lower_idx
			if len(ONLINE_GRAPHS[lower_idx].graph) > 0:
				print 'prob = ', ONLINE_GRAPHS[lower_idx].probability
				ONLINE_GRAPHS[idx].probability = ONLINE_GRAPHS[lower_idx].probability
				break
			lower_idx -= 1
		if lower_idx < 0:
			ONLINE_GRAPHS[idx].probability = 1.0
	ONLINE_GRAPHS[idx].insert(new_point)
	# ONLINE_GRAPHS[idx].show_graph()

	print 'whole graph probability: '
	i = QOS_SLOT_NUM - 1
	while i >= 0:
		if ONLINE_GRAPHS[i].witnessed:
			print 'graph idx: ', i, '[', i*QOS_SLOT_STEP, ', ', (i+1)*QOS_SLOT_STEP, '], prob = ', ONLINE_GRAPHS[i].probability, 'graph_size = ', len(ONLINE_GRAPHS[i].graph)
		i -= 1
	print 'insert done'
	return explore

def probablistic_accpet(prob):
	assert prob >= 0.0
	assert prob <= 1.0
	val = random.uniform(0.0, 1.0)
	print 'prob = ', prob
	print 'generated val = ', val
	if val > prob:
		return False
	else:
		return True

def adjust_graph_probability(idx, sched_suc):
	print 'adjust_graph_probability'
	global QOS_SLOT_NUM
	global QOS_SLOT_STEP
	global ONLINE_GRAPHS

	if sched_suc:
		while idx >= 0:
			ONLINE_GRAPHS[idx].probability = min(1.0, ONLINE_GRAPHS[idx].probability * 1.1)
			print 'sched_suc graph idx ', idx, ' [', idx*QOS_SLOT_STEP, ', ', (idx+1)*QOS_SLOT_STEP, '], graph_size = ', len(ONLINE_GRAPHS[idx].graph), ', prob = ', ONLINE_GRAPHS[idx].probability
			idx -= 1
	else:
		while idx < QOS_SLOT_NUM:
			ONLINE_GRAPHS[idx].probability /= 4.0
			print 'sched_fail graph idx ', idx, ' [', idx*QOS_SLOT_STEP, ', ', (idx+1)*QOS_SLOT_STEP, '], graph_size = ', len(ONLINE_GRAPHS[idx].graph), ', prob = ', ONLINE_GRAPHS[idx].probability
			idx += 1

# @sched_suc, if scheduling according to current optimal graph meets qos
# @return True if acceptable graph is found, otherwise False
def get_optimal_graph(sched_suc):
	print 'get_optimal_graph'
	global ONLINE_GRAPHS
	global QOS_SLOT_NUM
	global CUR_GRAPH_IDX
	global VIOL_GRAPH_IDX

	if sched_suc:
		test_idx = 0
		least_graph_idx = -1
		if CUR_GRAPH_IDX >= 0:
			test_idx = CUR_GRAPH_IDX + 1
		while test_idx < QOS_SLOT_NUM:
			# print 'test_idx = ', test_idx, ', QOS_SLOT_NUM = ', QOS_SLOT_NUM
			if len(ONLINE_GRAPHS[test_idx].graph) > 0:
				# print 'sched_suc, test graph ', test_idx
				if least_graph_idx < 0:
					least_graph_idx = test_idx
				accept = probablistic_accpet(ONLINE_GRAPHS[test_idx].probability)
				if accept:
					CUR_GRAPH_IDX = test_idx
				else:
					return True
			test_idx += 1
		# assert least_graph_idx >= 0
		if CUR_GRAPH_IDX < 0:
			assert least_graph_idx >= 0
			CUR_GRAPH_IDX = least_graph_idx
		return CUR_GRAPH_IDX
	else:
		test_idx = 0
		least_graph_idx = -1
		if CUR_GRAPH_IDX >= 0:
			# in NORMAL state
			test_idx = CUR_GRAPH_IDX - 1
		else:
			# in EXPLORE state
			if VIOL_GRAPH_IDX < 0:
				test_idx = QOS_SLOT_NUM - 1
			else:
				test_idx = VIOL_GRAPH_IDX - 1

		while test_idx >= 0:
			# print 'test_idx = ', test_idx, ', QOS_SLOT_NUM = ', QOS_SLOT_NUM
			if len(ONLINE_GRAPHS[test_idx].graph) > 0:
				# print 'sched_fail, test graph ', test_idx
				least_graph_idx = test_idx
				accept = probablistic_accpet(ONLINE_GRAPHS[test_idx].probability)
				if accept:
					CUR_GRAPH_IDX = test_idx
					return True
			test_idx -= 1

		CUR_GRAPH_IDX = least_graph_idx
		return CUR_GRAPH_IDX >= 0

# choose one dimension to slow down
def random_slowdown(end2end_lat):
	print 'random_slowdown'
	global APP_SERVER
	global APP_FREQ_VEC
	global APP_CORE_NUM
	global LOWEST_FREQ
	global FREQ_RESO

	global OBSERVE_CYCLE
	global OBSERVE_COUNTER
	global OBSERVE_GRAPH_IDX

	OBSERVE_GRAPH_IDX = get_graph_idx(end2end_lat)

	victims = []
	for app in APP_FREQ_VEC:
		if APP_FREQ_VEC[app] == LOWEST_FREQ[APP_SERVER[app]]:
			continue
		else:
			victims.append(app)

	if len(victims) == 0:
		OBSERVE_COUNTER = 0
		return False
	else:
		OBSERVE_COUNTER = OBSERVE_CYCLE
		idx = random.randint(0, len(victims) - 1)
		vic_app = victims[idx]
		APP_FREQ_VEC[vic_app] -= FREQ_RESO[APP_SERVER[vic_app]]
		return True

def restore_full_freq():
	global APP_SERVER
	global APP_FREQ_VEC
	global HIGHEST_FREQ
	for app in APP_FREQ_VEC:
		APP_FREQ_VEC[app] = HIGHEST_FREQ[APP_SERVER[app]]

def adjust_freq(lat_vec):
	print 'adjust_freq'
	global APP_FREQ_VEC
	global APP_CORE_NUM

	global CUR_OPT_POINT

	global APP_SERVER
	global HIGHEST_FREQ
	global LOWEST_FREQ
	global FREQ_RESO

	global OBSERVE_COUNTER
	global OBSERVE_CYCLE

	assert CUR_OPT_POINT != None

	slacks = []
	for app in APP_FREQ_VEC:
		if CUR_OPT_POINT.lat_vec[app] > lat_vec[app] and APP_FREQ_VEC[app] != LOWEST_FREQ[APP_SERVER[app]]:
			slacks.append(app)

		elif CUR_OPT_POINT.lat_vec[app] <= lat_vec[app] and APP_FREQ_VEC[app] != HIGHEST_FREQ[APP_SERVER[app]]:
			APP_FREQ_VEC[app] += FREQ_RESO[APP_SERVER[app]]

	# pesimistically assume all apps' performance are correlated and only slow down one tier at a time
	if len(slacks) > 0:
		idx = random.randint(0, len(slacks) - 1)
		vic_app = slacks[idx]
		APP_FREQ_VEC[vic_app] -= FREQ_RESO[APP_SERVER[vic_app]]
		OBSERVE_COUNTER = OBSERVE_CYCLE
		return True
	else:
		return False

def adjust_freq_observe(lat_vec):
	print 'adjust_freq'
	global APP_FREQ_VEC
	global APP_CORE_NUM

	global CUR_OPT_POINT

	global APP_SERVER
	global HIGHEST_FREQ
	global LOWEST_FREQ
	global FREQ_RESO

	assert CUR_OPT_POINT != None

	slacks = []
	for app in APP_FREQ_VEC:
		if CUR_OPT_POINT.lat_vec[app] <= lat_vec[app] and APP_FREQ_VEC[app] != HIGHEST_FREQ[APP_SERVER[app]]:
			APP_FREQ_VEC[app] += FREQ_RESO[APP_SERVER[app]]

def adjust_freq_viol(lat_vec):
	print 'adjust_freq_viol'
	global APP_FREQ_VEC
	global APP_SERVER
	global HIGHEST_FREQ
	global CUR_OPT_POINT

	if CUR_OPT_POINT != None:
		speedup = False
		for app in APP_FREQ_VEC:
			if CUR_OPT_POINT.lat_vec[app] < lat_vec[app] and APP_FREQ_VEC[app] != HIGHEST_FREQ[APP_SERVER[app]]:
				speedup = True
				APP_FREQ_VEC[app] += FREQ_RESO[APP_SERVER[app]]

		if not speedup:
			# speedup all applications with spare room
			for app in APP_FREQ_VEC:
				if APP_FREQ_VEC[app] != HIGHEST_FREQ[APP_SERVER[app]]:
					APP_FREQ_VEC[app] += FREQ_RESO[APP_SERVER[app]]
	else:
		# speedup all dimensions
		for app in APP_FREQ_VEC:
			if APP_FREQ_VEC[app] != HIGHEST_FREQ[APP_SERVER[app]]:
				APP_FREQ_VEC[app] += FREQ_RESO[APP_SERVER[app]]

# application specific
def get_stats():
	# end2end
	global STATS_FILE
	global APP_SERVER
	global APP_FREQ_VEC
	global CUR_QPS
	global CUR_ROUND

	lat_vec = {}
	round_match = False
	with open('%s' %STATS_FILE, 'r') as f:
		lines = f.readlines()
		if len(lines) == 0:
			return None
		for line in lines:
			data = line.split(';')
			# print data
			# latency, nginx, memc_get, memc_set, memc_find, pure_nginx
			lat_vec['end2end'] 		= int(data[0].split(':')[1])
			lat_vec['nginx'] 		= int(data[1].split(':')[1])
			lat_vec['memcached'] 	= int(data[2].split(':')[1])
			sim_round				= int(data[4].split(':')[1])
			if sim_round == CUR_ROUND:
				CUR_QPS				= int(data[3].split(':')[1])
				print 'CUR_QPS = ', CUR_QPS
				print data
				get_qps_range()
				# CUR_ROUND += 1
				round_match = True
				break

		# lat_vec['memc_get'] 	= int(data[2])
		# lat_vec['memc_set'] 	= int(data[3])
		# lat_vec['memc_find'] 	= int(data[4])
		# lat_vec['pure_ngx'] 	= int(data[5])

	if not round_match:
		return None
	# show lat_vec
	for key in lat_vec:
		print key, ':', float(lat_vec[key]), ';',
	print ''
	return lat_vec

def pars_config():
	global CONFIG
	global HIGHEST_FREQ
	global APP_SERVER
	global SERVER_APP
	global APP_CORE_NUM
	global PORTS

	with open('%s' %CONFIG, 'r') as f:
		lines = f.readlines()
		for line in lines:
			(spec, words) = line.split(':')
			words = words.split()
			if spec == 'app':
				app 			 = words[0]
				server 			 = words[1]
				core_num 		 = int(words[2])
				APP_SERVER[app]  = server
				if app != 'jaeger':
					if server not in SERVER_APP:
						SERVER_APP[server] = []
					SERVER_APP[server].append(app)
					APP_FREQ_VEC[app] =  HIGHEST_FREQ[server]
					APP_CORE_NUM[app] = core_num

			elif spec == 'server':
				server  = words[0]
				assert server in IP_ADDR	
				PORTS[server] = int(words[1])

	print APP_SERVER
	print SERVER_APP
	print PORTS

# def connect():
# 	global IP_ADDR
# 	global SERVER_APP
# 	global PORTS
# 	global SOCKS
# 	for serv in SERVER_APP:
# 		sock = socket(AF_INET, SOCK_STREAM)
# 		sock.connect((IP_ADDR[serv], PORTS[serv]))
# 		SOCKS[serv] = sock
# 		print '%s connected' %serv

# def send_cmd():
# 	global APP_FREQ_VEC
# 	global SERVER_APP
# 	global SOCKS

# 	for serv in SERVER_APP:
# 		cmd = ''
# 		for app in SERVER_APP[serv]:
# 			if app == 'jaeger':
# 				continue
# 			cmd += app + ':' + str(APP_FREQ_VEC[app]) + ';'
# 		if cmd == '':
# 			continue
# 		cmd = cmd[:-1] + '\n'
# 		SOCKS[serv].sendall(cmd)

def send_cmd():
	global APP_FREQ_VEC
	global DEC_FILE
	global CUR_ROUND

	with open(DEC_FILE, 'w+') as f:
		cmd = "nginx: %u; memcached: %u; cur_round: %d" %(APP_FREQ_VEC['nginx'], APP_FREQ_VEC['memcached'], CUR_ROUND)
		CUR_ROUND += 1
		f.write(cmd)


def send_terminate():
	global SOCKS
	for serv in SOCKS:
		SOCKS[serv].sendall('terminate\n')

def show_freq_vec():
	global APP_FREQ_VEC
	string = ''
	for app in APP_FREQ_VEC:
		if app == 'jaeger':
			continue
		string += app + ':' + str(APP_FREQ_VEC[app]) + '\t'
	if string != '':
		string = string[:-1]
	return string

def help():
	print '1st: RUN_TIME (s)'
	print '2nd: INTERVAL (s)'
	print '3rd: target (ms)'
	print '4th: config file path (optional)'

# interval: monitor interval (s)
# target: target tail lat (ms)
def main():
	global HIGHEST_FREQ 
	global LOWEST_FREQ  
	global FREQ_RESO	 
	global RUN_TIME
	global INTERVAL
	global APP_FREQ_VEC
	global FREQ_CHECKPOINT
	global CONFIG
	
	global QOS_SLOT_STEP
	global OBSERVE_COUNTER
	global POINT_STATIC_CYCLE
	global POINT_STATIC_COUNTER

	global ONLINE_GRAPHS
	global CUR_OPT_POINT
	global VIOL_POINT
	global CUR_GRAPH_IDX
	global VIOL_GRAPH_IDX
	global SCHED_STATE

	global CUR_QPS
	global TOTAL_ROUND
	global CUR_ROUND
	global QPS_EXPERIENCE

	if sys.argv[1] == '-h':
		help()
		return

	RUN_TIME = int(sys.argv[1])
	INTERVAL = float(sys.argv[2])
	TOTAL_ROUND = int(RUN_TIME/INTERVAL)
	print 'TOTAL_ROUND = ', TOTAL_ROUND
	# target in millisecond, change into ns
	target = int(sys.argv[3]) * 1000000.0
	init_online_graphs(target)

	if len(sys.argv) >= 5:
		CONFIG = sys.argv[4]

	pars_config()
	# connect()
	start_time 			= time.time()
	prev_monitor_time 	= time.time()

	while True:
		# cur_time = time.time()
		# if cur_time - start_time >= RUN_TIME:
		# 	send_terminate()
		# 	break
		# if cur_time - prev_monitor_time < INTERVAL:
		# 	time.sleep(INTERVAL)
		# 	continue
		# else:
		# 	prev_monitor_time = cur_time

		if CUR_ROUND >= TOTAL_ROUND:
			break	# terminate

		print "\nAt time: %f" %((CUR_ROUND + 1) * INTERVAL)
		print 'CUR_ROUND = ', CUR_ROUND
		stats = None
		while True:
			stats = get_stats()
			if stats != None:
				break
			else:
				time.sleep(INTERVAL)

		# simulator output stats in ns
		end2end_lat = stats['end2end']
		print 'end2end_lat = ', float(end2end_lat)/1000000.0, ' ms'
		del stats['end2end']

		qos_viol = False
		explore = False
		if end2end_lat > target:
			print 'qos violated'
			qos_viol = True
		else:
			print 'qos met'
			new_point = GraphPoint(end2end_lat, stats)
			explore = insert_new_point(new_point)
			print 'new point explore = ', explore
			if explore:
				graph_idx = get_graph_idx(end2end_lat)
				explore   = probablistic_accpet(ONLINE_GRAPHS[graph_idx].probability)
				print 'new point final explore = ', explore

		if SCHED_STATE == 'NORMAL':
			print 'NORMAL'
			assert CUR_OPT_POINT != None
			print 'CUR_GRAPH_IDX changed = ', CUR_GRAPH_IDX
			print 'current_optimal_graph: [', QOS_SLOT_STEP*CUR_GRAPH_IDX/1000.0, ', ', QOS_SLOT_STEP*(CUR_GRAPH_IDX+1)/1000.0, ']'

			ONLINE_GRAPHS[CUR_GRAPH_IDX].viol_stats['run_time'] += 1
			if qos_viol:
				assert not explore
				SCHED_STATE    = 'RECOVER'
				if len(QPS_EXPERIENCE) == 1:
					VIOL_POINT     = CUR_OPT_POINT
					VIOL_GRAPH_IDX = CUR_GRAPH_IDX
					ONLINE_GRAPHS[VIOL_GRAPH_IDX].viol_stats['total_viol'] += 1
					ONLINE_GRAPHS[VIOL_GRAPH_IDX].delete(VIOL_POINT)
					adjust_graph_probability(VIOL_GRAPH_IDX, False)
				else:
					print 'no punishment for point since load changed'
					VIOL_POINT 	   = None
					VIOL_GRAPH_IDX = -1
				QPS_EXPERIENCE = []
				graph_found = get_optimal_graph(False)
				if graph_found:
					print 'CUR_GRAPH_IDX changed to ', CUR_GRAPH_IDX
					print 'current_optimal_graph: [', QOS_SLOT_STEP*CUR_GRAPH_IDX/1000.0, ', ', QOS_SLOT_STEP*(CUR_GRAPH_IDX+1)/1000.0, ']'
					ONLINE_GRAPHS[CUR_GRAPH_IDX].get_optimal_point(stats)
				else:
					CUR_GRAPH_IDX = -1
					CUR_OPT_POINT = None
				adjust_freq_viol(stats)
			else:
				if CUR_QPS not in QPS_EXPERIENCE:
					QPS_EXPERIENCE.append(CUR_QPS)
				if explore:
					CUR_OPT_POINT = None
					CUR_GRAPH_IDX = -1
					SCHED_STATE = 'EXPLORE'
					random_slowdown(end2end_lat)	# ORIGINAL_QPS is set in random_slowdown
				else:
					adjust_graph_probability(CUR_GRAPH_IDX, True)
					POINT_STATIC_COUNTER -= 1
					OBSERVE_COUNTER -= 1
					if OBSERVE_COUNTER <= 0:
						# give scheduler a chance to look for a better scheduling point
						if POINT_STATIC_COUNTER <= 0:
							POINT_STATIC_COUNTER = POINT_STATIC_CYCLE
							graph_found = get_optimal_graph(True)
							assert graph_found
							print 'CUR_GRAPH_IDX changed to ', CUR_GRAPH_IDX
							print 'current_optimal_graph: [', QOS_SLOT_STEP*CUR_GRAPH_IDX/1000.0, ', ', QOS_SLOT_STEP*(CUR_GRAPH_IDX+1)/1000.0, ']'
							ONLINE_GRAPHS[CUR_GRAPH_IDX].get_optimal_point(stats)
						adjust_freq(stats)
					else:
						adjust_freq_observe(stats)

		elif SCHED_STATE == 'EXPLORE':
			# while in explore stage the optimal point should not be decided
			print 'EXPLORE'
			if qos_viol:
				SCHED_STATE = 'RECOVER'
				if len(QPS_EXPERIENCE) == 1:
					VIOL_GRAPH_IDX = OBSERVE_GRAPH_IDX
				else:
					print 'no punishment for point since load changed'
					VIOL_GRAPH_IDX = -1
				graph_found = get_optimal_graph(False)
				if graph_found:
					print 'CUR_GRAPH_IDX = ', CUR_GRAPH_IDX
					print 'current_optimal_graph: [', QOS_SLOT_STEP*CUR_GRAPH_IDX/1000.0, ', ', QOS_SLOT_STEP*(CUR_GRAPH_IDX+1)/1000.0, ']'
					ONLINE_GRAPHS[CUR_GRAPH_IDX].get_optimal_point(stats)
				QPS_EXPERIENCE = []
			else:
				if CUR_QPS not in QPS_EXPERIENCE:
					QPS_EXPERIENCE.append(CUR_QPS)
				OBSERVE_COUNTER -= 1
				if OBSERVE_COUNTER <= 0:
					random_slowdown(end2end_lat)
				
		elif SCHED_STATE == 'RECOVER':
			print 'RECOVER'
			if qos_viol:
				# if VIOL_GRAPH_IDX >= 0:
				# 	adjust_graph_probability(VIOL_GRAPH_IDX, False)
				# 	ONLINE_GRAPHS[VIOL_GRAPH_IDX].viol_stats['total_viol'] += 1
				# 	ONLINE_GRAPHS[VIOL_GRAPH_IDX].viol_stats['run_time'] += 1
				adjust_freq_viol(stats)
			else:
				SCHED_STATE = 'NORMAL'
				QPS_EXPERIENCE = [CUR_QPS]
				# there is at least one graph inserted
				graph_found = get_optimal_graph(stats)
				# at least one point (current one) is inserted
				assert graph_found
				print 'CUR_GRAPH_IDX = ', CUR_GRAPH_IDX
				print 'current_optimal_graph: [', QOS_SLOT_STEP*CUR_GRAPH_IDX/1000.0, ', ', QOS_SLOT_STEP*(CUR_GRAPH_IDX+1)/1000.0, ']'
				ONLINE_GRAPHS[CUR_GRAPH_IDX].get_optimal_point(stats)
				adjust_freq(stats)
				
		else:
			print 'Undefined State: ', SCHED_STATE
			exit(0)

		send_cmd()
		print 'new freq_vec'
		print show_freq_vec()


if __name__ == '__main__':
	main()
