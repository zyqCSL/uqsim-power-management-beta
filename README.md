# msimulator
Run power management experiment:
1) In root directory, do ./microsim ./architecture/2tier_ngx_8_memc_2/json 300 100 expo 10
   The parameters are application architecture directory, number of http connections from workload generator to application frontend, network delay, request inter-arrival distribution type, and input load kqps number

2) In sched_test directory, do python sched.py 10 0.1 500 ./sched_config_ath8.txt
   The paremters are experiment time, scheduling interval, qos target and application deployment configuration (should be consistent with json).

Then the simulator and the python script will execute in lockstep.
