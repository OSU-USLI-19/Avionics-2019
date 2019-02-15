#!/usr/bin/env python
import sys
import argparse
import serial
import re
import utm
import math
import numpy

import matplotlib
matplotlib.use("Qt5Agg")
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

from serial.tools.list_ports import comports

from PyQt5.QtWidgets import (QApplication, QLabel, QPushButton, QVBoxLayout, QWidget, QInputDialog, QMessageBox, QListWidget, QSizePolicy, QTextBrowser)
from PyQt5.QtCore import pyqtSlot, pyqtSignal, Qt, QThread, QObject, QTimer
from PyQt5.QtGui import QFontDatabase, QTextOption, QTextCursor

class GPSDatum:
	def __init__(self, h, m, s, latitude, longitude):
		self.h = h
		self.m = m
		self.s = s
		self.latitude = latitude
		self.longitude = longitude

	def cartesian(self):
		x, y, _, _ = utm.from_latlon(self.latitude, self.longitude)
		return x, y

	def seconds(self):
		return (360 * self.h) + (60 * self.m) + self.s

	def string(self):
		second_str = "{0:.3f}".format(self.s).zfill(6)
		output_str = "{0:02d}:{1:02d}:{2} -> {3:.4f}, {4:.4f}"
		return output_str.format(self.h, self.m, second_str, self.latitude, self.longitude)

class InputWorker(QObject):
	new_datum = pyqtSignal(object)

	def __init__(self, input_name, args):
		QObject.__init__(self)

		self.input_name = input_name
		self.args = args

		self.input_stream = None
		self.quitting = False

	def parse_datum(self, cur_line):
		# Skip data sent while ATU is not locked.
		if cur_line.endswith("0000.0000,N,00000.0000,E,000.0"):
			return None

		# Extract the time and break into hours, minutes, seconds.
		time_pattern = re.compile(r"([0-9]{2})([0-9]{2})([0-9]{2}\.[0-9]{3}),A")
		match = re.search(time_pattern, cur_line)
		if match is not None:
			hour   = int(match.group(1))
			minute = int(match.group(2))
			second = float(match.group(3))

		# If the data does not match the expected format, skip it.
		else:
			return None
		
		# Extract the latitude and convert to decimal degree form.
		lat_pattern = re.compile(r"([0-9]{2})([0-9]{2}\.[0-9]+),(N|S)")
		match = re.search(lat_pattern, cur_line)
		if match is not None:
			deg = float(match.group(1))
			min = float(match.group(2))
			lat = deg + (min / 60)
			if str(match.group(3)) == "S":
				lat = -lat

		# If the data does not match the expected format, skip it.
		else:
			return None

		# Extract the longitude and convert to decimal degree form.
		lon_pattern = re.compile(r"([0-9]{3})([0-9]{2}\.[0-9]+),(E|W)")
		match = re.search(lon_pattern, cur_line)
		if match is not None:
			deg = float(match.group(1))
			min = float(match.group(2))
			lon = deg + (min / 60)
			if str(match.group(3)) == "W":
				lon = -lon

		# If the data does not match the expected format, skip it.
		else:
			return None

		return GPSDatum(hour, minute, second, lat, lon)

	@pyqtSlot()
	def run(self):
		# Get the file or serial port to read from.
		if self.args.input:
			# Open a file to use as input.
			self.input_stream = open(self.input_name, "r")
		else:
			# Open serial port at port_name at 9600 baud and with 30000 second timeout.
			# TODO: More serial testing
			self.input_stream = serial.Serial(self.input_name, 9600, timeout=30000, rtscts=True, dsrdtr=True)

		# Determine the name of the output file.
		if self.args.output:
			output_file = self.args.output
		else:
			output_file = "output.txt"

		# Opens a file for writing GPS data to.
		output = open(output_file, "w")

		while not self.quitting:
			line = ""

			# Read characters until we find the "@" delimiter.
			if self.args.input:
				c = self.input_stream.read(1)
				while c and c != "@" and not self.quitting:
					c = self.input_stream.read(1)

				# Break if quitting.
				if self.quitting:
					break
				
				# Break at end of file.
				if not c:
					break

				# Read a line of GPS data.
				line = self.input_stream.read(50)
			else:
				remaining_chars = 51
				loop_counter = 0
				while True:
					if self.quitting:
						self.input_stream.close()
						output.close()
						return

					if self.input_stream.in_waiting > 0:
						c = self.input_stream.read(1)
						# print "DO READ " + str(remaining_chars) + "     " + c
						if remaining_chars <= 50:
							line += c
							remaining_chars -= 1
							if remaining_chars == 0:
								break
						elif c == "@":
							remaining_chars -= 1

					if loop_counter % 50 == 0:
						app.processEvents()
						loop_counter = 0
					else:
						loop_counter += 1

			try:
				decoded = line.decode("utf-8")
			except UnicodeDecodeError:
				print("ERROR: Could not decode line " + line);
				continue

			# Parse the line of data.
			datum = self.parse_datum(line)
			if not datum:
				continue

			# Write time, latitude, and longitude to stdout and output file.
			datum_str = datum.string()
			output.write(datum_str + "\n")

			# Perform subclass-specific actions.
			self.new_datum.emit(datum)

			# Allow the GUI to process pending events.
			app.processEvents()

		# Close the input stream.
		self.input_stream.close()
	
		# Close the output file.
		output.close()

	@pyqtSlot()
	def send_command(self):
		if not self.args.input and self.input_stream:
			self.input_stream.write("0".encode("utf-8")) #TODO: Send whatever actually needs to be sent here

	@pyqtSlot()
	def finish(self):
		self.quitting = True

class DataDisplay(QWidget):
	def __init__(self, args):
		QWidget.__init__(self)

		self.args = args

	def new_datum(self, datum):
		raise NotImplementedError()
	
class GraphDisplay(DataDisplay):
	def __init__(self, args):
		DataDisplay.__init__(self, args)

		self.lon_data = [0]
		self.lat_data = [0]

		self.origin = None

		self.fig = Figure()
		self.graph = FigureCanvas(self.fig)
		self.graph.setParent(self)
		self.graph.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
		self.graph.updateGeometry()

		self.axes = self.fig.add_subplot(111)
		self.axes.set_title("Launch Vehicle Drift")
		self.axes.set_xlabel("East (m)")
		self.axes.set_ylabel("North (m)")
		self.axes.grid(color="k", linestyle="-", linewidth=0.5)

		self.line, = self.axes.plot(self.lon_data, self.lat_data)

		self.graph.draw()

		self.text = None
		self.props = dict(boxstyle="square", facecolor="aliceblue", alpha=0.5)

		self.timer = QTimer(self)
		self.timer.timeout.connect(self.update_graph)
		self.timer.start(10)

		self.layout = QVBoxLayout()
		self.layout.addWidget(self.graph)
		self.setLayout(self.layout)

	def save_fig(self, filename):
		self.fig.savefig(filename)

	def new_datum(self, datum):
		x, y = datum.cartesian()

		if not self.origin:
			self.origin = (x, y)

		else:
			x = x - self.origin[0]
			y = y - self.origin[1]

			self.lon_data.append(x)
			self.lat_data.append(y)

	@pyqtSlot()
	def update_graph(self):
		self.line.set_xdata(self.lon_data)
		self.line.set_ydata(self.lat_data)
		
		self.axes.draw_artist(self.axes.patch)
		self.axes.draw_artist(self.line)
		self.axes.relim()
		self.axes.autoscale_view()
		self.graph.draw()
		self.graph.flush_events()

		x = self.lon_data[-1]
		y = self.lat_data[-1]
		dist = math.sqrt(x**2 + y**2)
		angle = math.degrees(math.atan2(y, x))
		if self.text:
			self.text.remove()
		data_str = "Distance: {0:.2f} m\nAngle: {1:.2f}$^\circ$".format(dist, angle)
		self.text = self.axes.text(0.05, 0.05, data_str, fontsize=12, transform=self.axes.transAxes, bbox=self.props)

class TextDisplay(DataDisplay):
	def __init__(self, args):
		DataDisplay.__init__(self, args)

		self.layout = QVBoxLayout() 
		self.text = QTextBrowser()
		self.text.setWordWrapMode(QTextOption.NoWrap)
		self.text.setFont(QFontDatabase.systemFont(QFontDatabase.FixedFont))
		self.text.insertPlainText("Application started!")
		self.layout.addWidget(self.text)
		self.setLayout(self.layout)

	def new_datum(self, datum):
		self.text.moveCursor(QTextCursor.End)
		self.text.insertPlainText("\n" + datum.string())
		v_scroll = self.text.verticalScrollBar()
		v_scroll.setValue(v_scroll.maximum())

class MainWindow(QWidget):
	finish_thread = pyqtSignal()

	def __init__(self, app, args):
		QWidget.__init__(self)

		self.SPEED_THRESHOLD = .1 # meters / second FIXME: Use the correct value here
		self.STATIONARY_MIN_TIME = 5 # seconds FIXME: Use the correct value here

		self.prev_coords = None
		self.prev_time = None
		self.stationary_start_time = None

		# Create the display, either a text or graphical display depending on flags.
		if args.textonly:
			self.display = TextDisplay(args)
		else:
			self.display = GraphDisplay(args)

		self.button = QPushButton("Eject")
		self.button.setEnabled(False)
		
		# Create layout and add widgets
		self.layout = QVBoxLayout()
		self.layout.addWidget(self.display)
		self.layout.addWidget(self.button)
		self.setLayout(self.layout)

		input_name = args.input
		# If not in file-input mode, read from a serial port.
		if not input_name:
			# Get a list of available serial ports.
			ports = [p.device for p in serial.tools.list_ports.comports()]
			# If no ports were found, show an error and quit
			if len(ports) == 0:
				error_box = QMessageBox(QMessageBox.Critical, "Error", "Error: No available serial ports found.  Connect a device or start in file-input mode (with -i)", QMessageBox.Ok, self)
				error_box.buttonClicked.connect(QApplication.quit)
				error_box.exec_()
			# If only one port was found, use it
			elif len(ports) == 1:
				input_name = ports[0]
			#If multiple ports were found, allow the user to select one
			else:
				input_name = self.select_serial_port(ports)

		# Create input-reading thread and worker.
		self.input_thread = QThread()
		self.input_worker = InputWorker(input_name, args)

		# Connect button click signal to action function.
		self.button.clicked.connect(self.input_worker.send_command)

		# Connect thread start signal to worker run slot.
		self.input_thread.started.connect(self.input_worker.run)

		# Connect quitting signal to worker quit slot.
		self.finish_thread.connect(self.input_worker.finish)

		# Connect signal providing a newly recieved datum to the datum display and processing functions.
		self.input_worker.new_datum.connect(self.display.new_datum)
		self.input_worker.new_datum.connect(self.new_datum)

		# Move the worker to the thread.
		self.input_worker.moveToThread(self.input_thread)

	def select_serial_port(self, ports):
		ok = False
		while not ok:
			port, ok = QInputDialog.getItem(self, "Select Serial Port", "Select a serial port to use:", ports, editable=False)
			if not ok:
				message_box = QMessageBox(QMessageBox.Critical, "Error", "Error: Must select a serial port or start in file-input mode (with -i)", QMessageBox.Ok, self)
				close_button = message_box.addButton("Quit", QMessageBox.DestructiveRole)
				close_button.clicked.connect(QApplication.quit)
				message_box.exec_()
		return port

	def new_datum(self, datum):
		# FIXME: Is this the check we want to be doing?

		# Get the new datum's coordinates and timestamp
		coords = numpy.array(datum.cartesian())
		time = datum.seconds()

		# Only check for stationariness after first datum
		if self.prev_coords is not None:
			# Compute position and time differences to determine speed
			dist = numpy.linalg.norm(coords - self.prev_coords) # meters
			time_diff = time - self.prev_time # seconds

			# If the speed is less than the threshold, start checking if enough time has elapsed
			if (dist / time_diff) < self.SPEED_THRESHOLD:
				if self.stationary_start_time:
					# If enough time has elapsed with a low enough speed, consider the transmitter to be stationary and allow the button to be pressed
					if (time - self.stationary_start_time) >= self.STATIONARY_MIN_TIME:
						self.button.setEnabled(True)
				else:
					# Set the initial stationary timestamp on the first loop within the speed threshold
					self.stationary_start_time = time
			else:
				# If the speed is greater than the threshold, disable the button and reset the initial stationary timestamp
				self.button.setEnabled(False)
				self.stationary_start_time = None
		
		# Overwrite the previous data with the newest
		self.prev_coords = coords
		self.prev_time = time

	def closeEvent(self, event):
		# Tell the input thread to quit and wait for it to finish before closing.
		self.finish_thread.emit()
		self.input_thread.quit()
		self.input_thread.wait()

		if isinstance(self.display, GraphDisplay):
			# Prompt user to save the figure.
			filename, ok = QInputDialog.getText(self, "Save Graph?", "Enter a filename to save the graph under, or press Cancel to quit without saving:")
			if ok:
				self.display.save_fig(filename)	

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="USLI 2019 ground station")
	parser.add_argument("-o", "--output", help="specify a file to write output to")
	parser.add_argument("-i", "--input", help="specify a file to read input from in place of a serial port")
	parser.add_argument("-t", "--textonly", action="store_true", help="display incoming data as text rather than graphically")
	args = parser.parse_args()	

	app = QApplication(["USLI 2019 Ground Station"])

	main_window = MainWindow(app, args)
	main_window.showMaximized()

	main_window.input_thread.start()
	
	sys.exit(app.exec_())
