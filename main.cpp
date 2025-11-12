#include "config.h"
#include <thread>

#include "crsf/crsf.h"
#include "libs/rpi_hal.h"
#include "libs/joystick.h"
#include "telemetry_server.h"

// Главная точка входа Linux-приложения для Raspberry Pi
// Полная замена Arduino setup()/loop()
int main() {
#if USE_CRSF_RECV == true
#if DEVICE_2 == true
  PWMinit();      // Инициализация PWM для сервоприводов
#elif DEVICE_1 == true
  analogInit();   // Инициализация режимов GPIO/PWM для Н-моста
#endif
#endif
#if USE_CRSF_RECV == true
  crsfInitRecv(); // Запуск CRSF приёма
#endif
#if USE_CRSF_SEND == true
  crsfInitSend(); // Запуск CRSF передачи
#endif
#if PIN_INIT == true
  pinInit();      // Инициализация пинов реле/камеры
#endif

  // флаг доступности (не используется, можно удалить/раскомментировать при необходимости)
  // bool isCan = true;
  const uint32_t crsfSendPeriodMs = 10; // ~100 Гц отправка каналов для реалтайма
  uint32_t lastSendMs = 0;
  // Инициализация джойстика (не критично, если недоступен)
  if (js_open("/dev/input/js0")) {
    printf("Джойстик подключен: %d осей, %d кнопок\n", js_num_axes(), js_num_buttons());
  } else {
    printf("Предупреждение: джойстик недоступен, работа без управления\n");
  }

  // Запуск веб-сервера телеметрии в отдельном потоке
  std::thread webServerThread([]() {
    // Ждем инициализации CRSF (уменьшено для реалтайма)
    rpi_delay_ms(500);
    startTelemetryServer((CrsfSerial*)crsfGetActive(), 8081, 10);
  });
  webServerThread.detach();


  // Главный цикл
  for (;;) {
#if USE_CRSF_RECV == true
    loop_ch();
#endif

#if USE_CRSF_SEND == true
    uint32_t currentMillis = rpi_millis();

    // Читать события джойстика (неблокирующе)
    js_poll();

    // Преобразуем оси джойстика [-32767..32767] в CRSF [1000..2000]
    auto axisToUs = [](int16_t v) -> int {
      // нормируем к [-1..1]
      const float nf = (v >= 0) ? (static_cast<float>(v) / 32767.0f)
                                : (static_cast<float>(v) / 32768.0f);
      // диапазон [1000..2000]
      float us = 1500.0f + nf * 500.0f;
      int ius = static_cast<int>(us + 0.5f);
      if (ius < 1000) ius = 1000;
      if (ius > 2000) ius = 2000;
      return ius;
    };

    // Обработка осей джойстика только в режиме joystick
    std::string mode = getWorkMode();
    if (mode == "joystick") {
      int16_t ax0 = 0, ax1 = 0, ax2 = 0, ax3 = 0;
      bool axis0_ok = js_get_axis(0, ax0);
      bool axis1_ok = js_get_axis(1, ax1);
      bool axis2_ok = js_get_axis(2, ax2);
      bool axis3_ok = js_get_axis(3, ax3);
      
      if (axis0_ok) crsfSetChannel(1, axisToUs(ax2)); // Roll
      if (axis1_ok) crsfSetChannel(2, axisToUs(-ax3)); // Pitch
      if (axis2_ok) crsfSetChannel(3, axisToUs(-ax1)); // Throttle
      if (axis3_ok) crsfSetChannel(4, axisToUs(ax0)); // Yaw
    }

    // Отправляем RC-каналы с частотой ~100 Гц для реалтайма
    if (currentMillis - lastSendMs >= crsfSendPeriodMs) {
      lastSendMs = currentMillis;
      crsfSendChannels();
    }


#endif
    // Реалтайм без задержек - максимальная скорость обработки
    // rpi_delay_ms(0); // Полностью убираем задержку для реалтайма
  }

  return 0;
}
