
let fxtal = 25e6

const cl = console.log

let regs = [
	[2,0],            //interrupt masks: none
	[3,0b11111110],   //enebled outputs: CLK0
	[9,0xff],         //OEB: disable all (not used)
	[15,0],           //PLL source: XTAL for all
	[16,0b01001111],  //CLK0 cfg:
					//  7: PWRUP,
					//  6: MultiSynth0 is in frac mode,
					//  5: MS0 src = PLLA
					//  4: Invert out - not
					//  3:2: input source: MS0
					//  1:0: Out current = 8mA

	[24,0],           // CLKx_DIS_STATE = LOW
	[165,0],          //CLK0 phase offset = 0
	[183,0b11000000]  //XTAL load cap = 10pf

];

function setReg(n,v)
{
	let reg = regs.find(e => e[0]==n)
	if(!reg){
	reg = [n,v]
	regs.push(reg)
	}
	else
	reg[1] = v
}

function getReg(n)
{
	let reg = regs.find(e => e[0]==n)
	return reg ? reg[1] : 0;
}


(async ()=>{
	try{

	cl(mkFraction(+(process.argv[2])))

	return

	let fout = +(process.argv[2] || 0)

	if(!fout)throw 'Missing fout'

	let fvco,abc;
	if(fout > 112.5e6){

	let r16 = getReg(16)
	r16 += 1<<6
	setReg(16,r16)

	//gen only with VCO/PLL (MultiSynth = 4,6 or 8)
	fvco = fout*4
	abc = mkFraction(fvco/fxtal);

	let MSNA_P1 = abc.a*128 + Math.floor(128*abc.b/abc.c) - 512
	let MSNA_P2 = abc.b*128 - abc.c*Math.floor(128*abc.b/abc.c)
	let MSNA_P3 = abc.c
	}


	}catch(e){
	cl(e)
	}
})()


function mkFraction(num)
{
	let intg = Math.floor(num);
	num = num%1
	let best_proximity;
	for(let i=1;i < 1<<19;i++){
	let v = Math.floor(num*i)
	let num_ = v/i
	let prox = num_ < num ? num_/num : num/num_

	if(!best_proximity || prox > best_proximity[0])
		best_proximity = [prox,v,i]

	if(prox > 0.99999)
		break
	}

	return {a:intg,b:best_proximity[1],c:best_proximity[2]}

}
