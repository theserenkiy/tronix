import network, machine, usocket

def init_wlan(ssid, key):
	wlan = network.WLAN()       # create station interface (the default, see below for an access point interface)
	wlan.active(True)           # activate the interface
	if not wlan.isconnected():
		print('connecting to network...')
		wlan.connect(ssid,key)
		while not wlan.isconnected():
			machine.idle()

	print('network config:', wlan.ipconfig('addr4'))


def fetch_socket(host,port,msg="a",len=8192):
	res = None
	try:
		# Create a socket object
		client_socket = usocket.socket(usocket.AF_INET, usocket.SOCK_STREAM)
		# This assumes that if "type" is not specified, an address for
		# SOCK_STREAM will be returned, which may be not true
		addrinfo = usocket.getaddrinfo(host, port)
		print(addrinfo)
		# client_socket.connect(addrinfo[0][-1])

		# Connect to the server
		client_socket.connect((host,port))
		print(f"Connected to {host}:{port}")

		# Send data to the server
		client_socket.send(msg.encode())

		# Receive data from the server
		data = client_socket.recv(len)
		res = data.decode()
		# print("Received from server: ",res)
		
	except Exception as e:
		print(f"An error occurred: {e}")
	finally:
		# Close the socket
		if 'client_socket' in locals() and client_socket:
			client_socket.close()
			print("Connection closed.")
		return res
	

GAMMA = [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11, 12, 12, 13, 14, 14, 15, 15, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 27, 28, 29, 30, 32, 33, 35, 36, 38, 40, 42, 43, 45, 48, 50, 52, 54, 57, 60, 62, 65, 68, 71, 75, 78, 82, 85, 89, 93, 98, 102, 107, 112, 117, 122]