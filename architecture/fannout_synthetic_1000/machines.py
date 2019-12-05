import sys
import os
import json
import make_arch as march 

NUM_QUE = 20

FANOUT = 1000

def main():
	global NUM_QUE
	global FANOUT
	core_aff = []
	for i in range(0, NUM_QUE):
		aff = march.make_Simp_core_aff(i, [i])
		core_aff.append(aff)
	core_list = range(0, NUM_QUE)

	machines = []
	sched = march.make_service_sched("LinuxNetStack", [NUM_QUE, core_list], core_aff)

	# need 2x machines + 1 to hold both ngx, memcached pairs and a load balancer
	for i in range(0, FANOUT + 1):
		mach_name = "machine_" + str(i)
		mach = march.make_machine(mid = i, name = mach_name, cores = 40, netSched = sched)
		machines.append(mach)

	with open("./json/machines.json", "w+") as f:
		json.dump(machines, f, indent=2)

if __name__ == "__main__":
	main()

