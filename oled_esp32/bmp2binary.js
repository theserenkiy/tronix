const bmp = require('bmp-js')
const fs = require('fs')


const file = './danya.bmp';

let buf = fs.readFileSync(file)

let bmpData = bmp.decode(buf)

let pixels = [...bmpData.data.filter((v,i)=>!((i+1)%4)).map(v => v > 128 ? 0 : 1)];

let bytes = []
for(let row = 0;row < 8;row++)
{
    for(let col=0;col < 128;col++)
    {
        let startPix = row*1024+col
        let pix_addrs = [0,1,2,3,4,5,6,7].map(v => v*128+startPix)
        
        let pixs = pixels.filter((v,i)=>pix_addrs.includes(i))
        let byte = 0
        for(let [i,pix] of pixs.entries())
            byte = byte | pix << i
        bytes.push(byte)
    }
}

console.log(bytes.length,JSON.stringify(bytes))