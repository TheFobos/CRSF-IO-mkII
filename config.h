#ifndef CONFIG_H
#define CONFIG_H

#define USE_CRSF_RECV true   // включить приём CRSF на Raspberry Pi
#define USE_CRSF_SEND true   // включить отправку телеметрии CRSF
#define USE_LOG false    // включить журналы для отладки yaw

#define DEVICE_1 false  // режим: 1 — Н-мост с ШИМ и направлением; 2 — сервоприводы 50 Гц
#define DEVICE_2 false
#define PIN_INIT false  // инициализация доп. пинов (реле/камера)

// Raspberry Pi 5: используем BCM-номера пинов GPIO
// ВНИМАНИЕ: проверьте соответствие реальному подключению!
#define  motor_1_digital 17   // GPIO17 — направление мотора 1
#define  motor_2_digital 27   // GPIO27 — направление мотора 2
#define  motor_1_analog  18   // GPIO18 (PWM0) — ШИМ мотора 1
#define  motor_2_analog  19   // GPIO19 (PWM1) — ШИМ мотора 2
#define  rele_1 22            // GPIO22 — реле 1
#define  rele_2 23            // GPIO23 — реле 2
#define  camera 24            // GPIO24 — управление камерой

// Для Raspberry Pi задаём PWM чип/канал sysfs (может отличаться на вашей системе)
// Примеры: pwmchip0/pwm0 для GPIO18 (PWM0), pwmchip0/pwm1 или pwmchip2/pwm1 для GPIO19 — проверьте /sys/class/pwm/
#define  PWM_CHIP_M1 0  // pwmchip номер для мотора 1
#define  PWM_NUM_M1  0  // номер канала внутри pwmchip для мотора 1
#define  PWM_CHIP_M2 0  // pwmchip номер для мотора 2
#define  PWM_NUM_M2  1  // номер канала внутри pwmchip для мотора 2

#define SERIAL_BAUD 115200   // обычная отладочная скорость, если нужна
#define CRSF_BAUD 420000     // скорость CRSF

// Пути к последовательным портам Raspberry Pi для CRSF
// Обычно: "/dev/ttyAMA0" (PL011) и "/dev/ttyS0" (miniUART)
#define CRSF_PORT_PRIMARY "/dev/ttyAMA0"
#define CRSF_PORT_SECONDARY "/dev/ttyS0"

// Светодиод на Raspberry Pi отсутствует по умолчанию; можно назначить GPIO
#define LED              5
#define LED_BUILTIN      5
#endif
