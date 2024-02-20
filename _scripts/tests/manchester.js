const fs = require('fs');
const cl = console.log;

const period = 8; //ticks
const jitter = 0; 

function writeStream(stream,val,tacts)
{
	tacts += Math.round(Math.random()*jitter*2)-jitter;
	for(let i=0;i < tacts;i++)
		stream.push(val ? 1 : 0);
}

function mkManchester(data)
{
	const stream = [];
	let state = 0;
	for(let byte of data)
	{
		cl(byte.toString(2))
		for(let i=7; i >= 0;i--)
		{
			let bit = (byte & (1 << i)) ? 1 : 0;
			//cl(bit)
			if(!bit)
			{
				state = !state;
			}
			writeStream(stream,state,period/2);
			state = !state;
			writeStream(stream,state,period/2);
		}
	}
	//cl('--------');
	return stream;
}


function decode(stream)
{
	let prev = 0;
	let in_delay = 0;
	let v0;
	for(let i=0; i < stream.length; i++)
	{
		let v = stream[i];
		if(in_delay)
		{
			in_delay = 0;
		}
		if(prev==v)
			continue;
		
		in_delay = 1;
		i += (period*1.5)-1;
		v0 = v;
	}
}

function readManchester(stream,skip=0)
{
	let data_bits = [];
	let prev = 0;
	let counter = 0;
	let in_delay = false;
	let v0,v1;
	let a='',b='';
	for(let v of stream)
	{
		if(skip)
		{
			skip--;
			continue;
		}
		a += v;
		counter++;
		if(in_delay)
		{
			if(counter >= period*0.75)
			{
				b += 'C';
				v1 = v;
				data_bits.push((v0 == v1) ? 1 : 0);
				in_delay = false;
				prev = v;
				continue;
			}
			else
			{
				b += '-';
				continue;
			}
		}

		if(v==prev)
		{
			b+= ' ';
			continue;
		}
		//cl('e',v)
		b += 'E'
		v0 = v;
		prev = v;
		counter = 0;
		in_delay = true;
	}
	fs.writeFileSync('stream.txt',a+'\n'+b);
	return data_bits;
}

function checkManchester(len_bytes)
{
	let in_data = [0b10000000];
	for(let i=0;i < len_bytes;i++)
	{
		in_data.push(Math.floor(Math.random()*256));
	}
	let stream = mkManchester(in_data);
	let out_bits = readManchester(stream);

	let ptr = 0;
	let in_bits = [];
	for(let bit of out_bits){}

}

cl('INPUT DATA:');
let stream = mkManchester([0b10000001,0b10100110]);
cl('\nSTREAM: ',stream.join(''));
let data = readManchester(stream,0);

let slices = []
for(let i=data.length;;i-=8)
{
	let s = i < 0 ? 0 : i;
	slices.push(data.slice(s,i+8).join(''));
	if(!s)break;
}

cl('\nDECODED DATA: \n',slices.reverse().join('\n'));