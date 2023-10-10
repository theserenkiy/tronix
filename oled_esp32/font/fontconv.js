//User-defined params:

const inputFile = 'font16.bmp';
const symHeightPixels = 16
const glyphs = '0123456789ABCDEF'

//####################################
const fs = require('fs')
const bmp = require("bmp-js");

const cl = console.log

const symHeightBytes = Math.ceil(symHeightPixels/8)
const symHeight = symHeightBytes*8
const glyphlist = glyphs.split('')

let bmpBuffer = fs.readFileSync(inputFile);
let bmpData = bmp.decode(bmpBuffer).data;

cl({bmpData})

let gs = []
for(let i=0; i < bmpData.length/4; i++)
{
	gs.push((bmpData[i*4+1]+bmpData[i*4+2]+bmpData[i*4+3])/3 < 200)
}


let cols = [];
let imgWidth = gs.length/symHeight;
for(let i=0;i < imgWidth;i++)
{
	let col = []
	for(let j=0;j < symHeight;j++)
		col = col | (gs[imgWidth*j+i] ? 1 : 0) << j
	cols.push(col)
}

let letters_cols = [[]]
let letterNum = 0;
for(let col of cols)
{
	if(!col)
	{
		if(letters_cols[letterNum].length)
		{
			letterNum++
			letters_cols[letterNum] = []
		}
		continue
	}
	letters_cols[letterNum].push(col)
	//cl(col.toString(2).padStart(8,'0'))
}

let letters_bytes = {}
let num = 0;
for(let letter of letters_cols)
{
	if(!letter.length)continue;

	let letter_b = [];
	for(let bnum=0;bnum < symHeightBytes;bnum++)
	{
		for(let col of letter)
		{
			letter_b.push((col >> (bnum*8)) & 0xff)
		}
	}
	let glyph = glyphlist[num++] || '@'+num++
	letters_bytes[glyph] = letter_b;
}

let font = {
	height_bytes: symHeightBytes,
	glyphs: letters_bytes
}
let out = 'font = '+JSON.stringify(font)
let outputFile = inputFile.replace(/\.[^\.]+$/,'')+'.py';
fs.writeFileSync(outputFile,out)

for(let [glyph,bytes] of Object.entries(letters_bytes))
{
	let letterWidth = bytes.length/symHeightBytes
	cl({glyph,letterWidth})
	// for(let b of letter)
	// 	cl(b.toString(2).padStart(8,'0'))

	for(let colnum=0;colnum < letterWidth;colnum++)
	{
		let bb = []
		for(let bnum=symHeightBytes-1;bnum >= 0; bnum--)
		{
			let b = bytes[bnum*letterWidth+colnum];
			bb.push(b.toString(2).padStart(8,'0'))
		}
		cl(bb.join(' '))
	}
	cl('')
}


cl('\nWell done!\nData written to '+outputFile)