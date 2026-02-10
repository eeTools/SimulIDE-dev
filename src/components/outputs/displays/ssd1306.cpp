/***************************************************************************
 *   Copyright (C) 2019 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>

#include "ssd1306.h"
#include "itemlibrary.h"

#include "doubleprop.h"
#include "stringprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("Ssd1306",str)

Component* Ssd1306::construct( QString type, QString id )
{ return new Ssd1306( type, id ); }

LibraryItem* Ssd1306::libraryItem()
{
    return new LibraryItem(
        "SSD1306",
        "Displays",
        "ssd1306.png",
        "Ssd1306",
        Ssd1306::construct );
}

Ssd1306::Ssd1306( QString type, QString id )
       : OledController( type, id )
{
    m_address = m_cCode = 0b00111100; // 0x3A - 60

    m_rotate = true;

    setColorStr("White");

    setSize( 128,64 );
    setLabelPos(-32,-60, 0);

    addPropGroup( { tr("Main"), {
        new StrProp <Ssd1306>("Color",tr("Color"), "White,Blue,Yellow;"+tr("White")+","+tr("Blue")+","+tr("Yellow")
                             ,this, &Ssd1306::colorStr, &Ssd1306::setColorStr,0,"enum" ),

        new IntProp <Ssd1306>("Width", tr("Width"), "_px"
                             , this, &Ssd1306::width, &Ssd1306::setWidth, propNoCopy,"uint" ),

        new IntProp <Ssd1306>("Height", tr("Height"), "_px"
                             ,this,&Ssd1306::height, &Ssd1306::setHeight, propNoCopy,"uint" ),

        new BoolProp<Ssd1306>("Rotate", tr("Rotate"), ""
                             , this, &Ssd1306::imgRotated, &Ssd1306::setImgRotated ),
    }, 0} );

    addPropGroup( { tr("I2C"), {
        new IntProp <Ssd1306>("Control_Code", tr("I2C Address"), ""
                             , this, &Ssd1306::cCode, &Ssd1306::setCcode,0,"uint" ),

        new DoubProp<Ssd1306>("Frequency",tr("I2C Frequency"), "_kHz"
                             , this, &Ssd1306::freqKHz, &Ssd1306::setFreqKHz ),
    }, 0} );
}
Ssd1306::~Ssd1306(){}

void Ssd1306::proccessCommand()
{
    m_lastCommand = m_rxReg;
    m_readIndex = 0;
    m_readBytes = 0;

    if( m_rxReg < 0x20 )
    {
        if( m_addrMode != PAGE_ADDR_MODE ) return;

        if( m_rxReg < 0x10 )                              // Lower Colum Start Address for Page Addresing mode
            m_addrX = (m_addrX & 0xF0) | (m_rxReg & 0x0F);
        else                                              // Higher Colum Start Address for Page Addresing mode
            m_addrX = (m_addrX & 0x0F) | ((m_rxReg & 0x0F) << 4);

        if( m_addrX >= m_maxWidth) m_addrX -= m_maxWidth;
    }
    else if( m_rxReg>=0x40 && m_rxReg<=0x7F )            // Display Start Line
    {
        m_ramOffset = m_rxReg & m_lineMask;
    }
    else if( m_rxReg>=0xB0 && m_rxReg<=0xB7 )            // Page Start Address for Page Addresing mode
    {
        if( m_addrMode == PAGE_ADDR_MODE ) m_addrY = m_rxReg & m_rowMask;
    }
    else{
    switch( m_rxReg )
    {
        case 0x20: m_readBytes = 1;    break; // Memory Addressing Mode
        case 0x21: m_readBytes = 2;    break; // Column Address (Start-End)
        case 0x22: m_readBytes = 2;    break; // Page Address (Start-End)
        case 0x23: m_readBytes = 1;    break; // Fade Out / Blinking Mode
        case 0x26:                            // Continuous Horizontal Right Scroll Setup
        case 0x27: m_readBytes = 6;    break; // Continuous Horizontal Left Scroll Setup
        case 0x29:                            // Continuous Vertical and Horizontal Right Scroll Setup
        case 0x2A: m_readBytes = 5;    break; // Continuous Vertical and Horizontal Left Scroll Setup
        case 0x2C:                            // One Column Horizontal Right Scroll Setup
        case 0x2D: m_readBytes = 6;    break; // One Column Horizontal Left Scroll Setup
        case 0x2E: m_scroll = false;   break; // Deactivate scroll
        case 0x2F: m_scroll = true;           // Activate scroll
            m_scrollCount = 0;
            break;
        case 0x81: m_readBytes = 1;    break; // Contrast Control
        case 0x8D: m_readBytes = 1;    break; // Charge Pump
        case 0xA0: m_remap = false;    break; // Segment Re-map OFF
        case 0xA1: m_remap = true;     break; // Segment Re-map ON
        case 0xA3: m_readBytes = 2;    break; // Vertical Scroll Area
        case 0xA4: m_dispFull = false; break; // Entire Display Off
        case 0xA5: m_dispFull = true;  break; // Entire Display ON
        case 0xA6: m_dispInv  = false; break; // Inverse Display OFF
        case 0xA7: m_dispInv  = true;  break; // Inverse Display ON
        case 0xA8: m_readBytes = 1;    break; // Multiplex Ratio
        case 0xAE: reset();            break; // Display OFF
        case 0xAF: m_dispOn = true;    break; // Display ON
        case 0xC0: m_scanInv = false;  break; // COM Output Scan Inverted OFF
        case 0xC8: m_scanInv = true;   break; // COM Output Scan Inverted ON
        case 0xD3: m_readBytes = 1;    break; // Display Offset
        case 0xD5: m_readBytes = 1;    break; // Display Clock Divide Ratio/Oscillator Frequency
        case 0xD6: m_readBytes = 1;    break; // Zoom in Mode
        case 0xD9: m_readBytes = 1;    break; // Precharge
        case 0xDA: m_readBytes = 1;    break; // COM Pins Hardware Configuration
        case 0xDB: m_readBytes = 1;    break; // VCOM DETECT
        }
    }
}

void Ssd1306::parameter()
{
    m_readIndex++;
    if( m_readIndex > m_readBytes ) return;
    if( m_readIndex == m_readBytes ) m_readBytes = 0;

    switch( m_lastCommand )
    {
        case 0x20: m_addrMode = m_rxReg & 3; break;      // Memory Addressing Mode
        case 0x21:{                                      // Set Column Address (Start-End)
            if( m_addrMode == PAGE_ADDR_MODE ) return;
            if( m_readIndex == 1 ) m_addrX=m_startX = m_rxReg & 0x7F; // 0b01111111
            else                   m_endX   = m_rxReg & 0x7F; // 0b01111111
        }break;
        case 0x22:{                                      // Set Page Address (Start-End)
            if( m_addrMode == PAGE_ADDR_MODE ) return;
            if( m_readIndex == 1 ) m_addrY=m_startY = m_rxReg & m_rowMask; // 0b00000111
            else                   m_endY   = m_rxReg & m_rowMask; // 0b00000111
        }break;
        case 0x26:                                       // Horizontal Right Scroll Setup
        case 0x27:                                       // Horizontal Left Scroll Setup
        case 0x29:                                       // Vertical and Horizontal Right Scroll Setup
        case 0x2A: configScroll( m_lastCommand ); break; // Vertical and Horizontal Left Scroll Setup
        case 0x2C:                                       // One Column Horizontal Right Scroll Setup
        case 0x2D: m_scrollSingle = true;                // One Column Horizontal Left Scroll Setup
            configScroll( m_lastCommand-6 );             // Same than 0x26/27
            break;
        case  0xA3:{                                     // Vertical Scroll Area
            switch( m_readIndex ) {
            case 1: m_scrollTop  = m_rxReg & m_lineMask; break;
            case 2: m_scrollRows = m_rxReg & 0x7F; break;
            }
        }break;
        case  0xA8: // Multiplex Ratio
        {
            uint8_t muxRatio = m_rxReg & m_lineMask;  // 0b00111111
            if( muxRatio > 14 ) m_mr = muxRatio;
        }break;
        case 0xD3: m_dispOffset = m_rxReg & m_lineMask; break; // Display Offset Set vertical shift by COM from 0d~63d
    }
}

void Ssd1306::configScroll( uint8_t command )
{
    m_scrollV   = command > 0x27;
    m_scrollDir = command & 0b11;

    switch( m_readIndex ) {
    case 1:                                       break;
    case 2: m_scrollStartY = m_rxReg & m_rowMask; break; // Define start page address
    case 3:{                                             // Scroll step time in terms of frame frequency
        switch( m_rxReg & m_rowMask ){
        case 0: m_scrollStep = 5;
        case 1: m_scrollStep = 64;
        case 2: m_scrollStep = 128;
        case 3: m_scrollStep = 256;
        case 4: m_scrollStep = 3;
        case 5: m_scrollStep = 4;
        case 6: m_scrollStep = 25;
        case 7: m_scrollStep = 2;
        }
    }break;
    case 4: m_scrollEndY    = m_rxReg & m_rowMask; break; // Define end page address
    case 5: m_vScrollOffset = m_rxReg & m_lineMask; break; // Vertical scrolling offset
    case 6: break;
    }
}
