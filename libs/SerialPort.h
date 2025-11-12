#pragma once

// Класс SerialPort для Linux (Raspberry Pi 5) с поддержкой нестандартных скоростей (в т.ч. 420000 бод)
// Использует termios2 (ioctl TCGETS2/TCSETS2)

#include <cstdint>
#include <cstddef>
#include <string>

class SerialPort {
public:
    // Конструктор: path — например "/dev/ttyAMA0" или "/dev/ttyS0"
    SerialPort(const std::string &path, uint32_t baud);
    ~SerialPort();

    bool isOpen() const { return _fd >= 0; }
    bool open();
    void close();

    // Неблокирующее чтение/запись (по умолчанию блокирующее с таймаутами через termios)
    int readByte(uint8_t &b);
    //БЕСПОЛЕЗНО: функция определена, но нигде не используется (используется только readByte)
    //int read(uint8_t *buf, size_t len);
    int write(const uint8_t *buf, size_t len);
    int writeByte(uint8_t b);

    void flush();

private:
    std::string _path;
    uint32_t _baud;
    int _fd;
    bool configureTermios2(uint32_t baud);
};


