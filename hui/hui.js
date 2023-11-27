let fs = require('fs');
let wav = require('node-wav');

let file = 'hui';

let buffer = fs.readFileSync(file+'.wav');
let result = wav.decode(buffer);
let bytes = [...result.channelData[0]].map(v => {
	let num = Math.round(128*(v+1));
	if(num==0)num = 1;
	return num;
})
console.log(result.sampleRate);
console.log(bytes);

let pre=[],post=[];
for(let i=1; i < 128; i+=4)
{
	pre.push(i);
	post.push(128-i);
}

bytes = [...pre,...bytes,...post,0,0];

let str = '';
for(let i=0; i < bytes.length; i+=16)
{
	let slice = bytes.slice(i,i+16).map(v => '0x'+v.toString(16));
	str += '.db '+slice.join(', ')+'\n';
}

fs.writeFileSync(file+'.bytes',str)
