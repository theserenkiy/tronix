const net = require('net');
const fs = require('fs')

const cl = console.log

let piska = 1;
const server = net.createServer(function(socket) {
    cl('Кто-то нежно в нас вошёл...')
    socket.on('data',()=>{
        
    })
	let payload = 'Danya Piska '+piska;
    //socket.write('HTTP/2 200 OK\r\nContent-Length: '+payload.length+'\r\nConnection: close\r\n\r\n'+payload);
    socket.write(new Uint8Array((fs.readFileSync('image.txt')+'').split(/[\s\,]+/).map(v => Math.floor((+v)/16))))
    socket.pipe(socket);
    socket.end()
    piska++
});

server.listen(666, '');

// var net = require('net');

// var client = new net.Socket();
// client.connect(1337, '127.0.0.1', function() {
// 	console.log('Connected');
// 	client.write('Hello, server! Love, Client.');
// });

// client.on('data', function(data) {
// 	console.log('Received: ' + data);
// 	client.destroy(); // kill client after server's response
// });

// client.on('close', function() {
// 	console.log('Connection closed');
// });