# Manual Mode Guide

Руководство по использованию ручного режима управления

## Что такое Manual Mode

Ручной режим позволяет программно управлять RC-каналами через API, минуя джойстик. Это полезно для:
- Автоматизации полетов
- Тестирования
- Интеграции с другими системами
- Сценариев управления

## Переключение режимов

### Режим джойстика (по умолчанию)

```bash
curl "http://localhost:8081/api/command?cmd=setMode&value=joystick"
```

В этом режиме каналы управляются джойстиком.

### Ручной режим

```bash
curl "http://localhost:8081/api/command?cmd=setMode&value=manual"
```

В этом режиме каналы управляются через API команды.

## Управление каналами

### Формат команды

```
http://localhost:8081/api/command?cmd=setChannel&value=номер=значение
```

Где:
- `номер` - номер канала (1-16)
- `значение` - значение в диапазоне 1000-2000

### RC Каналы

| Канал | Описание | Диапазон | Центр |
|-------|----------|----------|-------|
| 1 | Roll | 1000-2000 | 1500 |
| 2 | Pitch | 1000-2000 | 1500 |
| 3 | Throttle | 1000-2000 | 1500 |
| 4 | Yaw | 1000-2000 | 1500 |
| 5-16 | Aux | 1000-2000 | 1500 |

### Примеры

#### Установка канала в центр

```bash
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1500"
```

#### Roll влево

```bash
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1200"
```

#### Roll вправо

```bash
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1800"
```

#### Pitch вниз

```bash
curl "http://localhost:8081/api/command?cmd=setChannel&value=2=1200"
```

#### Pitch вверх

```bash
curl "http://localhost:8081/api/command?cmd=setChannel&value=2=1800"
```

#### Throttle минимум

```bash
curl "http://localhost:8081/api/command?cmd=setChannel&value=3=1000"
```

#### Throttle максимум

```bash
curl "http://localhost:8081/api/command?cmd=setChannel&value=3=2000"
```

#### Yaw влево

```bash
curl "http://localhost:8081/api/command?cmd=setChannel&value=4=1300"
```

#### Yaw вправо

```bash
curl "http://localhost:8081/api/command?cmd=setChannel&value=4=1700"
```

## Типичные сценарии

### 1. Возврат всех каналов в центр

```bash
curl "http://localhost:8081/api/command?cmd=setMode&value=manual"
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1500"
curl "http://localhost:8081/api/command?cmd=setChannel&value=2=1500"
curl "http://localhost:8081/api/command?cmd=setChannel&value=3=1000"
curl "http://localhost:8081/api/command?cmd=setChannel&value=4=1500"
```

### 2. Последовательность движений

```bash
# Право
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1800"
sleep 2

# Центр
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1500"
sleep 1

# Влево
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1200"
sleep 2

# Центр
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1500"
```

### 3. Программное управление (Python)

```python
import requests
import time

def set_channel(channel, value):
    url = f"http://localhost:8081/api/command?cmd=setChannel&value={channel}={value}"
    requests.get(url)

# Переключиться в ручной режим
requests.get("http://localhost:8081/api/command?cmd=setMode&value=manual")

# Поворот вправо
set_channel(1, 1800)
time.sleep(2)

# Центр
set_channel(1, 1500)
time.sleep(1)

# Поворот влево
set_channel(1, 1200)
time.sleep(2)

# Центр
set_channel(1, 1500)

# Вернуться к джойстику
requests.get("http://localhost:8081/api/command?cmd=setMode&value=joystick")
```

### 4. Bash скрипт

```bash
#!/bin/bash

API="http://localhost:8081/api/command"

# Переключиться в ручной режим
curl "$API?cmd=setMode&value=manual"

# Установить все каналы в центр
for i in {1..16}; do
  curl "$API?cmd=setChannel&value=$i=1500"
  sleep 0.05
done

# Тест Roll
curl "$API?cmd=setChannel&value=1=1800"
sleep 2
curl "$API?cmd=setChannel&value=1=1500"
sleep 1
curl "$API?cmd=setChannel&value=1=1200"
sleep 2
curl "$API?cmd=setChannel&value=1=1500"

# Вернуться к джойстику
curl "$API?cmd=setMode&value=joystick"
```

## Безопасность

### ⚠️ ВАЖНО

1. **Всегда устанавливайте безопасные значения перед переключением в ручной режим**
2. **Throttle должен быть на минимуме (1000) при наземных операциях**
3. **Всегда возвращайтесь в режим джойстика после тестирования**

### Рекомендуемая последовательность

```bash
# 1. Установить безопасные значения
curl "http://localhost:8081/api/command?cmd=setChannel&value=1=1500"
curl "http://localhost:8081/api/command?cmd=setChannel&value=2=1500"
curl "http://localhost:8081/api/command?cmd=setChannel&value=3=1000"  # Throttle минимум
curl "http://localhost:8081/api/command?cmd=setChannel&value=4=1500"

# 2. Переключиться в ручной режим
curl "http://localhost:8081/api/command?cmd=setMode&value=manual"

# 3. Выполнить операции

# 4. Вернуться в режим джойстика
curl "http://localhost:8081/api/command?cmd=setMode&value=joystick"
```

## Проверка текущего режима

```bash
curl http://localhost:8081/api/telemetry | grep workMode
```

Ответ:
```json
"workMode": "manual"
```
или
```json
"workMode": "joystick"
```

## Частота обновления

- Значения каналов устанавливаются мгновенно
- Отправка CRSF каналов: 100 Гц (каждые 10 мс)
- API ответ: немедленный

## Ограничения

- Ручной режим работает только через API
- Джойстик не реагирует в ручном режиме
- Значения сохраняются до следующего изменения
- При потере связи fail-safe срабатывает автоматически
