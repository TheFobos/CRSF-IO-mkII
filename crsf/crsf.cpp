#include "crsf.h"

#if USE_CRSF_RECV == true || USE_CRSF_SEND == true
#include "../libs/crsf/CrsfSerial.h"

// Raspberry Pi: создаём два последовательных порта для CRSF
// Примечание: вам может потребоваться включить UART в raspi-config и накатить оверлеи
static SerialPort crsfPort1(CRSF_PORT_PRIMARY, CRSF_BAUD);
static SerialPort crsfPort2(CRSF_PORT_SECONDARY, CRSF_BAUD);
static CrsfSerial crsf_1(crsfPort1, CRSF_BAUD);
static CrsfSerial crsf_2(crsfPort2, CRSF_BAUD);
static CrsfSerial *crsf = &crsf_1;

//БЕСПОЛЕЗНО: функция вызывается, но ничего не делает (LED закомментирован)
/*static void crsfLinkUp()
{
  // Индикатор связи - можно добавить LED если нужно
  // rpi_gpio_write(LED_BUILTIN, false);
}*/

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
  
  // static uint32_t newTime;
  // newTime = rpi_millis();

  // ПРОВЕРКА ПОТЕРИ СВЯЗИ (FAILSAFE)
  // Если от полетника не было НИКАКИХ данных более 1 секунды (1000 мс)
  // Управление сервоприводами теперь выполняет полетник, поэтому failsafe здесь не нужен
  // if (newTime - crsf->_lastReceive > 1000)
  // {
  //     // Устанавливаем моторы/сервы в безопасное положение
  // }

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

  // Вызываем обработчик текущего активного порта
  crsf->loop();
}

void crsfInitRecv()
{
  // Открываем последовательные порты для CRSF
  crsfPort1.open();
  crsfPort2.open();
  crsf_2.onLinkDown = &crsfLinkDown_2;
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

#endif
