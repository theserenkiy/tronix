
const cl = console.log;

async function api(url,data,onerror)
{
	let d;
	try{
		let res = await fetch(url,{
			method: 'POST',
			headers: {
				'Content-Type': 'application/json;charset=utf-8'
			},
			body: JSON.stringify(data)
		});

		d = await res.json()
		if(!d.ok)
			throw d.error || "Неизвестная ошибка"
	}catch(e)
	{
		if(onerror)
			onerror(e)
		else 
			alert("Server error: "+e)
	}
	return d;
}

function qs(q,c)
{
	return (c || document).querySelector(q)
}

function qsa(q,c)
{
	return [...(c || document).querySelectorAll(q)]
}

function on(el,ev,lst)
{
	el.addEventListener(ev,lst)
}

function debounce(func, wait=100) {
	let timeout;

	return function(...args) {
		const context = this; // Preserve 'this' context

		const later = function() {
			timeout = null;
			func.apply(context, args); // Execute the original function
		};

		clearTimeout(timeout); // Clear any existing timer
		timeout = setTimeout(later, wait); // Set a new timer
	};
}

function delay(ms)
{
	return new Promise(s => setTimeout(s,ms))
}

function togClass(el,cls,state)
{
	let c = (" "+el.className+" ")
	c = c.split(" "+cls+" ").join(" ")
	if(state)
		c += cls
		
	el.className = c.trim()
}


function hslToRgb(h, s, l) {
  let r, g, b;

  if (s === 0) {
    r = g = b = l; // achromatic
  } else {
    const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    const p = 2 * l - q;
    r = hueToRgb(p, q, h + 1/3);
    g = hueToRgb(p, q, h);
    b = hueToRgb(p, q, h - 1/3);
  }

  return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
}

function hueToRgb(p, q, t) {
  if (t < 0) t += 1;
  if (t > 1) t -= 1;
  if (t < 1/6) return p + (q - p) * 6 * t;
  if (t < 1/2) return q;
  if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
  return p;
}