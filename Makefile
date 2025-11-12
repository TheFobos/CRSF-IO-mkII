CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -I.
LDFLAGS := -lpthread

# Исходные файлы
SRC := \
	main.cpp \
	crsf/crsf.cpp \
	libs/crsf/CrsfSerial.cpp \
	libs/SerialPort.cpp \
	libs/rpi_hal.cpp \
	libs/crsf/crc8.cpp \
	libs/joystick.cpp \
	telemetry_server.cpp

# Объектные файлы
OBJ := $(SRC:.cpp=.o)

# Исполняемый файл
BIN := crsf_io_rpi

# Цель по умолчанию
all: $(BIN)

# Сборка основного приложения
$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Правило компиляции объектных файлов
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Очистка артефактов сборки
clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean


