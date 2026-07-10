// UUID должны в точности соответствовать тем, что в ESP32



class BLE
{

	SERVICE_UUID = "";
	CHAR_UUID    = "";
	connectedDeviceId = null;

	init(svc_uuid_hex_array, char_uuid_hex_array)
	{
		this.SERVICE_UUID = this.mkUUID(svc_uuid_hex_array)
		this.CHAR_UUID = this.mkUUID(char_uuid_hex_array)

		this.ensureBluetoothEnabled()
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
		return new Promise((s,j) => ble.isEnabled(() => s(1), () => s(0)))
	}

	enable()
	{
		return new Promise((s,j) => ble.enable(() => s(1), () => s(0)))
	}


	// Проверяем, включен ли Bluetooth (плагин сам запросит права, если нужно)
	async ensureBluetoothEnabled() {
		return await new Promise((s,j)=> ble.isEnabled(
				function() {
					clog("Bluetooth активен и доступен. Запускаем поиск...");
					startScan();
				},
				function() {
					clog("Bluetooth выключен или нет разрешений. Запрашиваем включение...");
					// Этот метод принудительно вызывает системное окно включения BT и запроса прав на Android
					ble.enable(
						function() {
							clog("Bluetooth успешно включен пользователем.");
							startScan();
						},
						function(err) {
							cerror("Пользователь отказался включить Bluetooth или дать права: " + err);
						}
					);
				}
			)
		)
	}

	startScan() {
		console.log("Сканирование всех BLE устройств начато...");
		
		// Передаем пустой массив [], чтобы сканировать всё вокруг
		ble.scan([], 10, function(device) {
			// Логируем все найденные устройства для отладки
			console.log('Найдено устройство: ' + (device.name || 'Без имени') + ' [' + device.id + ']');
			
			// Проверяем имя устройства, которое мы жестко прописали в коде ESP32
			if (device.name === "ESP32_GATT_SERVER") {
				console.log('Ура! ESP32 найден по имени. Останавливаем поиск и подключаемся...');
				ble.stopScan();
				connectToDevice(device.id);
			}
		}, function(err) {
			console.error('Ошибка сканирования: ' + JSON.stringify(err));
		})
	}

	// Подключение к ESP32
	connectToDevice(deviceId) {
		clog("Подключение к " + deviceId + "...");
		ble.connect(
			deviceId, 
			peripheral => {
				clog('Успешно подключено к ESP32!');
				this.connectedDeviceId = deviceId;
				
				// Отправляем тестовую строку через 2 секунды после подключения
				setTimeout(function() {
					sendData("Hello ESP32!");
				}, 2000);
				
			}, 
			peripheral => {
				cerror('Сбой подключения: ' + JSON.stringify(err));
				this.connectedDeviceId = null;
				setTimeout(function() {
					this.connectToDevice(deviceId);
				}, 3000);
			}
		);
	}

	// Отправка данных на ESP32
	sendData(message) {
		if (!this.connectedDeviceId) {
			cerror('Нет активного подключения');
			return;
		}

		const buffer = new TextEncoder().encode(message).buffer;

		ble.write(
			this.connectedDeviceId,
			this.SERVICE_UUID,
			this.CHAR_UUID,
			buffer,
			function() {
				clog('Данные успешно отправлены: ' + message);
			},
			function(err) {
				cerror('Ошибка отправки данных: ' + JSON.stringify(err));
			}
		);
	}
}
