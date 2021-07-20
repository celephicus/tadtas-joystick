import PySimpleGUI as sg
import serial, serial.tools.list_ports
import sys

serial_port = None
connected = True

comms_layout = [
	[ 
		sg.Text('Port'), 
		sg.Combo(values=[], key='-PORT-', size=(30, 1), disabled=True), # Note can be in one of readonly or disabled or enabled. 
		sg.Button('Refresh', key='-REFRESH-'),
		sg.Button('Connect', key='-CONNECT-'),
		sg.Button('Clear Log', key='-CLEAR-LOG-'),
	],
	[ sg.Text('Log'), ],
	[ sg.Multiline('', key='-LOG-', size=(None, 5), autoscroll=True, auto_refresh=True, write_only=True) ],
]

SCALING = (6, 5, 4, 3, 2, 1)	# Powers of 10.
SUPERSCRIPTS = u"\u2070\u00b9\u00b2\u00b3\u2074\u2075\u2076\u2077\u2078\u2079"
SCALING_TEXT = [u"\u00B1"+u"8"+u"\u00d7"+u"10{}".format(SUPERSCRIPTS[s]) for s in SCALING]
SCALING_VALUES = dict(zip(SCALING_TEXT, [8 + 10 ** s for s in SCALING]))



js_layout = [ [
	sg.Button('Zero', key='-ZERO-'), 
	sg.Text('Scale'), 
	sg.Combo(values=SCALING_TEXT, metadata=SCALING_VALUES, readonly=True, size=(7, 1), default_value=SCALING_TEXT[0], key='-SCALE-'), 
	sg.Text('Filter'), 
	sg.Combo(values=list(map(str, range(5))), readonly=True, size=(7, 1), default_value='0', enable_events=True, key='-FILTER-'), 
] ] + [
		[ 
		sg.Text(z, size=(1, 1)), 
		sg.ProgressBar(1000, orientation='h', size=(45, 20), key='-{}-BARGRAPH-'.format(z)), 
		sg.Text(size=(10, 1), background_color='green', text_color='red', key='-{}-VALUE-'.format(z)),
	] for z in 'XYZ'
]

layout = [
	[ sg.Frame('Serial Port', comms_layout) ],
	[ sg.Frame('Joystick', js_layout) ],
]
	
win = sg.Window('TADTas Zero Force Joystick Evaluator', layout, finalize=True)

def log(*args, **kwargs):
	print(*args, **kwargs, file=sys.stderr)
	win['-LOG-'].print(*args, **kwargs)
	
# List of ports held in combo box element. 
def refresh_ports():
	port_list = [(x.device, x.description) for x in sorted(serial.tools.list_ports.comports())]
	port_descriptions = [p[1] for p in port_list]
	log("Serial ports: {}".format(', '.join(port_descriptions)))
	win['-PORT-'].update(values=port_descriptions, set_to_index=0)
	win['-PORT-'].metadata = {p[1]: p[0] for p in port_list}
	if len(port_list) < 1:
		win['-PORT-'].update(disabled=True)
	else:
		win['-PORT-'].update(readonly=True)

def refresh_connect_button():
	win['-CONNECT-'].update(text="Connect" if serial_port is None else "Disconnect")

def refresh_joystick_values():
	win['-SCALE-'].update(disabled=not connected)
	for z in 'XYZ':
		win['-{}-BARGRAPH-'.format(z)].update(0)		# Just set curent to zero, if we are connected then it will get updated.
		win['-{}-VALUE-'.format(z)].update(value='0')

## Initialise.
refresh_ports()
refresh_connect_button()
refresh_joystick_values()

def close_serial_port():
	global serial_port
	if serial_port is not None:
		try: serial_port.close()
		except serial.serialutil.SerialException: pass
		serial_port = None

def do_connect_action(port_description):
	global serial_port
	if serial_port is None:
		comport_device = win['-PORT-'].metadata[port_description]
		log("Connect: {} ".format(comport_device), end='')
		try:
			serial_port = serial.Serial(comport_device, baudrate=115200, timeout=0.01)
		except serial.serialutil.SerialException:
			close_serial_port()
			log("failed.")
		else:
			send_js_command('1 js-dump')
			log("OK.")
	else:
		send_js_command('0 js-dump')		# Turn off dump.
		log("Closed {}.".format(serial_port.port))
		close_serial_port()
	refresh_connect_button()

def do_update_values_action(vals):
	scale = win['-SCALE-'].metadata[win['-SCALE-'].get()]
	for i, v in enumerate(vals):
		z = 'XYZ'[i]
		scaled_val = (1.0 + float(v) / scale) * 500.0
		win['-{}-BARGRAPH-'.format(z)].update(int(scaled_val+0.5))		
		win['-{}-VALUE-'.format(z)].update(value=str(v))

raw_line = b''
def do_read_joystick():
	global raw_line
	if serial_port is not None:
		try: 
			raw_line = raw_line + serial_port.readline()
		except serial.serialutil.SerialException: 
			close_serial_port()
			log("Serial port exception, port closed.")
			refresh_connect_button()
		else:
			if raw_line:
				if raw_line[-1] == 10:
					try: 
						line = raw_line.decode('ascii').strip()
					except ValueError: 
						log("Could not decode `{}'.".format(raw_line))
					else:
						log(line)
						if line.startswith("HX711:"):
							vals = line.split()
							if len(vals) != 5:
								log("Invalid format.")
							else:
								do_update_values_action(map(int, vals[2:5]))
					raw_line = b''

def send_js_command(cmd):
	if serial_port is not None:
		log("Sent command `{}.'".format(cmd))
		try: 
			serial_port.write((cmd + '\r').encode('ascii'))
		except serial.serialutil.SerialException: 
			pass	
							
while True:
	e, v = win.read(timeout=20)
	if e == '__TIMEOUT__':
		do_read_joystick()
		continue
	print(e, v)
	if e in (sg.WIN_CLOSED, 'Exit'):
		break
	elif e == '-REFRESH-':
		refresh_ports()
	elif e == '-CLEAR-LOG-':
		win['-LOG-'].update(value='')
	elif e == '-CONNECT-':
		do_connect_action(v['-PORT-'])
	elif e == '-ZERO-':
		send_js_command('js-z')
	elif e == '-FILTER-':
		send_js_command('{} 3 v!'.format(v['-FILTER-']))


win.close()