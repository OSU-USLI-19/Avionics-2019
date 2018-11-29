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
 
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
 
class Plotter(threading.Thread):
	def __init__(self, root, graph, ax):
		threading.Thread.__init__(self)

		self.root = root
		self.graph = graph
		self.ax = ax

		self.closing = threading.Event()
	
	def run(self):
		ydata = [0]
		xdata = [0]

		text = None
		line, = self.ax.plot(xdata, ydata);
		self.graph.draw()

		# Set labels and create grid.
		self.ax.set_title("Launch Vehicle Drift")
		self.ax.set_xlabel("East (m)")
		self.ax.set_ylabel("North (m)")
		self.ax.grid(color="k", linestyle="-", linewidth=0.5)

		# Regex for extracting time, latitude, and longitude.
		time_pattern = re.compile(r"([0-9]{2})([0-9]{2})([0-9]{2}\.[0-9]{3}),A")
		lat_pattern  = re.compile(r"([0-9]{2})([0-9]{2}\.[0-9]+),(N|S)")
		lon_pattern  = re.compile(r"([0-9]{3})([0-9]{2}\.[0-9]+),(E|W)")

		# Defines paramaters for distance/angle text box.
		props = dict(boxstyle="square", facecolor="aliceblue", alpha=0.5)

		# Flag for whether or not the origin has been read.
		read_origin = False

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

				# Read a line of GPS data.
				cur_line = filestream.read(50)

				# Skip data sent while ATU is not locked.
				if cur_line.endswith("0000.0000,N,00000.0000,E,000.0"):
					continue

				# Extract the time and break into hours, minutes, seconds.
				match = re.search(time_pattern, cur_line)
				if match is not None:
					hour   = int(match.group(1))
					minute = int(match.group(2))
					second = float(match.group(3))

				# If the data does not match the expected format, skip it.
				else:
					continue

				# Extract the latitude and convert to decimal degree form.
				match = re.search(lat_pattern, cur_line)
				if match is not None:
					deg = float(match.group(1))
					min = float(match.group(2))
					lat = deg + (min / 60)
					if str(match.group(3)) == "S":
						lat = -lat

				# If the data does not match the expected format, skip it.
				else:
					continue

				# Extract the longitude and convert to decimal degree form.
				match = re.search(lon_pattern, cur_line)
				if match is not None:
					deg = float(match.group(1))
					min = float(match.group(2))
					lon = deg + (min / 60)
					if str(match.group(3)) == "W":
						lon = -lon

				# If the data does not match the expected format, skip it.
				else:
					continue

				# Write time, latitude, and longitude to stdout and output file.
				second_str = "{0:.3f}".format(second).zfill(6)
				output_str = "{0:02d}:{1:02d}:{2} -> {3:.4f}, {4:.4f}"
				output.write(output_str.format(hour, minute, second_str, lat, lon) + "\n")
				print(output_str.format(hour, minute, second_str, lat, lon))

				# Convert lat/lon into UTM (standardized 2D cartesian projection).
				x, y, _, _ = utm.from_latlon(lat, lon)

				# Set first point as origin (0,0).
				if not read_origin:
					x_origin = x
					y_origin = y
					read_origin = True

				# All other points are relative to this origin.
				else:
					x = x - x_origin
					y = y - y_origin

					# Add new data point.
					xdata.append(x)
					ydata.append(y)
					line.set_xdata(xdata)
					line.set_ydata(ydata)

					# Redraw plot and adjust axes.
					self.ax.draw_artist(self.ax.patch)
					self.ax.draw_artist(line)
					self.ax.relim()	
					self.ax.autoscale_view()
					self.graph.draw()
					self.graph.flush_events()

					# Compute and print absolute distance and angle from origin.
					dist  = math.sqrt(x**2 + y**2)
					angle = math.degrees(math.atan2(y,x))
					if text is not None:
						text.remove()
					data_str = "Distance: {0:.2f} m\nAngle: {1:.2f}$^\circ$".format(dist, angle)
					text = self.ax.text(0.05, 0.05, data_str, fontsize=12, transform=self.ax.transAxes, bbox=props)

		# Close the output file.
		output.close()	
	
		# Tell the main window to quit
		self.root.event_generate("<<QUIT>>", when="tail")

def app():
	root = Tk()
	root.config(background='white')
	root.title("Ground Station")

	w = root.winfo_screenwidth()
	h = root.winfo_screenheight()
	root.geometry("%dx%d+0+0" % (w, h))

	fig = Figure()
	
	graph = FigureCanvasTkAgg(fig, master=root)
	graph.get_tk_widget().pack(side="top",fill='both',expand=True)

	ax = fig.add_subplot(111)
	ax.set_xlabel("X axis")
	ax.set_ylabel("Y axis")
	ax.grid()
 
	def do_thing():
		print "Did the thing"
 
	b = Button(root, text="Do The Thing", command=do_thing, bg="red", fg="white")
	b.pack()

	plotter = Plotter(root, graph, ax)
	plotter.start()

	def actually_close(*args):	
		# Prompt user to save the figure.
		file_name = tkSimpleDialog.askstring("Save graph as:", "Enter a filename to save the graph under:")
		if not file_name == None:
			fig.savefig(file_name)

		# Close the window
		root.quit()
		root.destroy()
	
	def handle_close():
		plotter.closing.set()

	root.protocol("WM_DELETE_WINDOW", handle_close)
	root.bind("<<QUIT>>", actually_close)
	
	root.mainloop()
 
if __name__ == '__main__':
	app()
