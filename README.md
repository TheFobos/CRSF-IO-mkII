# CRSF-IO-mkII

Raspberry Pi CRSF Protocol Interface для управления коптерами

## Описание

CRSF-IO-mkII - это система для Raspberry Pi, которая обеспечивает прием и отправку CRSF (Crossfire) протокола, управление через джойстик или Python bindings, веб-интерфейс для телеметрии и поддержку различных режимов работы.

## Возможности

- ✅ Прием RC-каналов от полетного контроллера
- ✅ Отправка команд управления
- ✅ Телеметрия в реальном времени (GPS, Батарея, Attitude)
- ✅ Python GUI интерфейс
- ✅ Python bindings (pybind11) для интеграции с Python приложениями
- ✅ Поддержка двух режимов работы (Joystick/Manual)
- ✅ Fail-safe защита
- ✅ Полное покрытие unit-тестами (86 тестов)

## Быстрый старт

### Установка

```bash
git clone <repository_url>
cd CRSF-IO-mkII
make
```

### Запуск

```bash
sudo ./crsf_io_rpi
```

### Python GUI

```bash
python3 crsf_realtime_interface.py
```

### Python Bindings (pybind11)

```bash
cd pybind
python3 build_lib.py build_ext --inplace
python3 -c "from crsf_wrapper import CRSFWrapper; crsf = CRSFWrapper(); crsf.auto_init()"
```

Подробнее: [pybind/README.md](pybind/README.md)

## Документация

- **[Python Bindings Guide](docs/PYTHON_BINDINGS_README.md)** - Подробное руководство по работе с pybind11
- **[Configuration Guide](docs/CONFIG_README.md)** - Настройка и конфигурация
- **[Build Guide](docs/MAKEFILE_README.md)** - Руководство по сборке
- **[Manual Mode Guide](docs/MANUAL_MODE_GUIDE.md)** - Ручной режим управления через Python
- **[Telemetry Documentation](docs/README_telemetry.md)** - Документация по телеметрии
- **[Python Bindings](pybind/README.md)** - Pybind11 модуль для Python
- **[Unit Tests](unit/README.md)** - Документация по unit-тестам
- **[Documentation Index](docs/DOCUMENTATION_INDEX.md)** - Полный список документации

## Работа с Python Bindings

### Получение телеметрии

```python
from crsf_wrapper import CRSFWrapper

crsf = CRSFWrapper()
crsf.auto_init()

# Получить все данные телеметрии
telemetry = crsf.get_telemetry()
print(f"GPS: {telemetry['gps']['latitude']}, {telemetry['gps']['longitude']}")
print(f"Battery: {telemetry['battery']['voltage']}V")
print(f"Attitude: Roll={telemetry['attitude']['roll']}°")
```

### Управление каналами

```python
# Переключиться в ручной режим
crsf.set_work_mode("manual")

# Установить канал 1 в центр (1500)
crsf.set_channel(1, 1500)

# Установить все каналы одновременно
crsf.set_channels([1500, 1500, 1000, 1500, 1500, 1500, 1500, 1500,
                   1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500])

# Отправить пакет каналов
crsf.send_channels()
```

Подробнее: [PYTHON_BINDINGS_README.md](docs/PYTHON_BINDINGS_README.md)

## Тестирование

Проект включает полный набор unit-тестов (86 тестов), покрывающих все основные компоненты:

```bash
# Запуск всех тестов
cd unit
bash run_tests.sh

# Или через make
cd unit && make test
```

**Статус тестов:** ✅ Все 86 тестов успешно пройдены

Тесты покрывают:
- CRC8 вычисления и валидация
- Парсинг всех типов CRSF пакетов (GPS, Battery, Link Statistics, Attitude, Flight Mode)
- Кодирование/декодирование каналов
- Управление буфером и обработка ошибок
- Состояние связи и fail-safe механизм
- Отправка пакетов и телеметрия

Подробнее: [unit/README.md](unit/README.md)

## Структура проекта

```
CRSF-IO-mkII/
├── main.cpp              # Главная точка входа
├── config.h              # Конфигурация (см. docs/CONFIG_README.md)
├── Makefile              # Сборка (см. docs/MAKEFILE_README.md)
├── crsf/                 # CRSF модуль
├── libs/                 # Библиотеки
├── pybind/               # Python bindings (pybind11)
├── unit/                 # Unit-тесты (86 тестов)
├── telemetry_server.cpp  # Веб-сервер
├── crsf_realtime_interface.py  # Python GUI
└── docs/                 # Документация
```

## Требования

- Raspberry Pi (протестировано на Raspberry Pi 5)
- Linux с поддержкой GPIO и UART
- C++17 компилятор
- Python 3.x
- Google Test и Google Mock (для unit-тестов)

## Лицензия

(Укажите вашу лицензию)

## Поддержка

При возникновении проблем создайте issue в репозитории.
