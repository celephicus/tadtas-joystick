#!/usr/bin/env python3
import sys

import numpy as np
import sounddevice as sd

start_idx = 0
f1, f2 = 500, 500
amplitude = 0.4
device = None

samplerate = sd.query_devices(device, 'output')['default_samplerate']

def mk_samples(t, f):
	a = amplitude if f != 500 else 0.0
	return a * np.sin(2 * np.pi * f * t)
	
def callback(outdata, frames, time, status):
	if status:
		print(status, file=sys.stderr)
	global start_idx
	t = (start_idx + np.arange(frames)) / samplerate
	t = t.reshape(-1, 1)
	outdata[:] = list(zip(mk_samples(t, f1), mk_samples(t, f2)))
	start_idx += frames

stros = sd.OutputStream(device=device, channels=2, callback=callback, samplerate=samplerate)
stros.start()

import msvcrt
while True:
	if msvcrt.kbhit():
		key_stroke = msvcrt.getch()
		x = key_stroke.decode()
		if x == '1': f1 += 20
		if x == '2': f1 -= 20
		if x == '3': f2 += 20
		if x == '4': f2 -= 20
		if x == 'x': sys.exit(0)
		
