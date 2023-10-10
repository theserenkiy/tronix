
const cl = console.log

function hsl2rgb(h, s, l){
	s /= 100;
	l /= 100;
	const k = n => (n + h / 30) % 12;
	const a = s * Math.min(l, 1 - l);
	const f = n =>
		l - a * Math.max(-1, Math.min(k(n) - 3, Math.min(9 - k(n), 1)));
	return [Math.floor(255 * f(0)), Math.floor(255 * f(8)), Math.floor(255 * f(4))];
};

function cssColor(color){           
	return '#'+color.map(v => v.toString(16).padStart(2,'0')).join('')
}

function debounce(func, timeout = 300)
{
	let timer;
	return (...args) => {
		clearTimeout(timer);
		timer = setTimeout(() => { func.apply(this, args); }, timeout);
	};
}
