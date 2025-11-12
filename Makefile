CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -I.
LDFLAGS := -lpthread

SRC := \
	main.cpp \
	crsf/crsf.cpp \
	libs/crsf/CrsfSerial.cpp \
	libs/SerialPort.cpp \
	libs/rpi_hal.cpp \
	libs/crsf/crc8.cpp \
	libs/joystick.cpp \
	telemetry_server.cpp

OBJ := $(SRC:.cpp=.o)

BIN := crsf_io_rpi uart_test

all: $(BIN)

crsf_io_rpi: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

UART_TEST_SRC := uart_test.cpp libs/SerialPort.cpp
UART_TEST_OBJ := $(UART_TEST_SRC:.cpp=.o)

uart_test: $(UART_TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean


