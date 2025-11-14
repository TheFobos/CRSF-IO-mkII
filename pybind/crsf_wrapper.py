#!/usr/bin/env python3
"""
CRSF Python Wrapper
Обертка для работы с CRSF через pybind11
"""

import crsf_native
from typing import Dict, List, Optional
import ctypes
import os


class CRSFWrapper:
    """Python обертка для CRSF C++ библиотеки"""
    
    def __init__(self, crsf_ptr=None):
        """
        Инициализация обертки
        
        Args:
            crsf_ptr: Указатель на C++ CRSF объект (ctypes.c_void_p)
                     Если None, будет использован указатель из crsfGetActive()
        """
        self._initialized = False
        if crsf_ptr is not None:
            self.init(crsf_ptr)
    
    def init(self, crsf_ptr):
        """
        Инициализация CRSF экземпляра
        
        Args:
            crsf_ptr: Указатель на CRSF объект (int, ctypes.c_void_p или uintptr_t)
        """
        # Преобразуем указатель в целое число
        if isinstance(crsf_ptr, ctypes.c_void_p):
            ptr_value = crsf_ptr.value
        elif isinstance(crsf_ptr, int):
            ptr_value = crsf_ptr
        else:
            ptr_value = int(crsf_ptr)
        
        crsf_native.init_crsf_instance(ptr_value)
        self._initialized = True
    
    def auto_init(self):
        """
        Автоматическая инициализация CRSF
        
        Проверяет наличие файла с телеметрией, созданного основным приложением.
        Данные читаются из файла /tmp/crsf_telemetry.dat без прямого доступа к указателю
        (безопасно для межпроцессного взаимодействия).
        """
        # Проверяем наличие файла с телеметрией
        telemetry_file = "/tmp/crsf_telemetry.dat"
        if os.path.exists(telemetry_file):
            # Файл существует, значит основное приложение запущено и записывает данные
            self._initialized = True
            return
        
        raise RuntimeError("Файл с телеметрией не найден. Убедитесь, что crsf_io_rpi запущен.")
    
    def get_telemetry(self) -> Dict:
        """
        Получить телеметрию
        
        Returns:
            Словарь с данными телеметрии в формате, совместимом с API
        """
        if not self._initialized:
            raise RuntimeError("CRSF не инициализирован. Вызовите auto_init() сначала.")
        
        # Данные читаются из файла безопасным способом (без прямого доступа к указателю)
        data = crsf_native.get_telemetry()
        
        # Преобразуем в формат, совместимый с API
        return {
            'linkUp': data.linkUp,
            'activePort': data.activePort,
            'lastReceive': data.lastReceive,
            'timestamp': data.timestamp,
            'channels': data.channels,
            'packetsReceived': data.packetsReceived,
            'packetsSent': data.packetsSent,
            'packetsLost': data.packetsLost,
            'gps': {
                'latitude': data.latitude,
                'longitude': data.longitude,
                'altitude': data.altitude,
                'speed': data.speed
            },
            'battery': {
                'voltage': data.voltage,
                'current': data.current,
                'capacity': data.capacity,
                'remaining': data.remaining
            },
            'attitude': {
                'roll': data.roll,
                'pitch': data.pitch,
                'yaw': data.yaw
            },
            'attitudeRaw': {
                'roll': data.rollRaw,
                'pitch': data.pitchRaw,
                'yaw': data.yawRaw
            },
            'workMode': self.get_work_mode()
        }
    
    def set_work_mode(self, mode: str):
        """
        Установить режим работы
        
        Args:
            mode: 'joystick' или 'manual'
        """
        if mode not in ['joystick', 'manual']:
            raise ValueError(f"Неверный режим: {mode}. Допустимые значения: 'joystick', 'manual'")
        crsf_native.set_work_mode(mode)
    
    def get_work_mode(self) -> str:
        """
        Получить текущий режим работы
        
        Returns:
            'joystick' или 'manual'
        """
        return crsf_native.get_work_mode()
    
    def set_channel(self, channel: int, value: int):
        """
        Установить значение канала
        
        Args:
            channel: Номер канала (1-16)
            value: Значение (1000-2000)
        """
        if not (1 <= channel <= 16):
            raise ValueError(f"Номер канала должен быть от 1 до 16, получено: {channel}")
        if not (1000 <= value <= 2000):
            raise ValueError(f"Значение канала должно быть от 1000 до 2000, получено: {value}")
        crsf_native.set_channel(channel, value)
    
    def set_channels(self, channels: List[int]):
        """
        Установить все каналы одновременно
        
        Args:
            channels: Список значений каналов (16 элементов, 1000-2000)
        """
        if len(channels) < 16:
            raise ValueError(f"Должно быть 16 каналов, получено: {len(channels)}")
        crsf_native.set_channels(channels[:16])
    
    def send_channels(self):
        """Отправить пакет каналов"""
        crsf_native.send_channels()
    
    @property
    def is_initialized(self) -> bool:
        """Проверка инициализации"""
        return self._initialized

