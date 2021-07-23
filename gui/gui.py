import PySimpleGUI as sg
import serial, serial.tools.list_ports
import sys, math

serial_port = None
connected = True

comms_layout = [
	[ 
		sg.Text('Port'), 
		sg.Combo(values=[], key='-PORT-', size=(30, 1), disabled=True), # Note can be in one of readonly or disabled or enabled. 
		sg.Button('Refresh Ports List', size=(7, 2), key='-REFRESH-'),
		sg.Button('Connect', key='-CONNECT-'),
		sg.Button('Clear Log', key='-CLEAR-LOG-'),
	],
	[ sg.Text('Log'), sg.Checkbox('Log raw data', key='CB-LOG-RAW-DATA'), ],
	[ sg.Multiline('', key='-LOG-', size=(None, 5), autoscroll=True, auto_refresh=True, write_only=True) ],
]

# Scaling for joystick vales which are 24 bit signed so approx +/-8.34e6 fullscale.
JS_SCALING = [int(2**23 / math.sqrt(10)**x) for x in range(7)]
JS_SCALING_TEXT = ('min',) + tuple(map(str,range(1, len(JS_SCALING)-1))) + ('max',)
JS_SCALING_VALUES = dict(zip(JS_SCALING_TEXT, JS_SCALING))

# GUI elements are scaled to this value, progress bar 0..max, graph canvas -max..max.
GUI_MAX = 1000.0

js_layout = [ [
	sg.Button('Zero', key='-ZERO-'), 
	sg.Text('Scale'), 
	#sg.Slider(range=(0,len(JS_SCALING)), orientation='h', size=(10, 5), enable_events=True, key='-SCALE-'),       
	sg.Combo(values=JS_SCALING_TEXT, metadata=JS_SCALING_VALUES, readonly=True, size=(7, 1), default_value=JS_SCALING_TEXT[0], enable_events=True, key='-SCALE-'), 
	sg.Text('Filter'), 
	sg.Combo(values=list(map(str, range(5))), readonly=True, size=(7, 1), default_value='0', enable_events=True, key='-FILTER-'), 
] ] + [
		[ 
		sg.Text(z, size=(1, 1)), 
		sg.ProgressBar(GUI_MAX, orientation='h', size=(45, 20), key='-{}-BARGRAPH-'.format(z)), 
		sg.Text(size=(10, 1), background_color='white', text_color='black', key='-{}-VALUE-'.format(z)),
	] for z in 'XYZ'
]

layout = [
	[ sg.Frame('Serial Port', comms_layout) ],
	[ sg.Frame('Joystick', js_layout) ],
	[ sg.Graph(canvas_size=(400, 400), graph_bottom_left=(-GUI_MAX, -GUI_MAX), graph_top_right=(GUI_MAX, GUI_MAX), background_color='white', key='graph')],      
]
	
win = sg.Window('TADTas Zero Force Joystick Evaluator', layout, finalize=True)

def log(*args, **kwargs):
	print(*args, **kwargs, file=sys.stderr)
	win['-LOG-'].print(*args, **kwargs)

def send_js_command(cmd):
	if serial_port is not None:
		log("Sent command `{}.'".format(cmd))
		try: 
			serial_port.write((cmd + '\r').encode('ascii'))
		except serial.serialutil.SerialException: 
			pass	
	
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
	refresh_connect_button()

def refresh_connect_button():
	win['-CONNECT-'].update(text="Connect" if serial_port is None else "Disconnect", disabled=not win['-PORT-'].metadata)

def refresh_joystick_values():
	win['-SCALE-'].update(disabled=not connected)
	for z in 'XYZ':
		win['-{}-BARGRAPH-'.format(z)].update(0)		# Just set curent to zero, if we are connected then it will get updated.
		win['-{}-VALUE-'.format(z)].update(value='0')

def do_action_filter(f):
	send_js_command('{} 3 v!'.format(f))
def do_zero_action():
	send_js_command('js-z')

## Initialise.
refresh_ports()
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
		try:
			serial_port = serial.Serial(comport_device, baudrate=115200, timeout=0.01)
		except serial.serialutil.SerialException:
			close_serial_port()
			log("Connect failed on {}.".format(comport_device))
		else:
			log("Connect OK on {}.".format(comport_device))
			send_js_command('1 js-dump')
			do_action_filter(win['-FILTER-'].get())
			do_zero_action()
			do_action_rescale()
	else:
		send_js_command('0 js-dump')		# Turn off dump.
		log("Closed {}.".format(serial_port.port))
		close_serial_port()
	refresh_connect_button()

def get_scale():
	scale_descr = win['-SCALE-'].get()
	return scale_descr, win['-SCALE-'].metadata[scale_descr]
	
def do_update_values_action(vals):
	scale_descr, scale = get_scale()
	scaled_vals = [float(x) / scale for x in vals]
	for i, v in enumerate(vals):
		z = 'XYZ'[i]
		win['-{}-BARGRAPH-'.format(z)].update(int((1.0 + scaled_vals[i]) * GUI_MAX / 2.0 + 0.5))		
		win['-{}-VALUE-'.format(z)].update(value=str(v))
	
	CURSOR_CIRCLE,CURSOR_CROSSHAIR,CURSOR_LINEWIDTH = GUI_MAX/10,GUI_MAX/5, 2
	win['graph'].erase()
	X,Y = scaled_vals[0] * GUI_MAX, scaled_vals[1] * GUI_MAX
	win['graph'].draw_circle((X,Y), CURSOR_CIRCLE, line_color='black', line_width=CURSOR_LINEWIDTH)
	win['graph'].draw_line((X-CURSOR_CROSSHAIR,Y), (X+CURSOR_CROSSHAIR,Y), color='black', width=CURSOR_LINEWIDTH)
	win['graph'].draw_line((X,Y-CURSOR_CROSSHAIR), (X,Y+CURSOR_CROSSHAIR), color='black', width=CURSOR_LINEWIDTH)

	
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
						if win['CB-LOG-RAW-DATA'].get():
							log(line)
						if line.startswith("HX711:"):
							vals = line.split()
							if len(vals) != 5:
								log("Invalid format.")
							else:
								do_update_values_action(list(map(int, vals[2:5])))
					raw_line = b''

def do_action_rescale():
	scale_descr, scale = get_scale()
	log(f"Fullscale set to \u00b1{scale:.3g} ({scale_descr}).")
							
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
		do_zero_action()
	elif e == '-FILTER-':
		do_action_filter(v['-FILTER-'])
	elif e == '-SCALE-':
		do_action_rescale()
		
win.close()