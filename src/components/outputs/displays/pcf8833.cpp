/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "pcf8833.h"
#include "itemlibrary.h"
#include "simulator.h"

#define tr(str) simulideTr("PCF8833",str)

Component* PCF8833::construct( QString type, QString id )
{ return new PCF8833( type, id ); }

LibraryItem* PCF8833::libraryItem()
{
    return new LibraryItem(
        "Pcf8833" ,
        "Displays",
        "pcf8833.png",
        "Pcf8833",
        PCF8833::construct );
}

PCF8833::PCF8833( QString type, QString id )
       : TftController( type, id )
       , Spi3Pins( id, this )
{
    m_graphical = true;

    m_maxWidth  = 132;
    m_maxHeight = 132;
    setDisplaySize( m_maxWidth, m_maxHeight );

    m_area = QRectF(-m_width/2-6,-m_height/2-6, m_width+12, m_height+12+8);

    m_pin.resize( 4 );
    m_pin[0] = &m_pinCS;
    m_pin[1] = &m_pinDI;
    m_pin[2] = &m_pinCK;
    m_pin[3] = &m_pinRS;

    m_clkPin = &m_pinCK;

    Simulator::self()->addToUpdateList( this );

    setLabelPos(-m_width/2+16,-m_height/2-20, 0);
    setShowId( true );
}
PCF8833::~PCF8833(){}

void PCF8833::initialize()
{
    clearDDRAM();
    displayReset();
    updateStep();
}


void PCF8833::displayReset()
{
    TftController::displayReset();
    Spi3Pins::reset();
    //m_dataBytes = 2; //16bit mode
}

void PCF8833::endTransaction()
{
    Spi3Pins::endTransaction();
    m_rxReg = m_buffer;
    if( m_isData ) dataReceived();
    else           commandReceived();
}

void PCF8833::setPixelMode()
{
    switch( m_rxReg & 0b111 )
    {
    case 2: m_dataBytes = 1; break; // 3/3/3
    case 3: m_dataBytes = 2; break; // 5/6/5
    case 5: m_dataBytes = 3; break; // 4/4/4
    }
}

void PCF8833::writeRam()
{
    m_dataIndex++;
    if( m_dataIndex > m_dataBytes ) m_dataIndex = 1;

    uint32_t buffer = m_rxReg;
    switch( m_dataBytes )
    {
        case 1:  // RRRGGGBB 3/3/3
        {
            m_data = buffer & 0b11<<(0+6);
            buffer >>= 2;
            m_data |= (buffer & 0b111)<<(8+5);
            buffer >>= 3;
            m_data |= (buffer & 0b111)<<(16+5);
            TftController::writeRam();
        }break;
        case 2:  // RRRRRGGG GGGBBBBB 5/6/5
        {
            switch( m_dataIndex ) {
            case 1:{
                m_data = (buffer & 0b111)<<(8+3+2);  // GGG---
                buffer >>= 3;
                m_data |= (buffer & 0b11111)<<(16+3);// RRRRR
            }break;
            case 2:{
                m_data |= (buffer & 0b11111)<<(0+3); // BBBBB
                buffer >>= 5;
                m_data = (buffer & 0b111)<<(8+2);    // ---GGG
                TftController::writeRam();
            }break;
            }
        }break;
        case 3:  // RRRRGGGG BBBB-RRRR GGGGBBBB 4/4/4
        {
            switch( m_dataIndex ) {
                case 1:{
                    m_colorData = (buffer & 0b1111)<<(8+4);   // GGGG
                    buffer >>= 4;
                    m_colorData |= (buffer & 0b1111)<<(16+4); // RRRR
                }break;
                case 2:{
                    m_data = m_colorData;
                    m_colorData = (buffer & 0b1111)<<(16+4);  // RRRR Next Pixel
                    buffer >>= 4;
                    m_data |= (buffer & 0b1111)<<(0+4);   // BBBB This Pixel
                    TftController::writeRam();            // First Pixel
                }break;
                case 3:{
                    m_data = m_colorData;
                    m_data |= (buffer & 0b1111)<<(0+4);   // BBBB
                    buffer >>= 4;
                    m_data |= (buffer & 0b1111)<<(8+4);   // GGGG
                    TftController::writeRam();            // Second Pixel
                }break;
            }
        }break;
    }
}
