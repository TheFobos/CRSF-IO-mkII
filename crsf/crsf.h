#ifndef CRSF_CRSF_H
#define CRSF_CRSF_H

#include <cstdint>
#include "../libs/rpi_hal.h"
#include "../libs/SerialPort.h"
#include "config.h"

void crsfInitRecv();
void crsfInitSend();
void loop_ch();
void crsfSetChannel(unsigned int ch, int value);
void crsfSendChannels();
void crsfTelemetrySend();
// Получить указатель на активный CRSF объект
void* crsfGetActive();
// Инициализация GPIO/PWM под Raspberry Pi
void PWMinit();       // настройка PWM (50 Гц для сервоприводов)
void analogInit();    // начальная инициализация ШИМ/цифровых пинов
void pinInit();       // инициализация дополнительных пинов (реле, камера)

#endif
