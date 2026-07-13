import WBLE from "./ble.js"

function onBLE(code, msg, level)
{
	let lvl = level=="error" ? 2 : 0
	clog(`${level.toUpperCase()}: [${code}] ${JSON.stringify(msg)}`, lvl)
}

export async function initApp() {
	try{
		clog('Cordova готова. Проверяем Bluetooth...');
		
		const wble = new WBLE()

		wble.addListener(onBLE)

		if(!await wble.init(
			"POPLAVOK",
			[0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00],
			[0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x01]
		))
			throw "Cannot connect to Bluetooth"
		
		await wble.scanUntilSuccess()
		
		let dev = wble.devices[0]
		if(await dev.connect())
		{
			// for(let i = 0;;i++)
			// {
			// 	dev.send("Hello world! "+i,"hello",1)
			// 	await delay(1000)
			// }
		}
			

	}catch(e)
	{
		cerror("Fatal error: "+e)
	}
}

