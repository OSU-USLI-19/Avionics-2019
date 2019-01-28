import time
import threading
import sys
import glob
import serial
import re
import utm
import math

from Tkinter import *
import tkSimpleDialog
from random import randint
import argparse
from ScrolledText import ScrolledText
 
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

class GPSDatum:
	def __init__(self, h, m, s, latitude, longitude):
		self.h = h
		self.m = m
		self.s = s
		self.latitude = latitude
		self.longitude = longitude

	def string(self):
		second_str = "{0:.3f}".format(self.s).zfill(6)
		output_str = "{0:02d}:{1:02d}:{2} -> {3:.4f}, {4:.4f}"
		return output_str.format(self.h, self.m, second_str, self.latitude, self.longitude)

class DataDisplay(threading.Thread):
	def __init__(self, root):
		threading.Thread.__init__(self)
		
		self.root = root
	
		self.closing = threading.Event()

	def parse_datum(self, filestream):	
		# Read a line of GPS data.
		cur_line = filestream.read(50)

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

		return GPSDatum(hour, minute, second, lat, lon);

	def on_datum(self, datum):
		raise NotImplementedError()
	
	def run(self):	
		# Opens a file named output.txt for writing GPS data to.
		output = open("output.txt", "w")

		# Name of file to read data from.
		input_file = "GPRMC_Locked_2Mile_ATU_Tracking_data_noNewline.txt"
		
		with open(input_file, "r") as filestream:
			while not self.closing.is_set():
				# Read characters until we find the "@" delimiter.
				c = filestream.read(1)
				while c and c != "@":
					c = filestream.read(1)

				# Break at end of file.
				if not c:
					break

				# Parse a line of GPS data.
				datum = self.parse_datum(filestream)
				if not datum:
					continue
	
				# Write time, latitude, and longitude to stdout and output file.
				datum_str = datum.string()
				output.write(datum_str + "\n")
				print(datum_str)

				# Perform subclass-specific actions.
				self.on_datum(datum)
	
		# Close the output file.
		output.close()	
	
		if (self.closing.is_set()):
			# Tell the main window to quit.
			self.root.event_generate("<<QUIT>>", when="tail")

class GraphDisplay(DataDisplay):
	def __init__(self, root):
		DataDisplay.__init__(self, root)

		self.fig = Figure()
		self.graph = FigureCanvasTkAgg(self.fig, master=self.root)
		self.graph.get_tk_widget().pack(side="top",fill='both',expand=True)

		self.ax = self.fig.add_subplot(111)
		self.ax.set_xlabel("X axis")
		self.ax.set_ylabel("Y axis")
		self.ax.grid()

		self.ydata = [0]
		self.xdata = [0]

		self.text = None
		self.line, = self.ax.plot(self.xdata, self.ydata);
		self.graph.draw()

		# Set labels and create grid.
		self.ax.set_title("Launch Vehicle Drift")
		self.ax.set_xlabel("East (m)")
		self.ax.set_ylabel("North (m)")
		self.ax.grid(color="k", linestyle="-", linewidth=0.5)

		# Defines paramaters for distance/angle text box.
		self.props = dict(boxstyle="square", facecolor="aliceblue", alpha=0.5)

		# The point to use as the origin in the graph.
		self.origin = None

	def save_fig(self, filename):
		self.fig.savefig(filename)	

	def on_datum(self, datum):
		# Convert lat/lon into UTM (standardized 2D cartesian projection).
		x, y, _, _ = utm.from_latlon(datum.latitude, datum.longitude)

		# Set first point as origin (0,0).
		if not self.origin:
			self.origin = (x, y)

		# All other points are relative to this origin.
		else:
			x = x - self.origin[0]
			y = y - self.origin[1]

			# Add new data point.
			self.xdata.append(x)
			self.ydata.append(y)
			self.line.set_xdata(self.xdata)
			self.line.set_ydata(self.ydata)

			# Redraw plot and adjust axes.
			self.ax.draw_artist(self.ax.patch)
			self.ax.draw_artist(self.line)
			self.ax.relim()	
			self.ax.autoscale_view()
			self.graph.draw()
			self.graph.flush_events()

			# Compute and print absolute distance and angle from origin.
			dist  = math.sqrt(x**2 + y**2)
			angle = math.degrees(math.atan2(y,x))
			if self.text is not None:
				self.text.remove()
			data_str = "Distance: {0:.2f} m\nAngle: {1:.2f}$^\circ$".format(dist, angle)
			self.text = self.ax.text(0.05, 0.05, data_str, fontsize=12, transform=self.ax.transAxes, bbox=self.props)

class TextDisplay(DataDisplay):
	def __init__(self, root):
		DataDisplay.__init__(self, root)

		frame = Frame(root)
		scrollframe = Frame(frame)

		self.text = Text(scrollframe)
		scroll = Scrollbar(scrollframe, command=self.text.yview, width=24)
		self.text["yscrollcommand"] = scroll.set

		jump_button = Button(frame, text="Jump to end", command=self.jump_to_end)
		
		self.text.pack(side="left", fill="both", expand=True)
		scroll.pack(side="right", fill="y")

		scrollframe.pack(side="top", fill="both", expand=True)
		jump_button.pack(side="bottom", fill="x")
		frame.pack(side="top", fill="both", expand=True)

	def jump_to_end(self):
		self.text.see("end")

	def on_datum(self, datum):
		at_end = self.text.yview()[1] == 1.0
		self.text.insert("end","\n" + datum.string())
		if (at_end):
			self.text.see("end")

def app():
	parser = argparse.ArgumentParser(description="USLI 2019 ground station")
	parser.add_argument("-o", "--output", help="specify a file to write output to")
	parser.add_argument("-i", "--input", help="specify a file to read input from in place of a serial port")
	parser.add_argument("-t", "--textonly", action="store_true", help="display incoming data as text rather than graphically")
	args = parser.parse_args()

	root = Tk()
	root.config(background='white')
	root.title("Ground Station")

	w = root.winfo_screenwidth()
	h = root.winfo_screenheight()
	root.geometry("%dx%d+0+0" % (w, h))

	if args.textonly:
		display = TextDisplay(root)
	else:
		display = GraphDisplay(root)
 
	def do_thing():
		print "Did the thing"
 
	b = Button(root, text="Do The Thing", command=do_thing, bg="red", fg="white")
	b.pack()

	display.start()

	def actually_close(*args):	
		if isinstance(display, GraphDisplay):
			# Prompt user to save the figure.
			file_name = tkSimpleDialog.askstring("Save graph as:", "Enter a filename to save the graph under:")
			if not file_name == None:
				display.save_fig(file_name)

		# Close the window.
		root.quit()
		root.destroy()
	
	def handle_close():
		# If the display is still active, let it finish its iteration before quitting.
		if (display.is_alive()):
			display.closing.set()
		else:
			actually_close()

	root.protocol("WM_DELETE_WINDOW", handle_close)
	root.bind("<<QUIT>>", actually_close)
	
	root.mainloop()
 
if __name__ == '__main__':
	app()
