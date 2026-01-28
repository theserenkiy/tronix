const port = 2345;
const host = 'http://localhost:'+port;

process.title = "2812 SERVER @ "+port;

import path from 'path';
import fs from 'fs';
import express from "express";
import bodyParser from 'body-parser';

const root = path.resolve('.');
const cl = console.log

const app = express();
app.disable('etag');
app.use(express.static(root+'/static'))
app.use(bodyParser.json())

app.get('/', (req, res)=>{
	res.sendFile(root+"/index.html");
})

app.post('/api/:cmd',(req,res) => {

	let out = {ok: 1};
	let b = req.body;
	
	cl(b)
	try{
		let cmd = req.params.cmd
		switch(cmd)
		{
			case "set":
				fs.writeFileSync(root+"/static/values.json",JSON.stringify(b.values))
				break
			default:
				throw "Unknown command "+cmd;
		}
		out.ok = 1
	}
	catch(e)
	{
		cl('ERROR',e)
		out = {ok: 0, error: e+''}
	}
	
	res.json(out) 
})

app.listen(port, ()=>cl('Listening on port '+port));