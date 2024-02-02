const cl = console.log;

//array position = bit address on target device (e.g ROM bus)
//value = bit address on source device (e.g counters driving ROM)
let remap = [
	2,0,1
]

function makeBitRemapFunction(remap,fwd=1)
{
	if([...new Set(remap)].length != remap.length)
	throw 'Incorrect bit remap (some values duplicated)'

	let difs = {}
	for(let [i,v] of remap.entries())
	{
		let dif = fwd ? (v-i) : (i-v);
		if(!difs[dif])
			difs[dif] = [];
		difs[dif].push(fwd ? i : v);
	}
	let exp = []
	for(let dif in difs){
		let shifter = '';
		dif = +dif;
		if(dif != 0)
			shifter = (dif > 0 ? '<<' : '>>')+Math.abs(dif);
		let d = difs[dif];
		let max_bit = Math.max(...d)
		let mask = '';
		for(let i=0;i <= max_bit;i++)
		{
			mask = (d.includes(i) ? '1' : '0')+mask;
		}
		exp.push('((a & 0b'+mask+')'+shifter+')');
	}

	exp_s = exp.join('|')

	cl(exp_s)

	return new Function(['a'],'return '+exp_s);
}


let fwd = makeBitRemapFunction(remap,1);
let bwd = makeBitRemapFunction(remap,0);

let len = remap.length;
let maxnum = Math.pow(2,len);

let start = Date.now()

for(let i=0;i < maxnum;i++)
{
	cl(i.toString(2).padStart(len,'0')+' => '+fwd(i).toString(2).padStart(len,'0')+' / '+bwd(i).toString(2).padStart(len,'0'))
}

cl('Time taken: '+(Date.now()-start))