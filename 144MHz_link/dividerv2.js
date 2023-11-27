
let base_freq = 144000000;
let step = 25000;
let xtal = 16000000;
let divider = 256;

let out = {}
for(let i=0; i < 80; i++)
{
	let freq = base_freq+(i*step);
	let divfreq = freq/divider;
	let div100k = divfreq/8192;//16384;
	let tacts = Math.floor(xtal/div100k);
	let b256 = [Math.floor(tacts/256),tacts%256];
	out[freq] = {div:div100k.toFixed(2),tacts,b256};
}

console.log(out)
