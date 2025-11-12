# API Documentation

API CRSF-IO-mkII для управления и получения телеметрии

## Endpoints

### Телеметрия

**GET** `/api/telemetry`

Получить все данные телеметрии в реальном времени.

**Ответ:**
```json
{
  "linkUp": true,
  "activePort": "UART Active",
  "lastReceive": 123456,
  "timestamp": "12:34:56.789",
  "channels": [1500, 1500, 1000, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500],
  "packetsReceived": 1234,
  "packetsSent": 5678,
  "packetsLost": 0,
  "gps": {
    "latitude": 55.7558,
    "longitude": 37.6173,
    "altitude": 150.5,
    "speed": 25.3
  },
  "battery": {
    "voltage": 12.6,
    "current": 1500,
    "capacity": 5000,
    "remaining": 85
  },
  "attitude": {
    "roll": -2.5,
    "pitch": 1.2,
    "yaw": 45.0
  },
  "attitudeRaw": {
    "roll": -437,
    "pitch": 210,
    "yaw": 7875
  },
  "workMode": "joystick"
}
```

**Пример использования:**
```bash
curl http://localhost:8081/api/telemetry
```

### Команды

**GET** `/api/command?cmd=<команда>&value=<значение>`

Выполнить команду управления.

#### Доступные команды:

##### 1. Переключение режима

```bash
# Режим джойстика
curl "http://localhost:8081/api/command?cmd=setMode&value=joystick"

# Ручной режим
curl "http://localhost:8081/api/command?cmd=setMode&value=manual"
```

##### 2. Установка канала (только в ручном режиме)

Формат: `номер=значение`

```bash
# Канал 1 = 1500 (центр)
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1500"

# Канал 2 = 1200 (минимум)
curl "http://localhost:8081/api/command?cmd=setChannel&value=2=1200"

# Канал 3 = 1800 (максимум)
curl "http://localhost:8081/api/command?cmd=setChannel&value=3=1800"

# Канал 4 (Yaw) влево
curl "http://localhost:8081/api/command?cmd=setChannel&value=4=1300"
```

**Диапазон значений:** 1000 - 2000

## RC Каналы

| Канал | Описание | Диапазон |
|-------|----------|----------|
| 1 | Roll | 1000-2000 |
| 2 | Pitch | 1000-2000 |
| 3 | Throttle | 1000-2000 |
| 4 | Yaw | 1000-2000 |
| 5-16 | Вспомогательные | 1000-2000 |

**Значения:**
- `1000` - Минимум/Влево/Вниз
- `1500` - Центр/Нейтраль
- `2000` - Максимум/Вправо/Вверх

## Режимы работы

### Joystick Mode

Управление через подключенный джойстик:
- Ось 0 → Roll (Канал 1)
- Ось 1 → Pitch (Канал 2)
- Ось 2 → Throttle (Канал 3)
- Ось 3 → Yaw (Канал 4)

### Manual Mode

Управление через API команды. Каналы устанавливаются программно.

## Примеры использования

### Python

```python
import requests

# Получить телеметрию
response = requests.get('http://localhost:8081/api/telemetry')
data = response.json()
print(f"Roll: {data['attitude']['roll']}°")
print(f"Yaw: {data['attitude']['yaw']}°")

# Переключиться в ручной режим
requests.get('http://localhost:8081/api/command?cmd=setMode&value=manual')

# Установить канал
requests.get('http://localhost:8081/api/command?cmd=setChannel&value=1=1500')
```

### Bash Script

```bash
#!/bin/bash

# Переключиться в ручной режим
curl "http://localhost:8081/api/command?cmd=setMode&value=manual"

# Установить все каналы в центр
for i in {1..16}; do
  curl "http://localhost:8081/api/command?cmd=setChannel&value=$i=1500"
  sleep 0.1
done
```

## Частота обновления

- Телеметрия: Каждые 10 мс
- Команды: Мгновенно
- RC-каналы отправка: 100 Гц (каждые 10 мс)

## CORS

API поддерживает CORS для веб-интеграции:

```
Access-Control-Allow-Origin: *
```

## Формат данных

### Attitude

Углы наклона коптера:
- **Roll**: Крен (-180° до +180°)
- **Pitch**: Тангаж (-180° до +180°)
- **Yaw**: Рысканье (0° до 360°)

### AttitudeRaw

Сырые значения из CRSF пакета (int16_t):
- Используются для отладки
- Коэффициент конвертации: 175.0

### GPS

- **Latitude**: Широта (градусы)
- **Longitude**: Долгота (градусы)
- **Altitude**: Высота (метры, относительно земли)
- **Speed**: Скорость (км/ч)

### Battery

- **Voltage**: Напряжение (В)
- **Current**: Ток (мА)
- **Capacity**: Емкость (мАч)
- **Remaining**: Остаток (%)

### Channels

Массив из 16 значений, каждое в диапазоне 1000-2000 (микросекунды PWM).
