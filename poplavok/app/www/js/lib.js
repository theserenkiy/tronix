
const cl = console.log

function delay(ms)
{
    return new Promise(s => setTimeout(s,ms))
}

function clog(s,level="log")
{
    // let colors = ["#000", "#b80", "#a00"]
    // document.body.innerHTML += `<p style="color:${colors[lvl]}">${s}</p>`
    // cl("LEVEL",level)
    console[level](s)
}
