import sys, getopt
import time
import struct
import os

system = os.name

if system != 'nt':
	if os.getuid() != 0:
		print "Please re-run with sudo"
		raw_input("Press enter to continue...")
		sys.exit()

try:
	import serial.tools
except ImportError, e:
	print "Attempting to download and install pyserial..."
			
	import urllib2
	url = "https://pypi.python.org/packages/source/p/pyserial/pyserial-2.6.tar.gz"
	file_name = url.split('/')[-1]
	u = urllib2.urlopen(url)
	f = open(file_name, 'wb')
	meta = u.info()
	file_size = int(meta.getheaders("Content-Length")[0])
	print("Downloading: {0} Bytes: {1}".format(url, file_size))

	file_size_dl = 0
	block_sz = 8192
	while True:
		buffer = u.read(block_sz)
		if not buffer:
			break

		file_size_dl += len(buffer)
		f.write(buffer)
		p = float(file_size_dl) / file_size
		status = r"{0}  [{1:.2%}]".format(file_size_dl, p)
		status = status + chr(8)*(len(status)+1)
		sys.stdout.write(status)

	f.close()
	
	try:
		with open(file_name): pass
	except IOError:
		print "There was an error downloading pyserial! Check your connection."
		raw_input("Press enter to continue...")
		sys.exit()
		
	print("Extracting pyserial...")
	import tarfile
	try:
		tar = tarfile.open(file_name)
		tar.extractall()
		tar.close()
	except:
		print "There was an error extracting pyserial!"
		raw_input("Press enter to continue...")
		sys.exit()
		
	print('Installing pyserial...')
	os.chdir('pyserial-2.6')
	os.system('python setup.py install')
	os.chdir('..')
	
	raw_input("Install Complete. Please re-run the script. Press enter to continue...")
	sys.exit()

from serial.tools import list_ports 

port = ''
baud = 115200
get_time = False
try:
	opts, args = getopt.getopt(sys.argv[1:], "p:b:", ["list","get"])
except getopt.GetoptError:
	print "sync_time.py -p <COMX> -b <baud_rate>"
	sys.exit()
for opt, arg in opts:
	if opt == '-p':
		print "Port specified: " + arg
		port = arg
	elif opt == '-b':
		try:
			baud = int(arg)
		except ValueError:
			print "Invalid baud rate specified. Must be a integer."
			sys.exit()
	elif opt == '--list':
		ports = [port[0] for port in list_ports.comports()]
		if len(ports) == 0:
			print "No available serial ports found!"
			sys.exit()
		print "Available serial ports:"
		for p in ports:
			print p
		sys.exit()
	elif opt == '--get':
		get_time = True
		

if port == '':
	ports = [port for port in list_ports.comports()]
	if len(ports) > 0:
		if system == 'posix':
			p = ports[len(ports) - 1]
			port = p[0]
		elif system == 'nt':
			p = ports[0]
			port = p[0]
		elif system == 'mac':
			p = ports[len(ports) - 1]
			port = p[0]
		else:
			print "What system are you running?!"
			sys.exit()
		print "No port specified, using best guess serial port:\r\n" + p[1] + ", " + p[2] + "\r\n"
	else:
		print "Cannot find default port and no port given!"
		sys.exit()

com = None
try:
		com = serial.Serial(port, baud, timeout=1);
		print "Connected to " + port + " @ " + str(baud) + " baud"
except serial.SerialException, e:
	print "Unable to connect to the given serial port!\r\nTry the --list option to list available ports."
	print e
	sys.exit()
	
t = time.localtime()

epoch = int(time.mktime(t)) - time.altzone
data = struct.pack("I", epoch)

try:
	if get_time:
		print('Getting time...')
		b = com.write(bytes("gTIME"))
		res = com.read(5)
		try:
			if res[0] == 't':
				print('Current clock time:')
				unix = struct.unpack("<L", res[1:])[0] + time.altzone
				print('unix epoch: ' + str(unix))
				print(time.strftime('%m-%d-%Y %H:%M:%S', time.localtime(unix)))
			else:
				print('Error retrieving time!')
		except IndexError:
			print('Error retrieving time!')
	else:
		print ("Setting time to {0:02d}\\{1:02d}\\{2} {3:02d}:{4:02d}:{5:02d}".format(t.tm_mon, t.tm_mday, t.tm_year, t.tm_hour, t.tm_min, t.tm_sec))
		b = com.write(bytes("t" + data))
		res = com.read()
		if(b > 0 and res == '*'):
			print "Success syncing time!"
		else:
			print "There was an error syncing the time! Make sure your clock is in Serial Set Mode"
	com.close()
except serial.SerialTimeoutException:
	print "Timeout sending sync data! Please check your serial connection"
	sys.exit()

