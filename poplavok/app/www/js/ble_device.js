

const MAX_CONNECT_ATTEMPTS = 3

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
		this.connect_attempts = MAX_CONNECT_ATTEMPTS
	}

	status(code, msg,level="info")
	{
		return this.wble.status(code, {dev_id: this.id, msg}, level)
	}

	error(code, msg)
	{
		return this.status(code, msg, "error")
	}

	// Подключение к ESP32
	async connect(auto_reconnect=0) {
		this.status("connect","Подключение к " + this.id + "...");
		
		return new Promise(s => ble.connect(
			this.id, 
			peripheral => {
				this.is_connected = 1	
				this.reconnect_timeout = 0
				this.connect_attempts = MAX_CONNECT_ATTEMPTS
				this.subscribeToData()
				s(this.status("connect.ok","Устройство успешно подключено"))
			}, 
			err => {
				if(this.is_connected)
				{
					this.is_connected = 0;
					if(this.connect_attempts)
					{
						this.connect_attempts--
						this.reconnect()
						s(this.status("connect.reconneting","Попытка подключения","warning"))
						return
					}
				}
				
				s(this.error("connect.error",'Сбой подключения: ' + JSON.stringify(err)))
			}
		))
	}

	async reconnect()
	{
		this.status("reconnect")
		if(this.reconnect_timeout)
		{
			for(let i=this.reconnect_timeout; i > 0; i++)
			{
				this.status("reconnect.in", i)
				await delay(1000)
			}
		}
		
		this.reconnect_timeout += 2

		await this.connect(1)
	}


	async send(message, hint, as_text=0) {
		this.status("send", hint)
		if (!this.is_connected) {
			this.error("send.no_connection", 'Нет активного подключения'); 
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
				this.status("send.ok", 'Данные успешно отправлены');
				s(1)
			},
			err => {
				this.error("send.error",JSON.stringify(err));
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
				this.status("data.received",dataString);
			}, 
			error => {
				this.error("subscribe.error",error+"")
			}
		);
	}
}