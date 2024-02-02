import express from 'express';
import path from 'path';
import api from './api.js';
import {cl} from './server_lib.js';
const app = express();


app.use(express.static('static'))
//app.use(express.urlencoded({ extended: true }))
app.use(express.json())

app.get('/',(req,res) => {
    res.sendFile(path.resolve('index.html'))
})

app.post('/api/:cmd',async (req,res)=>{
    let out = {}
    try{
        let cmd = req.params.cmd;
        if(!api[cmd])
            throw `Unknown api command "${cmd}"`;
        cl('Api cmd '+cmd)
        out = await api[cmd](req.body,req,res);
    }catch(e){
        cl('ERROR '+e)
        out.error = e+'';
    }
    res.json(out);
})

app.listen(8080,()=>cl('Listening 8080'))