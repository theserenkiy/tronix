

let base_freq = 144000000;
let step = 25000;
let xtal = 16000000;
let ext_divider = 256;
let int_divider = 8192;

let xtal_div = 320000;

let out = {}
for(let i=0; i < 80; i++)
{
	let freq = base_freq+(i*step);
	out[freq] = calcV2(freq)
}

console.log(out)

function calcV1(freq)
{
	let divfreq_ext = freq/ext_divider;
	let divfreq_int = divfreq_ext/int_divider;
	let tacts = Math.floor(xtal/divfreq_int);
	let b256 = [Math.floor(tacts/256),tacts%256];
	return {div:divfreq_int.toFixed(2),tacts,b256};
}

function calcV2(freq)
{
	let divfreq_ext = freq/ext_divider;				//frequency from external divider (Fgen/256)
	let divfreq_int = divfreq_ext/int_divider;		//frequency after internal divider /256/n
	let tacts = Math.floor(xtal/divfreq_int);
	let b65k = [Math.floor(tacts/65536),(tacts%65536).toString(16)];
	return {div:divfreq_int.toFixed(2),tacts,b65k};
}
