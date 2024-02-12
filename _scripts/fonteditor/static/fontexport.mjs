
export default function fontExport(font,prefix,lang='C',order='rows',align="left")
{
	let gnum = 1;

	let symlist = [];
	let symref = [];
	let vars = {};
	for(let gl of font.glyphs)
	{
		let gvarname = prefix+'_glyph_'+gnum;
		for(let sym of gl.auto_symbols)
		{
			symlist.push('\''+sym+'\'');
			symref.push(''+gvarname);
		}
		let glcode;
		if(order="rows")
		{
			glcode = new Uint32Array(font.height + 1);
			glcode[0] = gl.width;
			for(let y=0; y < font.height; y++)
			{
				for(let x=0; x < gl.width; x++)
				{
					if(gl.image[y] && gl.image[y][x])
					{
						let align_top = align=='left' ? 31 : gl.width;
						glcode[y+1] |= 1 << (31-x);
					}
				}
			}
		}
		// else
		// {
		// 	let glcode = new Uint32Array(gl.width + 1);
		// 	for(let x=0; x < gl.width; x++)
		// 	{
		// 		for(let y=0; y < font.width; y++)
		// 		{
		// 			if(gl.image[y] && gl.image[y][x])
		// 			{
		// 				let align_top = align=='left' ? 31 : gl.width;
		// 				glcode[y+1] |= 1 << (31-x);
		// 			}
		// 		}
		// 	}
		// }
		vars[gvarname] = ['uint32_t',glcode.length,glcode];
		gnum++;
	}
	vars[prefix+'_symref'] = ['uint32_t *',symref.length,symref];
	vars[prefix+'_symlist'] = ['uint8_t',symlist.length,symlist];

	let code = '';
	for(let varname in vars)
	{
		let v = vars[varname];
		code += v[0]+' '+varname+'[] = {'+v[2].join(',')+'};\n';
	}
	return code;
}