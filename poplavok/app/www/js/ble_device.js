

export default class BLEDevice
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
			err => {
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

		if(!await this.wble.bt_enabled())
			return 0

		const buffer = as_text ? new TextEncoder().encode(message).buffer : message

		return new Promise(s => ble.write(
			this.id,
			this.wble.SERVICE_UUID,
			this.wble.CHAR_UUID,
			buffer,
			() => {
				this.status("send_ok", 'Данные успешно отправлены');
				s(1)
			},
			err => {
				this.error("send_error",JSON.stringify(err));
				s(0)
			}
		))
	}

	async subscribeToData() {
		this.status("subscribe")
		ble.startNotification(
			this.id, 
			this.wble.SERVICE_UUID, 
			this.wble.CHAR_UUID, 
			buffer => {
				let dataString = new TextDecoder().decode(buffer);
				console.log("Получены новые данные: " + dataString);
				
				// Обновляем UI
				this.status("received_data",dataString);
			}, 
			error => {
				this.error("subscribe_error",error+"")
			}
		);
	}
}