#!/bin/bash
numConns=$1
netLat=65
time=$(date)
echo $time

# tmp_dir='/filer-01/qsim/logs/'$time'_'$app$num
tmp_dir='/home/yz2297/qsim/logs/2tier/validation/rapl/8p_ngx_2.2GHz_4t_memc_1.6Ghz_20irq_conn_'$numConns
mkdir -p $tmp_dir


# kqps=(50 100 150 200 250 300 350 400 450 500 550 600 650 700 750 800 850 900 950 1000) # 20
# kqps=(50 100 150 200 250 300 350 400 450 500 550 600 650 700 750 800 850 900 950 1000 1050 1100 1150 1200) # 24
# lat=(20000.0 10000.0 6666.666666666667 5000.0 4000.0 3333.3333333333335 2857.1428571428573 2500.0 2222.222222222222 2000.0 1818.1818181818182 1666.6666666666667 1538.4615384615386 1428.5714285714287 1333.3333333333333 1250.0 1176.4705882352941 1111.111111111111 1052.6315789473683 1000.0)
# kqps=(100 200 300 400 500 600 700 800 900 1000 1100 1200 1300 1400 1500 1600 1700 1800 1900 2000 2100) #21
# kqps=(50 100 150 200 250 300 350 400) #8
# kqps=(50 100 150 200 250 300 350 400 450 500) #8
kqps=(50 100 150 200 250 300 350 400 450 500 550 600 650 700 750) #15
kqps=(20 40 60 80 100 120 140 160 180 200) #10
# kqps=(200)
# kqps=(10 20 30 40 50 60 70 80) #8
# kqps=(25 50 75 100 125 150 175 200) #8
# kqps=(100 200 300 400 500 600 700 800 900 1000 1100) #11
# kqps=(100 200 300 400 500 600 700 800 900 1000 1100 1200 1300 1400 1500 1600) #16

for i in {0..9}
# for i in {0..0}
do 
	kiloqps=${kqps[$i]}
	# echo $kiloqps
	qps=$(($kiloqps*1000))
	# simulate 10s
	# totalJobs=$(($kiloqps*1000*30))
	totalJobs=1000000
	# echo $qps
	filename=$tmp_dir'/''kqps_'${kqps[$i]}.out
	touch $filename
	./microsim ./architecture/2tier_2/ $totalJobs $numConns $netLat expo $qps > $filename 
	# ./test_memcached $numConns ${kqps[$i]} $totalJobs $numThreads $numCores debug-off
	echo done ${kqps[$i]}
done
echo done $filename all
