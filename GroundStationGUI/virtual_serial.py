#!/usr/bin/env python
import subprocess, serial, time, sys

device_port = "./device_serial"
client_port = "./client_serial"

link_name = "/dev/ttyS50"

socat_cmd = ["/usr/bin/socat", "-d", "-d", "PTY,link=%s,raw,echo=0" % device_port, "PTY,link=%s,raw,echo=0" % client_port]
socat = subprocess.Popen(socat_cmd, stdout=sys.stdout, stderr=sys.stderr)
time.sleep(1)
ser = serial.Serial(device_port, 9600, rtscts=True, dsrdtr=True)

ln_cmd = "sudo ln -s $(readlink %s) %s" % (client_port, link_name)
subprocess.call(ln_cmd, shell=True)

test_file = open("GPRMC_Locked_2Mile_ATU_Tracking_data_noNewline.txt", "r")
test_data = test_file.read().split("@")
test_file.close()

raw_input("Press enter to begin sending data")

try:
	for datum in test_data:
		ser.write("@" + datum)
		if ser.in_waiting > 0:
			c = ser.read(1)
			print "Received input: " + str(c)
except KeyboardInterrupt:
	pass

raw_input("Press enter to finish")

rm_cmd = "sudo rm %s" % link_name
subprocess.call(rm_cmd, shell=True)
