/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "qemuusart.h"


class Esp32Usart : public QemuUsart
{
    public:
        Esp32Usart( QemuDevice* mcu, QString name, int number );
        ~Esp32Usart();

        enum stm32UsartAction_t{
            ESP32_USART_CR0=1,
            ESP32_USART_CR1,
//          ESP32_USART_READ,
            ESP32_USART_WRITE,
            ESP32_USART_BAUD,
            ESP32_USART_ENABLE,
        };

        //void enable( bool e ) override;
        void connected( bool c ); // override;

        void doAction() override;

        void frameSent( uint8_t data ) override;

        //void endTransaction() override;

    private:
        void writeCR0( uint32_t data );
        void writeCR1( uint32_t data );
};

