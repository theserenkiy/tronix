import BLEDevice from "./ble_device.js"
import ble_mock from "./ble_mock.js"

if(window.cordova.platformId == "browser")
	window.ble = ble_mock


export default class WBLE
{
	SERVICE_UUID = "";
	CHAR_UUID    = "";
	DEVICE_NAME = "";
	dev_id = 0
	is_connected = 0
	bt_enabled = 0
	listeners = []

	devices = []

	async init(device_name, svc_uuid_hex_array, char_uuid_hex_array)
	{
		try{
			this.SERVICE_UUID = this.mkUUID(svc_uuid_hex_array)
			clog("SERVICE_UUID: "+this.SERVICE_UUID)
			this.CHAR_UUID = this.mkUUID(char_uuid_hex_array)
			this.DEVICE_NAME = device_name

			this.running = 1

			if(!await this.isEnabled())
			{
				this.status("bt.init","Подключаем Bluetooth")
				if(!await this.enable())
					return this.error("bt.cannot_enable","Не удалось включить Bluetooth")
				this.bt_enabled = 1
				cl("ENABLED")
			}
			this.pollBTEnable()
			return this.status("bt.ok","Bluetooth включён")
		}
		catch(e)
		{
			return this.error("bt.error",e+"")
		}
	}

	async stop()
	{
		this.running = 0
		for(let dev of this.devices)
		{
			await dev.stop()
		}
		this.devices = []
		
	}

	async pollBTEnable()
	{
		while(1)
		{
			// cl("POLL BT, running:",this.running)
			if(!this.running)
				break
			let res = await this.isEnabled()
			// cl("RES",res)
			if(!res && this.bt_enabled)
				this.error("bt.not_enabled","Bluetooth не включён")
			this.bt_enabled = res
			await delay(1000)
		}
	}

	addListener(callback)
	{
		this.listeners.push(callback)
	}

	status(code, msg, level="log")
	{
		this.listeners.map(v => v(code, msg, level))
		return {code, msg, level}
	}

	error(code, msg)
	{
		return this.status(code, msg, "error")
	}

	mkUUID(hex_array)
	{
		let strs = []
		for(let ind of [[0,6],[6,8],[8,10],[10,12],[12,16]])
		{
			strs.push(hex_array.slice(ind[0],ind[1]).toReversed().map(v => v.toString(16)).join(""))
		}
		return strs.toReversed().join("-").toLowerCase()
	}


	isEnabled()
	{
		return new Promise((s,j) => ble.isEnabled(() => s(1), () => s(0)))
	}

	enable()
	{
		return new Promise((s,j) => ble.enable(() => s(1), () => s(0)))
	}

	// async scanUntilSuccess(timeout=60)
	// {
	// 	for(let i=0;i < ;i++)
	// 	{
	// 		this.status("scan_attempt",i+"")
	// 		if(await this.scan(5,1))
	// 			break
	// 		await delay(1000)
	// 	}
	// }


	async scan(timeout=10, stop_when_found=1) {
		this.status("scan","Сканируем доступные устройства")
		let devices = []
		let to = new Promise((_,j) => setTimeout(() => j("timeout"),timeout*1000))
		let prm = new Promise((s,j) => {
			ble.scan([], timeout, device => {
				// Логируем все найденные устройства для отладки
				this.status("scan.device_found",device);
				if(device.name !== this.DEVICE_NAME)
					return

				if(this.devices.find(v => v.id == device.id))
				{
					this.status("scan.device_exists",device.id);
				}
				else
				{
					let dev = new BLEDevice(device, this)
					devices.push(dev)
					if(stop_when_found)
					{
						ble.stopScan();
						s(1)
					}
				}

			}, err => {
				// this.error("scan_error",err+"")
				j(err)
			})
			
		})

		try{
			await Promise.race([to, prm])
			cl("Race completed")
		}catch(e)
		{
			if(e != "timeout")
			{
				return this.error("scan.error",e+"")
			}
		}

		if(devices.length)
		{
			this.devices = [...this.devices, ...devices]
			return this.status("scan.ok",devices.map(d => d.id))
		}
		return this.error("scan.devices_not_found",`Устройство ${this.DEVICE_NAME} не найдено`)
	}

	async disconnectDevice(dev)
	{
		if(!dev)
			return
		await dev.stop()
		this.devices = this.devices.filter(d => d.id != dev.id)
	}

}
