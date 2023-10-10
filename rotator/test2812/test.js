const fs = require('fs')

console.log((fs.readFileSync('image.txt')+'').split(/[\s\,]+/).map(v => +v))