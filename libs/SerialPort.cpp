#include "SerialPort.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm/termbits.h>
#include <cerrno>
#include <cstring>

// Реализация SerialPort для Linux с termios2

SerialPort::SerialPort(const std::string &path, uint32_t baud)
    : _path(path), _baud(baud), _fd(-1) {}

SerialPort::~SerialPort() { close(); }

bool SerialPort::open() {
    if (_fd >= 0) return true;
    _fd = ::open(_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (_fd < 0) {
        return false; // не удалось открыть устройство
    }

    // Сбрасываем флаг O_NONBLOCK после открытия
    int flags = fcntl(_fd, F_GETFL, 0);
    if (flags >= 0) fcntl(_fd, F_SETFL, flags & ~O_NONBLOCK);

    if (!configureTermios2(_baud)) {
        close();
        return false;
    }
    return true;
}

void SerialPort::close() {
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
}

bool SerialPort::configureTermios2(uint32_t baud) {
    // Используем termios2 для установки произвольной скорости
    struct termios2 tio2;
    if (ioctl(_fd, TCGETS2, &tio2) < 0) return false;

    tio2.c_cflag &= ~CBAUD;
    tio2.c_cflag |= BOTHER | CS8 | CREAD | CLOCAL;
    tio2.c_cflag &= ~(PARENB | CSTOPB);
    tio2.c_iflag = IGNPAR;
    tio2.c_oflag = 0;
    tio2.c_lflag = 0;
    tio2.c_ispeed = baud;
    tio2.c_ospeed = baud;

    // Минимум один байт, таймаут ~10мс
    tio2.c_cc[VMIN] = 0;
    tio2.c_cc[VTIME] = 1;

    if (ioctl(_fd, TCSETS2, &tio2) < 0) return false;

    // Сбрасываем буферы через ioctl
    ioctl(_fd, TCFLSH, TCIOFLUSH);
    return true;
}

int SerialPort::readByte(uint8_t &b) {
    uint8_t tmp;
    int r = ::read(_fd, &tmp, 1);
    if (r == 1) { b = tmp; return 1; }
    return r;
}

//БЕСПОЛЕЗНО: функция определена, но нигде не используется (используется только readByte)
/*
int SerialPort::read(uint8_t *buf, size_t len) {
    return ::read(_fd, buf, len);
}
*/

int SerialPort::write(const uint8_t *buf, size_t len) {
    return ::write(_fd, buf, len);
}

int SerialPort::writeByte(uint8_t b) {
    return ::write(_fd, &b, 1);
}

void SerialPort::flush() {
    // Простая очистка буферов ввода/вывода
    ioctl(_fd, TCFLSH, TCIOFLUSH);
}


