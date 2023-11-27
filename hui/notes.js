let base_freq = 7000
let prescaler = 64
let divs = [1047,1175,1319,1397,1568,1760,1976,2093].map(v=>Math.round((v/1047)*base_freq))
let values = divs.map(v=>`FOSC/${prescaler}/${v}`)
let s = `.db ${values.join(', ')}\n.db ${values.reverse().join(', ')}`
console.log(s)
