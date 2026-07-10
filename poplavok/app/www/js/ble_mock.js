
// Проверяем, запущены ли мы в браузере без плагина
if (!window.ble) {
    console.warn("Внимание: Найдена среда браузера. Подключаем BLE-заглушку.");
    
    window.ble = {
        scan: function(services, seconds, success, failure) {
            console.log(`[Mock BLE] Старт сканирования на ${seconds} сек...`);
            
            // Имитируем, что через 1 секунду нашли устройство
            setTimeout(() => {
                success({
                    name: "Эмулированное Устройство",
                    id: "AA:BB:CC:DD:EE:FF",
                    rssi: -65
                });
            }, 1000);
        },
        stopScan: function(success, failure) {
            console.log("[Mock BLE] Сканирование остановлено.");
            if(success) success();
        },
        connect: function(deviceId, success, failure) {
            console.log(`[Mock BLE] Подключение к ${deviceId}...`);
            setTimeout(() => {
                // Возвращаем тестовую структуру периферийного устройства
                success({
                    name: "Эмулированное Устройство",
                    id: deviceId,
                    services: ["180d"],
                    characteristics: ["2a37"]
                });
            }, 1500);
        },
        read: function(deviceId, serviceUUID, characteristicUUID, success, failure) {
            // Возвращаем фейковый ArrayBuffer с данными (например, число 42)
            let buffer = new Uint8Array([42]).buffer;
            success(buffer);
        },
        disconnect: function(deviceId, success, failure) {
            console.log("[Mock BLE] Отключено.");
            if(success) success();
        }
    };
}
