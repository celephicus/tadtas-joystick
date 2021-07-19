import PySimpleGUI as sg
import serial, serial.tools.list_ports
import sys

serial_port = None

comms_layout = [
	[ 
		sg.Text('Port:'), 
		sg.Combo(values=[], key='-PORT-', size=(30, 1), disabled=True), # Note can be in one of readonly or disabled or enabled. 
		sg.Button('Refresh', key='-REFRESH-'),
		sg.Button('Connect', key='-CONNECT-'),
		sg.Button('Clear Log', key='-CLEAR-LOG-'),
	],
	[ sg.Text('Log'), ],
	[ sg.Multiline('', key='-LOG-', size=(None, 5), autoscroll=True, auto_refresh=True, write_only=True) ],
]

layout = [
	[ sg.Frame('Serial Port', comms_layout) ],
]
	
win = sg.Window('Customise your Journey', layout, finalize=True)

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
	
# Initial fill of combo box at startup. 
refresh_ports()
refresh_connect_button()

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
			serial_port = serial.Serial(comport_device, baudrate=115200)
		except serial.serialutil.SerialException:
			close_serial_port()
			log("failed.")
		else:
			log("OK.")
	else:
		log("Closed {}.".format(serial_port.port))
		close_serial_port()
	refresh_connect_button()
		
while True:
	e, v = win.read(timeout=20)
	if e == '__TIMEOUT__':
		# Do something!
		continue
	print(e, v)
	if e in (sg.WIN_CLOSED, 'Exit'):
		break
	if e == '-REFRESH-':
		refresh_ports()
	if e == '-CLEAR-LOG-':
		win['-LOG-'].update(value='')
	if e == '-CONNECT-':
		do_connect_action(v['-PORT-'])

win.close()