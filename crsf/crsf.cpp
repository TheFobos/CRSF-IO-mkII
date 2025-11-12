#include "crsf.h"

#if USE_CRSF_RECV == true || USE_CRSF_SEND == true
#include "libs/crsf/CrsfSerial.h"
#include "libs/log.h"

// Raspberry Pi: создаём два последовательных порта для CRSF
// Примечание: вам может потребоваться включить UART в raspi-config и накатить оверлеи
static SerialPort crsfPort1(CRSF_PORT_PRIMARY, CRSF_BAUD);
static SerialPort crsfPort2(CRSF_PORT_SECONDARY, CRSF_BAUD);
static CrsfSerial crsf_1(crsfPort1, CRSF_BAUD);
static CrsfSerial crsf_2(crsfPort2, CRSF_BAUD);
static CrsfSerial *crsf = &crsf_1;
// static uint32_t lastPortSwitchTime = 0; // Время последнего переключения порта - отключено

#if PIN_INIT == true
uint32_t old_time_rele1 = 0;
uint32_t old_time_rele2 = 0;
#endif

void packetChannels()
{
  static int16_t origCh1;
  static int16_t origCh2;
  // резервные каналы (не используются, оставлены для совместимости)
  // static int16_t origCh5;
  // static int16_t origCh8;
  static int16_t ch1;
  static int16_t ch2;

  origCh1 = crsf->getChannel(1);
  origCh2 = crsf->getChannel(2);
  // origCh5 = crsf->getChannel(5);
  // origCh8 = crsf->getChannel(8);

#if DEVICE_1 == true
  origCh2 = origCh2 / 2 - 750;
  origCh1
  
   = origCh1 / 2 - 750;
  ch1 = (origCh1 - origCh2) / 2;
  ch2 = -
  (origCh1 + origCh2) / 2;

#define DEAD_ZONE 50
  if (ch1 > -DEAD_ZONE && ch1 < DEAD_ZONE)
  {
    ch1 = 0;
  }

  if (ch2 > -DEAD_ZONE && ch2 < DEAD_ZONE)
  {
    ch2 = 0;
  }

  if (ch1 < 0)
  {
    // Направление и ШИМ для мотора 1
    rpi_gpio_write(motor_1_digital, true);
    rpi_pwm_set_duty_us({PWM_CHIP_M1, PWM_NUM_M1}, static_cast<uint32_t>(-ch1));
  }
  else
  {
    rpi_gpio_write(motor_1_digital, false);
    rpi_pwm_set_duty_us({PWM_CHIP_M1, PWM_NUM_M1}, static_cast<uint32_t>(ch1));
  }

  if (ch2 < 0)
  {
    rpi_gpio_write(motor_2_digital, true);
    rpi_pwm_set_duty_us({PWM_CHIP_M2, PWM_NUM_M2}, static_cast<uint32_t>(-ch2));
  }
  else
  {
    rpi_gpio_write(motor_2_digital, false);
    rpi_pwm_set_duty_us({PWM_CHIP_M2, PWM_NUM_M2}, static_cast<uint32_t>(ch2));
  }
#elif DEVICE_2 == true
  rpi_pwm_set_duty_us({PWM_CHIP_M1, PWM_NUM_M1}, static_cast<uint32_t>(origCh2));
  rpi_pwm_set_duty_us({PWM_CHIP_M2, PWM_NUM_M2}, static_cast<uint32_t>(origCh1));
#endif
#if PIN_INIT == true
  if (origCh5 > 1800)
  {
    if (old_time_rele1 == 0)
    {
      old_time_rele1 = millis();
    }
  }
  else
  {
    if (old_time_rele1 > 0)
    {
      old_time_rele1 = 0;
      digitalWrite(rele_1, LOW);
    }
  }
  if (origCh8 > 1800)
  {
    if (old_time_rele2 == 0)
    {
      old_time_rele2 = millis();
    }
  }
  else
  {
    if (old_time_rele2 > 0)
    {
      old_time_rele2 = 0;
      digitalWrite(rele_2, LOW);
    }
  }
#endif
}

static void crsfLinkUp()
{
  // Загорается индикатор связи: опустим GPIO LED_BUILTIN
  rpi_gpio_write(LED_BUILTIN, false);
}

static void crsfLinkDown_2()
{
  crsf = &crsf_1;
}

static void crsfLinkDown()
{
  crsf = &crsf_2;
}

void crsfSetChannel(unsigned int ch, int value)
{
  crsf->setChannel(ch, value); // Используем указатель на активный порт
}

void crsfSendChannels()
{
  crsf->packetChannelsSend(); // Используем указатель на активный порт
}

void* crsfGetActive()
{
  return (void*)crsf; // Возвращаем указатель на активный CRSF объект
}

void loop_ch()
{
  static uint32_t newTime;
  newTime = rpi_millis();

  // ПРОВЕРКА ПОТЕРИ СВЯЗИ (FAILSAFE)
  // Если от полетника не было НИКАКИХ данных более 1 секунды (1000 мс)
  if (newTime - crsf->_lastReceive > 1000)
  {
      // Устанавливаем моторы/сервы в безопасное положение
      #if DEVICE_1 == true
          rpi_pwm_set_duty_us({PWM_CHIP_M1, PWM_NUM_M1}, 0);
          rpi_pwm_set_duty_us({PWM_CHIP_M2, PWM_NUM_M2}, 0);
      #elif DEVICE_2 == true
          rpi_pwm_set_duty_us({PWM_CHIP_M1, PWM_NUM_M1}, 1500);
          rpi_pwm_set_duty_us({PWM_CHIP_M2, PWM_NUM_M2}, 1500);
      #endif
  }

  // ОТКЛЮЧЕНО: ПЕРЕКЛЮЧЕНИЕ ПОРТОВ
  // Если от полетника не было НИКАКИХ данных более 30 секунд (30000 мс)
  // И если связь вообще когда-либо была (_lastReceive != 0)
  // И прошло не менее 5 секунд с последнего переключения (стабилизация)
  /*
  if ((crsf->_lastReceive != 0) && (newTime - crsf->_lastReceive > 30000) && 
      (newTime - lastPortSwitchTime > 5000))
  {
      lastPortSwitchTime = newTime; // Запоминаем время переключения
      if (crsf == &crsf_1)
      {
        crsfLinkDown(); // Переключаемся на порт 2
      }
      else
      {
        crsfLinkDown_2(); // Переключаемся на порт 1
      }
  }
  */

  #if PIN_INIT == true
  if (old_time_rele1 > 0)
  {
    if (newTime - old_time_rele1 > 3000)
    {
      digitalWrite(rele_1, HIGH);
    }
  }
  if (old_time_rele2 > 0)
  {
    if (newTime - old_time_rele2 > 3000)
    {
      digitalWrite(rele_2, HIGH);
    }
  }
  #endif

  // Вызываем обработчик текущего активного порта
  crsf->loop();
}

void PWMinit()
{
  // Raspberry Pi: настраиваем GPIO и PWM для каналов моторов
  rpi_gpio_export(motor_1_digital);
  rpi_gpio_export(motor_2_digital);
  rpi_gpio_set_mode(motor_1_digital, RpiGpioMode::Output);
  rpi_gpio_set_mode(motor_2_digital, RpiGpioMode::Output);
  // PWM: 50 Гц для сервоприводов/ESC, начальное положение 1500 мкс
  rpi_pwm_export({PWM_CHIP_M1, PWM_NUM_M1});
  rpi_pwm_export({PWM_CHIP_M2, PWM_NUM_M2});
  rpi_pwm_set_frequency({PWM_CHIP_M1, PWM_NUM_M1}, 50);
  rpi_pwm_set_frequency({PWM_CHIP_M2, PWM_NUM_M2}, 50);
  rpi_pwm_set_duty_us({PWM_CHIP_M1, PWM_NUM_M1}, 1500);
  rpi_pwm_set_duty_us({PWM_CHIP_M2, PWM_NUM_M2}, 1500);
  rpi_pwm_enable({PWM_CHIP_M1, PWM_NUM_M1}, true);
  rpi_pwm_enable({PWM_CHIP_M2, PWM_NUM_M2}, true);
}

void analogInit()
{
  // Устанавливаем нулевую скважность PWM и готовим цифровые пины направления
  rpi_pwm_set_duty_us({PWM_CHIP_M1, PWM_NUM_M1}, 0);
  rpi_pwm_set_duty_us({PWM_CHIP_M2, PWM_NUM_M2}, 0);
  rpi_gpio_set_mode(motor_2_digital, RpiGpioMode::Output);
  rpi_gpio_set_mode(motor_1_digital, RpiGpioMode::Output);
}

void pinInit()
{
  // Инициализация пинов: камера и реле — в низкий уровень (выключено)
  rpi_gpio_set_mode(camera, RpiGpioMode::Output);
  rpi_gpio_set_mode(rele_1, RpiGpioMode::Output);
  rpi_gpio_set_mode(rele_2, RpiGpioMode::Output);
  rpi_gpio_write(camera, false);
  rpi_gpio_write(rele_2, false);
  rpi_gpio_write(rele_1, false);
}

void crsfInitRecv()
{
  // Открываем последовательные порты для CRSF
  crsfPort1.open();
  crsfPort2.open();
  crsf_2.onPacketChannels = &packetChannels;
  crsf_2.onLinkUp = &crsfLinkUp;
  crsf_2.onLinkDown = &crsfLinkDown_2;
  crsf_1.onPacketChannels = &packetChannels;
  crsf_1.onLinkUp = &crsfLinkUp;
  crsf_1.onLinkDown = &crsfLinkDown;
  // Простейшие проверки порта
  // Если основной порт не открылся — переключаемся на вторичный
  if (!crsfPort1.isOpen() && crsfPort2.isOpen()) {
    crsf = &crsf_2;
  }
}

void crsfInitSend()
{
  // Для Raspberry Pi используем первичный порт
  crsfPort1.open();
}

struct packet_CRSF_FRAMETYPE_BATTERY_SENSOR
{
  uint16_t voltage;
  uint16_t current;
  uint capacity : 24;
  uint8_t remaining;
};

#define swap2Bytes(x) ((((x) >> 8) & 0xFF) | ((x) << 8))

void crsfTelemetrySend()
{
  static packet_CRSF_FRAMETYPE_BATTERY_SENSOR packet;
  // static uint16_t kR = (47000 + 3300) / 33; // коэффициент делителя (не используется на RPi)
  packet.remaining = 0;
  uint16_t v = (uint16_t)((27.8 * 1000) / 100);

  // На Raspberry Pi нет аналогового входа по умолчанию; оставим заглушку
  // v = измеренное напряжение (мВ) — замените на чтение из вашего АЦП
  // v = read_adc_mv() * kR / 100; // пример
  uint16_t c = 0;
  packet.voltage = swap2Bytes(v);
  packet.current = swap2Bytes(c);
  uint32_t capacity = 0;
  packet.capacity = capacity;
  packet.current = (uint16_t)((7.3 * 1000) * 100);
  crsf_1.queuePacket(CRSF_ADDRESS_FLIGHT_CONTROLLER, CRSF_FRAMETYPE_BATTERY_SENSOR, (void *)&packet, 8);
}
#endif
