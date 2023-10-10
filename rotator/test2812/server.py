import socket

sock = socket.socket()
sock.bind(('',80))
sock.listen(1)

piska = 1

while True:
    conn, addr = sock.accept()
    print('Connected ',addr)

    while True:
        data = conn.recv(1024)
        if not data:
            break
        print(data)
        payload = 'Danya Piska '+str(piska)
        conn.send(bytearray('HTTP/2 200 OK\r\nContent-Length: '+str(len(payload))+'\r\nConnection:close\r\n\r\n'+payload,'ascii'))
        conn.close()
        piska += 1
        break