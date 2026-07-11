// UUID должны в точности соответствовать тем, что в ESP32



class BLE
{

	SERVICE_UUID = "";
	CHAR_UUID    = "";
	DEVICE_NAME = "";
	dev_id = 0
	is_connected = 0
	listeners = []
	ble = null

	async init(device_name, svc_uuid_hex_array, char_uuid_hex_array)
	{
		this.ble = window.cordova.platformId == "browser" ? ble_mock : ble
		try{
			this.SERVICE_UUID = this.mkUUID(svc_uuid_hex_array)
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

	async scanAndConnect()
	{
		if(await this.scan())
			await this.connect()
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
		return strs.toReversed().join("-")
	}


	isEnabled()
	{
		return new Promise((s,j) => this.ble.isEnabled(() => s(1), () => s(0)))
	}

	enable()
	{
		return new Promise((s,j) => this.ble.enable(() => s(1), () => s(0)))
	}


	async scan(timeout=10) {
		this.status("scan","Сканируем доступные устройства")
		let found = 0
		let to = new Promise((_,j) => setTimeout(j,timeout*1000))
		let prm = new Promise((s,j) => {
			this.ble.scan([], timeout, device => {
				// Логируем все найденные устройства для отладки
				this.status("scan_device",'Найдено устройство: ' + (device.name || 'Без имени') + ' [' + device.id + ']');
				
				// Проверяем имя устройства, которое мы жестко прописали в коде ESP32
				if (device.name === this.DEVICE_NAME) {
					// console.log('Ура! ESP32 найден по имени. Останавливаем поиск и подключаемся...');
					this.ble.stopScan();
					this.dev_id_found = device.id
					this.status("scan_device_found",`Устройство ${this.DEVICE_NAME} найдено!`)
					s(1)
					found = 1
				}
			}, function(err) {
				this.error("scan_failure",err+"")
				s(0)
			})
			
		})

		try{
			return await Promise.race([to, prm])
		}catch(e)
		{
			this.error("scan_device_not_found",`Устройство ${this.DEVICE_NAME} не найдено`)
			return 0
		}
	}

	// Подключение к ESP32
	async connect() {
		this.status("connect","Подключение к " + this.dev_id + "...");
		
		return new Promise(s => this.ble.connect(
			this.dev_id, 
			peripheral => {
				this.status("connect_ok","Устройство успешно подключено")
				this.is_connected = 1	
				s(1)
			}, 
			peripheral => {
				this.error("connect_failure",'Сбой подключения: ' + JSON.stringify(err));
				this.is_connected = 0;
				s(0)
				// setTimeout(function() {
				// 	this.status("reconnect","Попытка переподключения к устройству")
				// 	this.connect();
				// }, 3000);
			}
		))
	}


	async send(message, hint, as_text=0) {
		this.status("send", hint)
		if (!this.connectedDeviceId) {
			this.error("send_no_connection", 'Нет активного подключения');
			return 0;
		}

		const buffer = as_text ? new TextEncoder().encode(message).buffer : message

		return new Promise(s => this.ble.write(
			this.connectedDeviceId,
			this.SERVICE_UUID,
			this.CHAR_UUID,
			buffer,
			function() {
				this.status("send_ok", 'Данные успешно отправлены');
				s(1)
			},
			function(err) {
				this.error("send_failure",JSON.stringify(err));
				s(0)
			}
		))
	}
}
