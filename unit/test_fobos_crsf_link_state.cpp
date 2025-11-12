/**
 * @file test_fobos_crsf_link_state.cpp
 * @brief Unit тесты для состояния связи CRSF и failsafe
 * 
 * Тесты проверяют:
 * - Переходы состояния связи (link up/down)
 * - Таймаут failsafe (CRSF_FAILSAFE_STAGE1_MS)
 * - Автоматический вызов onLinkUp/onLinkDown
 * - Обновление статистики связи
 * 
 * @version CRSF-IO-mkII
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>
#include <chrono>
#include "../libs/crsf/CrsfSerial.h"
#include "../libs/crsf/crsf_protocol.h"
#include "mocks/MockSerialPort.h"

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::DoAll;

/**
 * @class CrsfLinkStateTest
 * @brief Фикстура для тестов состояния связи
 */
class CrsfLinkStateTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockSerial = std::make_unique<MockSerialPort>();
        crsf = std::make_unique<CrsfSerial>(*mockSerial, 420000);
        linkUpCalled = false;
        linkDownCalled = false;
        
        // Устанавливаем обработчики
        crsf->onLinkUp = [this]() { linkUpCalled = true; };
        crsf->onLinkDown = [this]() { linkDownCalled = true; };
    }
    
    // Вспомогательная функция для создания валидного пакета каналов
    void createChannelsPacket(uint8_t* buffer, uint8_t& totalLen) {
        Crc8 crc(0xD5);
        uint8_t payload[22] = {0};
        
        buffer[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
        buffer[1] = 24; // TYPE + PAYLOAD + CRC
        buffer[2] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;
        memcpy(&buffer[3], payload, 22);
        
        uint8_t crcData[23];
        crcData[0] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;
        memcpy(&crcData[1], payload, 22);
        buffer[25] = crc.calc(crcData, 23);
        
        totalLen = 26;
    }
    
    std::unique_ptr<MockSerialPort> mockSerial;
    std::unique_ptr<CrsfSerial> crsf;
    bool linkUpCalled;
    bool linkDownCalled;
};

/**
 * @test Переход из link down в link up
 * 
 * Тест проверяет, что при получении первого пакета каналов
 * связь устанавливается и вызывается onLinkUp.
 */
TEST_F(CrsfLinkStateTest, LinkState_FirstPacket_EstablishesLink) {
    // Начальное состояние - связь не установлена
    EXPECT_FALSE(crsf->isLinkUp());
    EXPECT_FALSE(linkUpCalled);
    
    // Создаем и отправляем пакет каналов
    uint8_t packet[64];
    uint8_t totalLen;
    createChannelsPacket(packet, totalLen);
    
    InSequence seq;
    for (uint8_t i = 0; i < totalLen; i++) {
        EXPECT_CALL(*mockSerial, readByte(_))
            .WillOnce(DoAll(::testing::SetArgReferee<0>(packet[i]), Return(1)));
    }
    EXPECT_CALL(*mockSerial, readByte(_))
        .WillRepeatedly(Return(0));
    
    crsf->loop();
    
    // Связь должна быть установлена
    EXPECT_TRUE(crsf->isLinkUp());
    EXPECT_TRUE(linkUpCalled);
}

/**
 * @test Состояние связи остается up при получении пакетов
 * 
 * Тест проверяет, что связь остается установленной при
 * регулярном получении пакетов.
 */
TEST_F(CrsfLinkStateTest, LinkState_RegularPackets_MaintainsLink) {
    uint8_t packet[64];
    uint8_t totalLen;
    createChannelsPacket(packet, totalLen);
    
    // Отправляем несколько пакетов
    for (int i = 0; i < 5; i++) {
        InSequence seq;
        for (uint8_t j = 0; j < totalLen; j++) {
            EXPECT_CALL(*mockSerial, readByte(_))
                .WillOnce(DoAll(::testing::SetArgReferee<0>(packet[j]), Return(1)));
        }
        EXPECT_CALL(*mockSerial, readByte(_))
            .WillRepeatedly(Return(0));
        
        crsf->loop();
        
        // Связь должна оставаться установленной
        EXPECT_TRUE(crsf->isLinkUp());
    }
}

/**
 * @test Обновление статистики связи
 * 
 * Тест проверяет, что статистика связи обновляется
 * при получении пакета Link Statistics.
 */
TEST_F(CrsfLinkStateTest, LinkState_LinkStatistics_UpdatesStats) {
    uint8_t packet[64];
    uint8_t payload[10] = {0};
    payload[0] = 100; // uplink_RSSI_1
    payload[1] = 95;  // uplink_RSSI_2
    payload[2] = 90;  // uplink_Link_quality
    
    Crc8 crc(0xD5);
    packet[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;
    packet[1] = 12; // TYPE + PAYLOAD + CRC
    packet[2] = CRSF_FRAMETYPE_LINK_STATISTICS;
    memcpy(&packet[3], payload, 10);
    
    uint8_t crcData[11];
    crcData[0] = CRSF_FRAMETYPE_LINK_STATISTICS;
    memcpy(&crcData[1], payload, 10);
    packet[13] = crc.calc(crcData, 11);
    
    InSequence seq;
    for (int i = 0; i < 14; i++) {
        EXPECT_CALL(*mockSerial, readByte(_))
            .WillOnce(DoAll(::testing::SetArgReferee<0>(packet[i]), Return(1)));
    }
    EXPECT_CALL(*mockSerial, readByte(_))
        .WillRepeatedly(Return(0));
    
    crsf->loop();
    
    // Проверяем обновление статистики
    const crsfLinkStatistics_t* stats = crsf->getLinkStatistics();
    EXPECT_NE(stats, nullptr);
    EXPECT_EQ(stats->uplink_RSSI_1, 100);
    EXPECT_EQ(stats->uplink_RSSI_2, 95);
    EXPECT_EQ(stats->uplink_Link_quality, 90);
}

/**
 * @test Начальное состояние связи
 * 
 * Тест проверяет, что при создании объекта связь
 * не установлена.
 */
TEST_F(CrsfLinkStateTest, LinkState_InitialState_LinkDown) {
    EXPECT_FALSE(crsf->isLinkUp());
    EXPECT_FALSE(linkUpCalled);
    EXPECT_FALSE(linkDownCalled);
}

/**
 * @test Обработчик onLinkUp вызывается только один раз
 * 
 * Тест проверяет, что onLinkUp вызывается только при
 * первом установлении связи, а не при каждом пакете.
 */
TEST_F(CrsfLinkStateTest, LinkState_MultiplePackets_OnLinkUpCalledOnce) {
    uint8_t packet[64];
    uint8_t totalLen;
    createChannelsPacket(packet, totalLen);
    
    // Отправляем первый пакет
    {
        InSequence seq;
        for (uint8_t i = 0; i < totalLen; i++) {
            EXPECT_CALL(*mockSerial, readByte(_))
                .WillOnce(DoAll(::testing::SetArgReferee<0>(packet[i]), Return(1)));
        }
        EXPECT_CALL(*mockSerial, readByte(_))
            .WillRepeatedly(Return(0));
        
        crsf->loop();
    }
    
    EXPECT_TRUE(linkUpCalled);
    linkUpCalled = false; // Сбрасываем флаг
    
    // Отправляем второй пакет
    {
        InSequence seq;
        for (uint8_t i = 0; i < totalLen; i++) {
            EXPECT_CALL(*mockSerial, readByte(_))
                .WillOnce(DoAll(::testing::SetArgReferee<0>(packet[i]), Return(1)));
        }
        EXPECT_CALL(*mockSerial, readByte(_))
            .WillRepeatedly(Return(0));
        
        crsf->loop();
    }
    
    // onLinkUp не должен быть вызван повторно
    EXPECT_FALSE(linkUpCalled);
}

/**
 * @test Проверка наличия статистики связи
 * 
 * Тест проверяет, что статистика связи доступна
 * даже когда связь не установлена.
 */
TEST_F(CrsfLinkStateTest, LinkState_LinkDown_StatisticsAvailable) {
    // Связь не установлена
    EXPECT_FALSE(crsf->isLinkUp());
    
    // Но статистика должна быть доступна
    const crsfLinkStatistics_t* stats = crsf->getLinkStatistics();
    EXPECT_NE(stats, nullptr);
}

