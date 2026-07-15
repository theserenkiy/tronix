

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
		this.connect_state = "initial"
		this.running = 1
		this.rssi = -100
		this.mtu = 20

		this.poll()
	}

	async stop()
	{
		this.running = 0
		cl("STOP")
		cl("CONNECTED:",this.is_connected)
		if(this.is_connected)
			await this.disconnect()
	}

	async disconnect()
	{
		cl("DISCONNECTING...")
		return new Promise(s => {
			ble.disconnect(this.id,() => s(1), err => {
				this.error("disconnect.error",err)
			})
		})
	}

	async poll()
	{
		while(1)
		{
			if(!this.running)
				break

			await this.updateRSSI()

			await delay(1000)
		}
	}

	async updateRSSI()
	{
		ble.readRSSI(
			this.id,
			rssi => this.rssi = rssi,
			() => {console.error("Cannot read RSSI")}
		)
	}

	status(code, msg,level="info")
	{
		return this.wble.status(code, {dev_id: this.id, msg}, level)
	}

	error(code, msg)
	{
		return this.status(code, msg, "error")
	}

	connect_failure(err)
	{
		try{
			if(this.connect_state == "initial")
				throw err

			if(this.is_connected)
			{
				this.connect_state = "reconnect"
				this.connect_attempts = 0
				this.is_connected = 0
			}

			if(this.connect_state == "reconnect")
			{
				if(this.connect_attempts >= MAX_CONNECT_ATTEMPTS)
				{
					this.connect_state = "initial"
					throw err
				}
				this.connect_attempts++
				let ret = this.status("connect.lost", err, "warn")
				this.status("connect.retry",`Связь потеряна. Попытка подключения ${this.connect_attempts} из ${MAX_CONNECT_ATTEMPTS}`)
				this.reconnect()
				return ret
			}
		}
		catch(err)
		{
			this.is_connected = 0;
			return this.error("connect.error",'Сбой подключения: ' + JSON.stringify(err))
		}
	}

	// Подключение к ESP32
	async connect(auto_reconnect=0) {
		this.status("connect","Подключение к " + this.id + "...");
		
		return new Promise(s => ble.connect(
			this.id, 
			peripheral => {
				this.onSuccessConnect()
				s(1)
			}, 
			err => {
				if(!this.running)
					return
				s(this.connect_failure(err))
			}
		))
	}

	async onSuccessConnect()
	{
		if(!this.running)
		{
			await this.disconnect()
			return
		}
		this.is_connected = 1
		this.connect_state = "ok"
		this.reconnect_timeout = 0
		this.connect_attempts = 0
		await this.requestMTU()
		await this.subscribeToData()
		this.status("connect.ok","Устройство успешно подключено")
	}

	async requestMTU()
	{
		return new Promise(s => ble.requestMtu(this.id, 
			512, 
			mtu => {
            	console.log('MTU set: ' + mtu);
				this.mtu = mtu-3
				s(1)
        	}, err => {
            	console.error('Не удалось изменить MTU: ' + JSON.stringify(err));
				s(0)
        	}
		));
	}

	async reconnect()
	{
		if(!this.running)
			return
		this.status("reconnect")
		if(this.reconnect_timeout)
		{
			for(let i=this.reconnect_timeout; i > 0; i--)
			{
				this.status("reconnect.in", i)
				await delay(1000)
			}
		}
		
		this.reconnect_timeout += 2

		await this.connect(1)
	}


	async send(buffer, hint, as_text=0) {
		this.status("send", hint)
		if (!this.is_connected) {
			this.error("send.no_connection", 'Нет активного подключения'); 
			return 0;
		}

		if(!this.wble.bt_enabled)
		{
			cl("Cannot send: BT disabled")
			return 0
		}

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