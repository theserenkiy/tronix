const bmp = require('bmp-js')
const fs = require('fs')
const file = './govno.bmp';
const rows = 2;
const cols = 70;
const cl = console.log;

function makePixels(data,col,row)
{
    let pixels = [];
    let start = col+cols*row*8;
    for(let i=0;i < 8;i++)
    {
        //bytes.push(0,data[start+i*cols] ? 255 : 0,0)
        pixels.push(data[start+i*cols])
    }
    return pixels
}

let buf = fs.readFileSync(file)

let bmpData = bmp.decode(buf)

let pixels = [...bmpData.data.filter((v,i)=>!((i+1)%4)).map(v => v > 128 ? 0 : 1)];


let img = []
for(let col=0;col < cols;col++)
{
    img.push(...makePixels(pixels,col,0))
    img.push(...makePixels(pixels,col,1))
}

for(let i=0;i < img.length;i+=16)
{
    let s = img.slice(i,i+16).map(v => v ? '8' : '_').join('')
    cl(s)
}

//img = img.map(v => [0,v ? 255 : 0,0]).flat()
console.log(img.length,JSON.stringify(img))