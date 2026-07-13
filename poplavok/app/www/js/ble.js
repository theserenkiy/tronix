// UUID должны в точности соответствовать тем, что в ESP32

if(window.cordova.platformId == "browser")
	ble = ble_mock

class BLEDevice
{
	constructor(device, wble)
	{
		this.wble = wble
		for(let k in device)
		{
			this[k] = device[k]
		}

		this.is_connected = 0

		this.reconnect_timeout = 0;
	}

	status(code, msg,level="info")
	{
		this.wble.status(code, {dev_id: this.id, msg}, level)
	}

	error(code, msg)
	{
		this.status(code, msg, "error")
	}

	// Подключение к ESP32
	async connect(auto_reconnect=1) {
		this.status("connect","Подключение к " + this.id + "...");
		
		return new Promise(s => ble.connect(
			this.id, 
			peripheral => {
				this.status("connect_ok","Устройство успешно подключено")
				this.is_connected = 1	
				this.reconnect_timeout = 0
				this.subscribeToData()
				s(1)
			}, 
			peripheral => {
				this.error("connect_error",'Сбой подключения: ' + JSON.stringify(err));
				this.is_connected = 0;
				if(auto_reconnect)
					this.reconnect()
				s(0)
			}
		))
	}

	async reconnect()
	{
		this.status("reconnect")
		if(this.reconnect_timeout)
		{
			for(let i=this.reconnect_timeout;i > 0; i++)
			{
				this.status("reconnect_in",i)
				await delay(1000)
			}
		}
		
		this.reconnect_timeout += 2

		await this.connect(1)
	}


	async send(message, hint, as_text=0) {
		this.status("send", hint)
		if (!this.is_connected) {
			this.error("send_no_connection", 'Нет активного подключения'); 
			return 0;
		}

		if(!await this.wble.checkEnabled())
			return 0

		const buffer = as_text ? new TextEncoder().encode(message).buffer : message

		return new Promise(s => ble.write(
			this.id,
			this.wble.SERVICE_UUID,
			this.wble.CHAR_UUID,
			buffer,
			function() {
				this.status("send_ok", 'Данные успешно отправлены');
				s(1)
			},
			function(err) {
				this.error("send_error",JSON.stringify(err));
				s(0)
			}
		))
	}

	async subscribeToData() {
		ble.startNotification(this.id, this.wble.SERVICE_UUID, this.wble.CHAR_UUID, 
			function(buffer) {
				// Этот коллбек вызывается КАЖДЫЙ РАЗ, когда устройство шлет данные
				let dataString = bytesToString(buffer);
				console.log("Получены новые данные: " + dataString);
				
				// Обновляем UI
				this.status("received_data",dataString);
			}, 
			function(error) {
				this.error("subscribe_error",error+"")
			}
		);
	}
}

class WBLE
{
	SERVICE_UUID = "";
	CHAR_UUID    = "";
	DEVICE_NAME = "";
	dev_id = 0
	is_connected = 0
	listeners = []

	devices = []

	async init(device_name, svc_uuid_hex_array, char_uuid_hex_array)
	{
		try{
			this.SERVICE_UUID = this.mkUUID(svc_uuid_hex_array)
			clog("SERVICE_UUID: "+this.SERVICE_UUID)
			this.CHAR_UUID = this.mkUUID(char_uuid_hex_array)
			this.DEVICE_NAME = device_name

			if(!await this.isEnabled())
			{
				this.status("init","Подключаем Bluetooth")
				if(!await this.enable())
					throw ["cannot_enable","Не удалось включить Bluetooth"]
			}
			return 1
		}
		catch(e)
		{
			this.status(e[0], e[1], "error")
			return 0
		}
	}

	

	addListener(callback)
	{
		this.listeners.push(callback)
	}

	status(code, msg,level="info")
	{
		this.listeners.map(v => v(code, msg, level))
	}

	error(code, msg)
	{
		this.status(code, msg, "error")
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

	async checkEnabled()
	{
		if(await this.isEnabled())
			return 1

		this.error("not_enabled","Bluetooth не включён")
	}

	isEnabled()
	{
		return new Promise((s,j) => ble.isEnabled(() => s(1), () => s(0)))
	}

	enable()
	{
		return new Promise((s,j) => ble.enable(() => s(1), () => s(0)))
	}


	async scan(timeout=10, stop_when_found=1) {
		this.status("scan","Сканируем доступные устройства")
		let devices = []
		let to = new Promise((_,j) => setTimeout(() => j("timeout"),timeout*1000))
		let prm = new Promise((s,j) => {
			ble.scan([], timeout, device => {
				// Логируем все найденные устройства для отладки
				this.status("scan_device_found",device);
				if(device.name !== this.DEVICE_NAME)
					return

				if(this.devices.find(v => v.id == device.id))
				{
					this.status("scan_device_exists",device.id);
				}
				else
				{
					let dev = new BLEDevice(device, this)
					devices.push(dev)
					if(stop_when_found)
						ble.stopScan();
				}

			}, function(err) {
				this.error("scan_error",err+"")
				s(0)
			})
			
		})

		try{
			await Promise.race([to, prm])
		}catch(e)
		{
			if(e == "timeout")
			{
				if(devices.length)
				{
					this.devices = [...this.devices, ...devices]
					this.status("scan_completed",devices.map(d => d.id))
					return 1
				}
				// this.error("scan_devices_not_found",`Устройство ${this.DEVICE_NAME} не найдено`)
			}
			else
			{
				this.error("scan_error",e+"")
			}
			
			return 0
		}
	}

	

}
