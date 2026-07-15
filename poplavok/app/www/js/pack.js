// [
// 	["uint8",[123]],
// 	["str","ABCDEFG",16],
// 	["int",[12,23]]
// ]
export function pack(struct)
{
	// cl("PACK",struct)
	const type_lengths = {
		int: 4,
		str: 1,
		uint8: 1,
		raw: 1
	}
	let totalLen = 0;
	
	for(let fld of struct)
	{
		totalLen += type_lengths[fld[0]] * (fld[2] || fld[1].length)
	}

	// console.log("total len", totalLen)
	let out = new Uint8Array(totalLen);
	let view = new DataView(out.buffer)

	let offs = 0;
	let data, type;
	for(let fld of struct)
	{
		type = fld[0]
		data = fld[1]
		if(type == "int")
		{
			for (let i = 0; i < data.length; i++) {
				view.setInt32(offs, data[i], true)
				offs += 4
			}
		}
		else if(type == "uint8")
		{
			for (let i = 0; i < data.length; i++) {
				view.setUint8(offs, data[i])
				offs += 1
			}
		}
		else if(type == "str")
		{
			const sview = new Uint8Array(view.buffer, offs, fld[2])
			sview.fill(0)
			new TextEncoder().encodeInto(data,sview.subarray(0,fld[2]-1))

			// const stringBuf = new TextEncoder().encode(data);
			// if(stringBuf.byteLength >= fld[2])
			// 	stringBuf = stringBuf.
			// out.set(sview.buffer, offs)
			offs += fld[2]
		}
		else if(type == "raw")
		{
			out.set(data, offs)
			offs += data.byteLength
		}
	}

	console.log("OUT",out)
	return out.buffer
}

//[["int",1,"id"], ["uint8",4,"data"], ...]
export function unpack(buf,struct)
{
	let out = {}
	let view = new DataView(buf)
	let offs = 0;
	for(let f of struct)
	{
		let d = []
		switch(f[0])
		{
			case "int":
				for(let i=0; i < f[1]; i++)
				{
					d.push(view.getInt32(offs,true))
					offs += 4
				}
				break
			
			case "uint8":
				for(let i=0; i < f[1]; i++)
				{
					d.push(view.getUint8(offs,true))
					offs += 1
				}
				break
			
			case "str":
				let sview = new Uint8Array(buf, offs, f[1])
				const nullIndex = sview.indexOf(0);
				d = new TextDecoder('utf-8')
					.decode(nullIndex !== -1 
						? sview.subarray(0, nullIndex) 
						: sview
					)
				offs += f[1]
				break
			case "raw":
				d = view.subarray(offs,offs+f[1]).buffer
				break
		}
		out[f[2]] = d
	}
	return out
}