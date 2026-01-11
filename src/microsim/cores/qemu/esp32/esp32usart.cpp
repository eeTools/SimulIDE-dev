/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QDebug>

#include "esp32usart.h"
#include "qemudevice.h"
#include "usarttx.h"
#include "usartrx.h"
#include "iopin.h"
#include "simulator.h"

#define QEMU_USART_SENT 0x04  // needed to be able to compile the program

Esp32Usart::Esp32Usart( QemuDevice* mcu, QString name, int number )
          : QemuUsart( mcu, name, number )
{
    //m_prescList = {2,4,8,16,32,64,128,256};

}
Esp32Usart::~Esp32Usart(){}

void Esp32Usart::connected( bool c )
{
    enable( c );
    if( c ) m_sender->getPin()->setPinMode( output );
}

void Esp32Usart::doAction()
{
    ////if( !m_enabled ) enable( true );
    //uint8_t  action = m_arena->data8;
    //uint32_t  data  = m_arena->data32;
    ////qDebug() << "Esp32Usart::doAction Uart:"<< m_number << "action:"<< action<< "data:" << data;
    //switch( action ) {
    //    case ESP32_USART_CR0:   writeCR0( data ); break;
    //    case ESP32_USART_CR1:   writeCR1( data ); break;
    //  //case ESP32_USART_READ:  /*readByte( data );*/ break;
    //    case ESP32_USART_WRITE:{ //qDebug() << "Esp32Usart::doAction send Byte:"<<m_number<<QChar(data);
    //        sendByte( data );
    //        //uint64_t nextTime = Simulator::self()->circTime()+m_sender->getFrameTime();
    //        //qDebug() << "Esp32Usart::doAction send Byte:"<< nextTime;
    //        //m_arena->qemuEvent = nextTime;
    //        //m_device->addEvent( nextTime, this );
    //    }break;
    //    case ESP32_USART_BAUD:  setBaudRate( data ); break; //qDebug() << "Esp32 Usart" << m_number <<"Baudrate:"<< data; break;

    //    default: break;
    //}
}

void Esp32Usart::writeCR0( uint32_t data )
{
    uint8_t parityOdd = (data & 1<< 0) ? 1 : 0;
    uint8_t parityEn  = (data & 1<< 1) ? 1 : 0;

    if( parityEn ) m_parity = parityOdd ? parODD : parEVEN;
    else           m_parity = parNONE;

    setDataBits( (data & 0b001100) >> 2 );
    m_stopBits = (data & 0b110000) >> 4;

    //uint8_t enableRx = (data & 1<< 2) ? 1 : 0;
    //uint8_t enableTx = (data & 1<< 3) ? 1 : 0;
    //uint8_t enabled  = (data & 1<<13) ? 1 : 0;

    //m_receiver->enable( enabled && enableRx );
    //m_sender->enable( enabled && enableTx );
    //qDebug() << "Stm32Usart::writeCR1"<< data << enabled << enableRx << enableTx;
}

void Esp32Usart::writeCR1( uint32_t data )
{

}

void Esp32Usart::frameSent( uint8_t data )
{
    QemuUsart::frameSent( data );

    //qDebug() << "Esp32Usart::frameSent"<< m_number<<Simulator::self()->circTime();

    //m_device->cancelEvents( this );
    //while( m_arena->qemuAction )        // Wait for previous action executed
    //{
    //    ; /// TODO: add timeout
    //}

    //m_arena->mask8  = QEMU_USART_SENT;
    //m_arena->data8  = m_number;
    //m_arena->qemuAction = SIM_USART;
}

//void Esp32Usart::endTransaction()
//{
//    SpiModule::endTransaction();
//    m_dataReg = m_srReg;
//    //m_interrupt->raise();
//}
