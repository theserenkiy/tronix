import net from 'net'
import fs from "fs"

const port = 3210;
const host = '0.0.0.0' //to listen on all available network interfaces

const server = net.createServer((socket) => {
	console.log(`Client connected: ${socket.remoteAddress}:${socket.remotePort}`);

	// Event listener for data received from the client
	socket.on('data', (data) => {
		let s = data.toString()
		console.log(`Received from client: ${s}`);
		let out = ""
		let payload = fs.readFileSync("static/values.json")
		if(/^GET /.test(s))
		{
			out += "HTTP/1.1 200 OK\r\nContent-Length: "+payload.byteLength+"\r\nConnection: close\r\n\r\n"
		}

		out += payload
		// Echo the data back to the client
		socket.write(out);
	});

	// Event listener for client disconnection
	socket.on('end', () => {
		console.log(`Client disconnected: ${socket.remoteAddress}:${socket.remotePort}`);
	});

	// Event listener for errors
	socket.on('error', (err) => {
		console.error(`Socket error: ${err.message}`);
	});
});

server.listen(port, host, () => {
	console.log(`TCP server listening on ${host}:${port}`);
});

// Optional: Handle server errors
server.on('error', (err) => {
	console.error(`Server error: ${err.message}`);
});