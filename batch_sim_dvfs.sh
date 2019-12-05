#!/bin/bash
ngx_freq=(2600 2400 2200 2000 1800 1600 1400 1200)
memc_freq=(2600 2400 2200 2000 1800 1600 1400 1200)
memc_freq_1=(2600 2400 2200 2000)
memc_freq_2=(1800 1600 1400 1200)

for i in {0..7}
# for i in {0..0}
do 
	for j in {0..3}
		do
			ngx=${ngx_freq[$i]}
			memc_1=${memc_freq_1[$j]}
			memc_2=${memc_freq_2[$j]}
			dir_1='/home/yz2297/ispass_2019/dvfs_valid/ngx_'$ngx'_memc_'$memc_1'/'
			dir_2='/home/yz2297/ispass_2019/dvfs_valid/ngx_'$ngx'_memc_'$memc_2'/'
			echo $dir_1
			echo $dir_2
			mkdir -p $dir_1
			mkdir -p $dir_2
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 10  $ngx_freq $memc_freq_1 > $dir_1'10k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 20  $ngx_freq $memc_freq_1 > $dir_1'20k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 30  $ngx_freq $memc_freq_1 > $dir_1'30k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 40  $ngx_freq $memc_freq_1 > $dir_1'40k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 50  $ngx_freq $memc_freq_1 > $dir_1'50k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 60  $ngx_freq $memc_freq_1 > $dir_1'60k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 70  $ngx_freq $memc_freq_1 > $dir_1'70k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 80  $ngx_freq $memc_freq_1 > $dir_1'80k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 90  $ngx_freq $memc_freq_1 > $dir_1'90k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 100 $ngx_freq $memc_freq_1 > $dir_1'100k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 110 $ngx_freq $memc_freq_1 > $dir_1'110k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 120 $ngx_freq $memc_freq_1 > $dir_1'120k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 130 $ngx_freq $memc_freq_1 > $dir_1'130k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 140 $ngx_freq $memc_freq_1 > $dir_1'140k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 150 $ngx_freq $memc_freq_1 > $dir_1'150k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 160 $ngx_freq $memc_freq_1 > $dir_1'160k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 170 $ngx_freq $memc_freq_1 > $dir_1'170k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 180 $ngx_freq $memc_freq_1 > $dir_1'180k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 190 $ngx_freq $memc_freq_1 > $dir_1'190k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 200 $ngx_freq $memc_freq_1 > $dir_1'200k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 10  $ngx_freq $memc_freq_2 > $dir_2'10k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 20  $ngx_freq $memc_freq_2 > $dir_2'20k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 30  $ngx_freq $memc_freq_2 > $dir_2'30k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 40  $ngx_freq $memc_freq_2 > $dir_2'40k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 50  $ngx_freq $memc_freq_2 > $dir_2'50k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 60  $ngx_freq $memc_freq_2 > $dir_2'60k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 70  $ngx_freq $memc_freq_2 > $dir_2'70k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 80  $ngx_freq $memc_freq_2 > $dir_2'80k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 90  $ngx_freq $memc_freq_2 > $dir_2'90k.txt'  & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 100 $ngx_freq $memc_freq_2 > $dir_2'100k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 110 $ngx_freq $memc_freq_2 > $dir_2'110k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 120 $ngx_freq $memc_freq_2 > $dir_2'120k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 130 $ngx_freq $memc_freq_2 > $dir_2'130k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 140 $ngx_freq $memc_freq_2 > $dir_2'140k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 150 $ngx_freq $memc_freq_2 > $dir_2'150k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 160 $ngx_freq $memc_freq_2 > $dir_2'160k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 170 $ngx_freq $memc_freq_2 > $dir_2'170k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 180 $ngx_freq $memc_freq_2 > $dir_2'180k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 190 $ngx_freq $memc_freq_2 > $dir_2'190k.txt' & \
			timeout 120m ./microsim ./architecture/2tier/json/ 320 65000 expo 200 $ngx_freq $memc_freq_2 > $dir_2'200k.txt'
			echo done ngx $ngx memc_1 $memc_1 memc_2 $memc_2
		done
done
echo done $filename all
