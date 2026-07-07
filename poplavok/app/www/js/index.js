document.addEventListener('deviceready', onDeviceReady, false);

// UUID должны в точности соответствовать тем, что в ESP32
const SERVICE_UUID = "00112233-4455-6677-8899-aabbccddeeff";
const CHAR_UUID    = "01112233-4455-6677-8899-aabbccddeeff";

let connectedDeviceId = null;

function onDeviceReady() {
    clog('Cordova готова. Проверяем Bluetooth...');
    ensureBluetoothEnabled();
}

// Проверяем, включен ли Bluetooth (плагин сам запросит права, если нужно)
function ensureBluetoothEnabled() {
    ble.isEnabled(
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
    );
}

function startScan() {
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
    });
}

// Подключение к ESP32
function connectToDevice(deviceId) {
    clog("Подключение к " + deviceId + "...");
    ble.connect(deviceId, function(peripheral) {
        clog('Успешно подключено к ESP32!');
        connectedDeviceId = deviceId;
        
        // Отправляем тестовую строку через 2 секунды после подключения
        setTimeout(function() {
            sendData("Hello ESP32!");
        }, 2000);
        
    }, function(err) {
        cerror('Сбой подключения: ' + JSON.stringify(err));
        connectedDeviceId = null;
    });
}

// Отправка данных на ESP32
function sendData(message) {
    if (!connectedDeviceId) {
        cerror('Нет активного подключения');
        return;
    }

    const buffer = new TextEncoder().encode(message).buffer;

    ble.write(
        connectedDeviceId,
        SERVICE_UUID,
        CHAR_UUID,
        buffer,
        function() {
            clog('Данные успешно отправлены: ' + message);
        },
        function(err) {
            cerror('Ошибка отправки данных: ' + JSON.stringify(err));
        }
    );
}
