
function clog(s,lvl=0)
{
    let colors = ["#000", "#b80", "#a00"]
    document.body.innerHTML += `<p style="color=${colors[lvl]}">${s}</p>`
}

function cwarn(s)
{
    clog(s,1)
}

function cerror(s)
{
    clog(s,2)
}