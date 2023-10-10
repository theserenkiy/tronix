import socket

sock = socket.socket()
sock.connect(('localhost', 666))
#sock.send(bytearray('','ascii'))

data = sock.recv(1024)
sock.close()

print(data)