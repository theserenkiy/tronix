import { pack } from "./pack.js";
// Проверяем, запущены ли мы в браузере без плагина


let notif_started = 0

const devices = [
	{
		name: "Эмулированное Устройство",
		id: "AA:BB:CC:DD:EE:FF",
		rssi: -65
	},{
		name: "POPLAVOK",
		id: "AA:BB:CC:DD:EE:FF",
		rssi: -65
	}
]

let connect_failure;
let once_connected = 0

export default {
	scan: async function(services, seconds, success, failure) {
		console.log(`[Mock BLE] Старт сканирования на ${seconds} сек...`);

		for(let dev of devices)
		{
			await delay(500)
			success(dev)
		}
		
	},
	stopScan: function(success, failure) {
		console.log("[Mock BLE] Сканирование остановлено.");
		if(success) success();
	},
	connect: function(deviceId, success, failure) {
		console.log(`[Mock BLE] Подключение к ${deviceId}...`);
		connect_failure = failure
		let fail = 0
			// once_connected || 
			// Math.random() < 0.5

		// if(!fail)
		// {
		// 	setTimeout(() => connect_failure("а вот так захотелось!"),3000)
		// }

		setTimeout(() => {
			// Возвращаем тестовую структуру периферийного устройства
			if(!fail)
			{
				once_connected = 1
				success({
					name: "Эмулированное Устройство",
					id: deviceId,
					services: ["180d"],
					characteristics: ["2a37"]
				});
			}
			else
				failure("Просто такая ошибка")
			
		}, 500);
	},
	read: function(deviceId, serviceUUID, characteristicUUID, success, failure) {
		// Возвращаем фейковый ArrayBuffer с данными (например, число 42)
		let buffer = new Uint8Array([42]).buffer;
		success(buffer);
	},
	disconnect: function(deviceId, success, failure) {
		console.log("[Mock BLE] Отключено.");
		if(success) success();
	},
	isEnabled: (ok, err) => {
		ok()
	},
	enable: (ok, err) => {
		if(1 || confirm("Ok to use BT?"))
			ok()
		else
			err()
	},
	startNotification: async (devid, svc, char, onData, err) => {
		if(notif_started)
			return
		notif_started = 1
		while(1)
		{
			let buf = pack([
				["uint8",[1]],
				["uint8",[Math.floor(Math.random()*3)]]
			])
			// cl("BUFLEN",buf.byteLength)
			cl("ONDATA")
			onData(buf)
			await delay(2000)
		}
	},
	readRSSI: (devid, ok, fail) =>
	{
		ok(-Math.random()*60-40)
	},
	requestMtu(devid, mtu, ok, fail)
	{
		ok(256)
	},
	write(devid, svc, char, buf, ok, err)
	{
		ok()
	}

}
