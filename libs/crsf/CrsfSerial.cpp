#include "CrsfSerial.h"
#include <cstring>


// Конструктор под Raspberry Pi: SerialPort уже открыт с нужной скоростью
CrsfSerial::CrsfSerial(SerialPort& port, uint32_t baud) :
    _port(port), _crc(0xd5), _baud(baud),
    _lastReceive(0), _lastChannelsPacket(0), _linkIsUp(false),
    _batteryVoltage(0.0), _batteryCurrent(0.0), _batteryCapacity(0.0), _batteryRemaining(0),
    _attitudeRoll(0.0), _attitudePitch(0.0), _attitudeYaw(0.0),
    _rawAttitudeBytes{0, 0, 0}
{
    // Ничего дополнительно не делаем: открытие и настройка порта снаружи
}

// Call from main loop to update
void CrsfSerial::loop()
{
    handleSerialIn();
}

void CrsfSerial::handleSerialIn()
{
    // Читаем не более 32 байт за раз, чтобы не блокировать основной цикл
    for (int i = 0; i < 32; ++i) { 
        uint8_t b;
        int r = _port.readByte(b);
        if (r <= 0) {
            break; // Прерываем, если в порту больше нет данных
        }

        _lastReceive = rpi_millis();
        _rxBuf[_rxBufPos++] = b;
        handleByteReceived();

        if (_rxBufPos == CRSF_MAX_PACKET_SIZE) {
            _rxBufPos = 0;
        }
    }

    checkPacketTimeout();
    checkLinkDown();
}

void CrsfSerial::handleByteReceived()
{
    bool reprocess;
    do {
        reprocess = false;
        if (_rxBufPos > 1) {
            uint8_t len = _rxBuf[1];
            // Sanity check the declared length isn't outside Type + X{1,CRSF_MAX_PAYLOAD_LEN} + CRC
            // assumes there never will be a CRSF message that just has a type and no data (X)
            if (len < 3 || len >(CRSF_MAX_PAYLOAD_LEN + 2)) {
                shiftRxBuffer(1);
                reprocess = true;
            }

            else if (_rxBufPos >= (len + 2)) {
                uint8_t inCrc = _rxBuf[2 + len - 1];
                uint8_t crc = _crc.calc(&_rxBuf[2], len - 1);
                if (crc == inCrc) {
                    processPacketIn(len);
                    shiftRxBuffer(len + 2);
                    reprocess = true;
                } else {
                    // Отбрасываем ВЕСЬ битый пакет, а не один байт
                    shiftRxBuffer(len + 2);
                    reprocess = true;
                }
            }  // if complete packet
        } // if pos > 1
    } while (reprocess);
}

void CrsfSerial::checkPacketTimeout()
{
    // If we haven't received data in a long time, flush the buffer a byte at a time (to trigger shiftyByte)
    if (_rxBufPos > 0 && rpi_millis() - _lastReceive > CRSF_PACKET_TIMEOUT_MS)
        while (_rxBufPos)
            shiftRxBuffer(1);
}

void CrsfSerial::checkLinkDown()
{
    // Проверяем общее время последнего получения ЛЮБЫХ данных, а не только RC-каналов
    if (_linkIsUp && rpi_millis() - _lastReceive > CRSF_FAILSAFE_STAGE1_MS) {
        if (onLinkDown)
            onLinkDown();
        _linkIsUp = false;
    }
}

void CrsfSerial::processPacketIn(uint8_t len)
{
    const crsf_header_t* hdr = (crsf_header_t*)_rxBuf;
    if (hdr->device_addr == CRSF_ADDRESS_FLIGHT_CONTROLLER) {
        switch (hdr->type) {
        case CRSF_FRAMETYPE_GPS:
            packetGps(hdr);
            break;
        case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
            // softSerial.println("CRSF_FRAMETYPE_RC_CHANNELS_PACKED");
            packetChannelsPacked(hdr);
            break;
        case CRSF_FRAMETYPE_LINK_STATISTICS:
            packetLinkStatistics(hdr);
            break;
        case CRSF_FRAMETYPE_ATTITUDE:
            packetAttitude(hdr);
            break;
        case CRSF_FRAMETYPE_FLIGHT_MODE:
            packetFlightMode(hdr);
            break;
        case CRSF_FRAMETYPE_BATTERY_SENSOR:
            packetBatterySensor(hdr);
            break;
        default:
        // Неизвестный тип пакета
        break;
        }
    } // CRSF_ADDRESS_FLIGHT_CONTROLLER
}

// Shift the bytes in the RxBuf down by cnt bytes
void CrsfSerial::shiftRxBuffer(uint8_t cnt)
{
    // If removing the whole thing, just set pos to 0
    if (cnt >= _rxBufPos) {
        _rxBufPos = 0;
        return;
    }

    //БЕСПОЛЕЗНО: указатель onShiftyByte никогда не устанавливается
    //if (cnt == 1 && onShiftyByte)
    //    onShiftyByte(_rxBuf[0]);

    // Otherwise do the slow shift down
    uint8_t* src = &_rxBuf[cnt];
    uint8_t* dst = &_rxBuf[0];
    _rxBufPos -= cnt;
    uint8_t left = _rxBufPos;
    while (left--)
        *dst++ = *src++;
}

void CrsfSerial::packetChannelsPacked(const crsf_header_t* p)
{
    crsf_channels_t* ch = (crsf_channels_t*)&p->data;
    _channels[0] = ch->ch0;
    _channels[1] = ch->ch1;
    _channels[2] = ch->ch2;
    _channels[3] = ch->ch3;
    _channels[4] = ch->ch4;
    _channels[5] = ch->ch5;
    _channels[6] = ch->ch6;
    _channels[7] = ch->ch7;
    _channels[8] = ch->ch8;
    _channels[9] = ch->ch9;
    _channels[10] = ch->ch10;
    _channels[11] = ch->ch11;
    _channels[12] = ch->ch12;
    _channels[13] = ch->ch13;
    _channels[14] = ch->ch14;
    _channels[15] = ch->ch15;

    // Преобразование CRSF-кода в микросекунды (1000..2000) с точным округлением
    const int crsfDelta = (CRSF_CHANNEL_VALUE_2000 - CRSF_CHANNEL_VALUE_1000);
    for (unsigned int i = 0; i < CRSF_NUM_CHANNELS; ++i) {
        int code = _channels[i];
        if (code < CRSF_CHANNEL_VALUE_1000) code = CRSF_CHANNEL_VALUE_1000;
        if (code > CRSF_CHANNEL_VALUE_2000) code = CRSF_CHANNEL_VALUE_2000;
        int num = (code - CRSF_CHANNEL_VALUE_1000) * 1000;
        _channels[i] = 1000 + (num + crsfDelta / 2) / crsfDelta; // округление к ближайшему
    }

    if (!_linkIsUp && onLinkUp)
        onLinkUp();
    _linkIsUp = true;
    _lastChannelsPacket = rpi_millis();

    //БЕСПОЛЕЗНО: onPacketChannels никогда не устанавливается, так как packetChannels удалена
    if (onPacketChannels)
        onPacketChannels();
}

void CrsfSerial::packetLinkStatistics(const crsf_header_t* p)
{
    const crsfLinkStatistics_t* link = (crsfLinkStatistics_t*)p->data;
    memcpy(&_linkStatistics, link, sizeof(_linkStatistics));

    //БЕСПОЛЕЗНО: указатель onPacketLinkStatistics никогда не устанавливается
    //if (onPacketLinkStatistics)
    //    onPacketLinkStatistics(&_linkStatistics);
}

void CrsfSerial::packetGps(const crsf_header_t* p)
{
    const crsf_sensor_gps_t* gps = (crsf_sensor_gps_t*)p->data;
    _gpsSensor.latitude = be32toh(gps->latitude);
    _gpsSensor.longitude = be32toh(gps->longitude);
    _gpsSensor.groundspeed = be16toh(gps->groundspeed);
    _gpsSensor.heading = be16toh(gps->heading);
    _gpsSensor.altitude = be16toh(gps->altitude);
    _gpsSensor.satellites = gps->satellites;

    //БЕСПОЛЕЗНО: указатель onPacketGps никогда не устанавливается
    //if (onPacketGps)
    //    onPacketGps(&_gpsSensor);
}

void CrsfSerial::write(uint8_t b)
{
    _port.writeByte(b);
}

void CrsfSerial::write(const uint8_t* buf, size_t len)
{
    _port.write(buf, len);
}

void CrsfSerial::queuePacket(uint8_t addr, uint8_t type, const void* payload, uint8_t len)
{
    if (!_linkIsUp)
        return;
    if (len > CRSF_MAX_PAYLOAD_LEN)
        return;

    //БЕСПОЛЕЗНО: устаревший Arduino код - закомментированные строки отладки
    // uint8_t* t = (uint8_t*)payload;
    // for (int i = 0; i < 22; i++) {
    //     t[i] = i;
    // }

    static uint8_t buf[CRSF_MAX_PACKET_SIZE];
    buf[0] = addr;
    buf[1] = len + 2; // type + payload + crc
    buf[2] = type;
    memcpy(buf + 3, payload, len);
    buf[len + 3] = _crc.calc(&buf[2], len + 1);
    // buf[len + 4] = 0x45;
    // buf[3] = 0x03;
    // Busywait until the serial port seems free
    //while (rpi_millis() - _lastReceive < 2)
    //    loop();
    // for (int i = 0; i < 25; i++) {
    //     buf[i] = 0;
    //     if (i)
    //         Serial.write(i);
    //     else {
    //         Serial.print(0, BYTE);
    //     }
    // }
    write(buf, len + 4);
    //БЕСПОЛЕЗНО: закомментированный лог
    // log_info("CRSF: отправлен пакет типа " + std::to_string(type));
}

//БЕСПОЛЕЗНО: функция определена, но нигде не вызывается
/*
void CrsfSerial::setPassthroughMode(bool val, unsigned int baud)
{
    _passthroughMode = val;
    // На Raspberry Pi не перенастраиваем порт здесь; просто очищаем буфер
    _port.flush();
}
*/

void CrsfSerial::packetChannelsSend()
{

    static int channels[CRSF_NUM_CHANNELS];
    const int crsfDelta = (CRSF_CHANNEL_VALUE_2000 - CRSF_CHANNEL_VALUE_1000);
    for (unsigned int i = 0; i < CRSF_NUM_CHANNELS; ++i) {
        int usTarget = _channels[i];
        if (usTarget < 1000) usTarget = 1000;
        if (usTarget > 2000) usTarget = 2000;

        // Первичное кодирование (округление к ближайшему)
        int num = (usTarget - 1000) * crsfDelta;
        int code = CRSF_CHANNEL_VALUE_1000 + (num + 500) / 1000;
        if (code > CRSF_CHANNEL_VALUE_2000) code = CRSF_CHANNEL_VALUE_2000;
        if (code < CRSF_CHANNEL_VALUE_1000) code = CRSF_CHANNEL_VALUE_1000;

        // Проверка: декодирование должно дать ровно usTarget
        int decodedUs = 1000 + ((code - CRSF_CHANNEL_VALUE_1000) * 1000 + crsfDelta / 2) / crsfDelta;
        if (decodedUs < usTarget && code < CRSF_CHANNEL_VALUE_2000) {
            // Подправим вверх до точного совпадения, шаг обычно <= 1
            int code2 = code + 1;
            int decoded2 = 1000 + ((code2 - CRSF_CHANNEL_VALUE_1000) * 1000 + crsfDelta / 2) / crsfDelta;
            if (decoded2 == usTarget) code = code2;
        } else if (decodedUs > usTarget && code > CRSF_CHANNEL_VALUE_1000) {
            int code2 = code - 1;
            int decoded2 = 1000 + ((code2 - CRSF_CHANNEL_VALUE_1000) * 1000 + crsfDelta / 2) / crsfDelta;
            if (decoded2 == usTarget) code = code2;
        }

        channels[i] = code;
    }

    crsf_channels_t ch;
    ch.ch0 = channels[0];
    ch.ch1 = channels[1];
    ch.ch2 = channels[2];
    ch.ch3 = channels[3];
    ch.ch4 = channels[4];
    ch.ch5 = channels[5];
    ch.ch6 = channels[6];
    ch.ch7 = channels[7];
    ch.ch8 = channels[8];
    ch.ch9 = channels[9];
    ch.ch10 = channels[10];
    ch.ch11 = channels[11];
    ch.ch12 = channels[12];
    ch.ch13 = channels[13];
    ch.ch14 = channels[14];
    ch.ch15 = channels[15];


    _linkIsUp = true;
    queuePacket(CRSF_ADDRESS_FLIGHT_CONTROLLER, CRSF_FRAMETYPE_RC_CHANNELS_PACKED, (void*)&ch, 22);
}

void CrsfSerial::packetAttitude(const crsf_header_t* p)
{
    if (p->frame_size >= 6) {
        // Читаем 6 байт ATTITUDE пакета
        int16_t rawVal0 = be16toh(*(int16_t*)&p->data[0]);
        int16_t rawVal2 = be16toh(*(int16_t*)&p->data[2]);
        int16_t rawVal4 = be16toh(*(int16_t*)&p->data[4]);
        
        // ВНИМАНИЕ: Эти коэффициенты получены экспериментально и могут различаться 
        // в зависимости от версии прошивки полетного контроллера (Betaflight/iNAV).
        // Стандартная спецификация CRSF: градусы × 100, но данные не соответствуют.
        
        // bytes 0-1 = Pitch, bytes 2-3 = Roll (поменяны местами!)
        // Сохраняем сырые значения (raw int16_t)
        _rawAttitudeBytes[0] = rawVal0;  // Pitch raw
        _rawAttitudeBytes[1] = rawVal2;  // Roll raw
        _rawAttitudeBytes[2] = rawVal4;  // Yaw raw
        
        // КОНВЕРТАЦИЯ С ПРАВИЛЬНЫМ ПОРЯДКОМ
        _attitudeRoll = rawVal2 / 175.0;    // bytes 2-3 = Roll
        _attitudePitch = rawVal0 / 175.0;   // bytes 0-1 = Pitch
        
        // Yaw: конвертация с нормализацией к диапазону 0-360 градусов
        double yawDegrees = rawVal4 / 175.0;
        
        // Нормализация yaw к диапазону 0-360°
        while (yawDegrees < 0) yawDegrees += 360.0;
        while (yawDegrees >= 360.0) yawDegrees -= 360.0;
        
        _attitudeYaw = yawDegrees;
    }
}

void CrsfSerial::packetFlightMode(const crsf_header_t* p)
{
    // FLIGHT_MODE пакет содержит строку с режимом полета
    if (p->frame_size > 0) {
        std::string flightMode((char*)p->data, p->frame_size - 1); // -1 для CRC
        // Данные телеметрии будут обновлены через веб-сервер
    }
}

void CrsfSerial::packetBatterySensor(const crsf_header_t* p)
{
    // BATTERY_SENSOR пакет содержит напряжение, ток, емкость
    if (p->frame_size >= 8) {
        uint16_t voltage = be16toh(*(uint16_t*)&p->data[0]); // мВ
        uint16_t current = be16toh(*(uint16_t*)&p->data[2]); // мА
        // Читаем 24-битное значение емкости
        uint32_t capacity = (p->data[4] << 16) | (p->data[5] << 8) | p->data[6]; // мАч
        uint8_t remaining = p->data[7]; // %
        
        // Сохраняем данные батареи
        _batteryVoltage = voltage / 100.0; // Конвертируем мВ в В
        _batteryCurrent = current; // мА
        _batteryCapacity = capacity; // мАч
        _batteryRemaining = remaining; // %
    }
}

