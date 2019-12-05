import sys
import os
import json
import make_arch as march 


def main():
	end_seconds = [5.0, 10.0, 15.0, 20.0]
	kqps = [50.0, 100.0, 140.0, 60.0]
	tail_lat_ms = 20.0
	slack = 0.2
	interval_s = 1.0

	client = march.make_client(epoch_end_seconds = end_seconds, epoch_kqps = kqps, target_tail_lat_ms = tail_lat_ms, 
		monitor_interval_seconds = interval_s)

	with open("./json/client.json", "w+") as f:
		json.dump(client, f, indent=2)

if __name__ == "__main__":
	main()

