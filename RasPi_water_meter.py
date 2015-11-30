#
#Main code for RaspPi-based water-use monitor
#
from flask import Flask, request  
from flask import render_template
from flask import send_from_directory
import random  
import time
import RPi.GPIO as GPIO  
import os
import serial

global totalUse
totalUse = 0.0

ser = serial.Serial(
	port='/dev/ttyAMA0',
	baudrate = 1200,
	parity=serial.PARITY_NONE,
	stopbits=serial.STOPBITS_ONE,
	bytesize=serial.EIGHTBITS,
	timeout=10
)


#Init Flask   
app = Flask(__name__)
app.debug = True
 

#Main flask handler   
@app.route('/')  
def home(name=None):  	
	return render_template('meter.html')
	
#Handler for shutting down the Pi gracefully
@app.route('/shutdown')
def shutdown(name=None):
	os.system("sudo shutdown -h now")
	return render_template('shutdown.html', selectedcss = 'ccompWonB.css')

#Handler for more button on main page
@app.route('/reset')
def reset(name=None):
	global totalUse
	totalUse = 0.0
    
	#Return the (space-separated) flow rate, total use
	return  ('%.1f' % 0.0) + \
	" " + ('%d' % totalUse)

	
#Handler for getting data (the HTML run by the main handler fires this periodically)
@app.route('/data')
def ccomp(name=None):
	global totalUse
	
	#Every time this handler is hit, let's get the latest flow rate from the serial port
	ser.flushInput()
	rcv = ser.readline()
	print rcv
	
	flowRate = float(rcv)
	totalUse = totalUse + (flowRate * 3.0)
    
	#Return the (space-separated) flow rate, flowStatTK
	return  ('%.1f' % flowRate) + " " + ('%d' % totalUse)


#Run the webserver (Note: The __name__ stuff is not needed here. It's useful only when the code is separated into modules, that may or may not be run as main.)
if __name__ == "__main__":
	app.run(host='0.0.0.0', port=int('80'), debug=True)
    

exit()

	

