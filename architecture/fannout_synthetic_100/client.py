import sys
import os
import json
import make_arch as march 


def main():
	end_seconds = [60]
	kqps = [8]

	client = march.make_client(epoch_end_seconds = end_seconds, epoch_kqps = kqps, 
		monitor_interval_sec = 1)

	with open("./json/client.json", "w+") as f:
		json.dump(client, f, indent=2)

if __name__ == "__main__":
	main()

