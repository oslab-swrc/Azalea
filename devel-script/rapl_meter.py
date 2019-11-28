import os
import sys
import struct
import time
import threading

MAX_INTEGER = 0xffffffff

current_time = lambda: int(round(time.time() * 1000000))

class RaplMeter:

	def __init__(self):		
		self.energy_begin = 0
		self.energy_end = 0
		
		self.time_begin = 0
		self.time_end = 0
		
		self.ready = False
		os.system("modprobe msr")
		self.msr = self.open_msr()
		self.energy_unit = self.get_energy_unit()

		print("Energy counter unit: 1/(2^%d)J" % self.energy_unit)
		print("rapl inited")
	
	def open_msr(self):
		msr_path = "/dev/cpu/0/msr"
		msr = os.open(msr_path, os.O_RDONLY)
		self.ready = True
		return msr

	def read_msr(self, msr_no):
		os.lseek(self.msr, msr_no, os.SEEK_SET)
		val = struct.unpack('Q', os.read(self.msr, 8))[0]
		return val

	def close_msr(self):
		os.close(self.msr)
		self.ready = False

	def energy_now(self):
		val = self.read_msr(0x611)
		return val & 0xffffffff

	def get_energy_unit(self):
		val = self.read_msr(0x606)
		return (val & 0x1f00) >> 8

	def is_ready(self):
		return self.ready

	def energy_diff(self):
		diff = 0
		if self.energy_end < self.energy_begin:	#overflow
			diff = MAX_INTEGER - self.energy_begin
			diff += self.energy_end
		else:
		    diff = self.energy_end - self.energy_begin
		
		return diff

	def start(self):
		self.time_begin = current_time()
		self.energy_begin = self.energy_now()


	def stop(self):
		self.time_end = current_time()
		self.energy_end = self.energy_now()

	def get_power(self):
		time_us = self.time_end - self.time_begin
	
		energy_uj = (self.energy_diff() * 1000000) >> self.energy_unit
		power_w = float(energy_uj) / time_us
		return time_us, energy_uj, power_w

	def close(self):
		self.close_msr()

if __name__ == "__main__":
	pmterm = RaplMeter()

	for i in range(300):
		pmterm.start()
		time.sleep(1)
		pmterm.stop()

		print("%d\t%.2f" % (i, pmterm.get_power()[2]))
		
	pmterm.close()