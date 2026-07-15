
export default function(struct)
{
	cl("PACK",struct)
	const type_lengths = {
		int: 4,
		str: 1,
		uint8: 1,
		raw: 1
	}
	let totalLen = 0;
	
	for(let fld of struct)
	{
		totalLen += type_lengths[fld[0]] * fld[1].length
	}

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
				view.setInt32(offs, data[i], true); 
				offs += 4
			}
		}
		else if(type == "uint8")
		{
			for (let i = 0; i < data.length; i++) {
				view.setUint8(offs, data[i]); 
				offs += 1
			}
		}
		else if(type == "str")
		{
			const stringBuf = new TextEncoder().encode(data);

			out.set(stringBuf, offs);
			offs += stringBuf.byteLength
		}
		else if(type == "raw")
		{
			out.set(data, offs)
			offs += data.byteLength
		}
	}

	return out.buffer
}