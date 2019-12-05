import sys
import os
import json
import make_arch as march 

NUM_QUE = 20

def main():
	global NUM_QUE
	core_aff = []
	for i in range(0, NUM_QUE):
		aff = march.make_Simp_core_aff(i, [i])
		core_aff.append(aff)
	core_list = range(0, NUM_QUE)
	sched = march.make_service_sched("LinuxNetStack", [NUM_QUE, core_list], core_aff)
	m0 = march.make_machine(mid = 0, name = "machine_0", cores = 100, netSched = sched)
	m1 = march.make_machine(mid = 1, name = "machine_1", cores = 100, netSched = sched)
	m2 = march.make_machine(mid = 2, name = "machine_2", cores = 100, netSched = sched)
	m3 = march.make_machine(mid = 3, name = "machine_3", cores = 100, netSched = sched)

	machines = [m0, m1, m2, m3]

	with open("./json/machines.json", "w+") as f:
		json.dump(machines, f, indent=2)

if __name__ == "__main__":
	main()

