/***************************************************************************
 *   Copyright (C) 2019 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>

#include "ili9341.h"
#include "itemlibrary.h"
#include "connector.h"
#include "simulator.h"

Component* Ili9341::construct( QString type, QString id )
{ return new Ili9341( type, id ); }

LibraryItem* Ili9341::libraryItem()
{
    return new LibraryItem(
        "Ili9341" ,
        "Displays",
        "ili9341.png",
        "Ili9341",
        Ili9341::construct );
}

Ili9341::Ili9341( QString type, QString id )
       : Component( type, id )
       , eClockedDevice( id )
       , m_pinCS (  270, QPoint(-56, 184), id+"-PinCS"  , 0, this, input )
       , m_pinRst(  270, QPoint(-48, 184), id+"-PinRst" , 0, this, input )
       , m_pinDC (  270, QPoint(-40, 184), id+"-PinDC"  , 0, this, input )
       , m_pinMosi( 270, QPoint(-32, 184), id+"-PinMosi", 0, this, input )
       , m_pinSck(  270, QPoint(-24, 184), id+"-PinSck" , 0, this, input )
       //, m_pinMiso( 270, QPoint(-16, 184), id+"-PinMiso" , 0, this )
{
    m_graphical = true;
    
    m_area = QRectF( -126, -168, 252, 344 );

    m_pinCS.setLabelText( "CS" );
    m_pinCS.setInputHighV( 2.31 );
    m_pinCS.setInputLowV( 0.99 );
    m_pinRst.setLabelText( "Rst" );
    m_pinRst.setInputHighV( 2.31 );
    m_pinRst.setInputLowV( 0.99 );
    m_pinDC.setLabelText( "D/C" );
    m_pinDC.setInputHighV( 2.31 );
    m_pinDC.setInputLowV( 0.99 );
    m_pinMosi.setLabelText( "Mosi" );
    m_pinMosi.setInputHighV( 2.31 );
    m_pinMosi.setInputLowV( 0.99 );
    m_pinSck.setLabelText( "SCK" );
    m_pinSck.setInputHighV( 2.31 );
    m_pinSck.setInputLowV( 0.99 );
    //m_pinMiso.setLabelText( "Miso" );

    m_pin.resize( 5 );
    m_pin[0] = &m_pinCS;
    m_pin[1] = &m_pinRst;
    m_pin[2] = &m_pinDC;
    m_pin[3] = &m_pinMosi;
    m_pin[4] = &m_pinSck;
    //m_pin[5] = &m_pinMiso;

    m_clkPin = &m_pinSck;

    Simulator::self()->addToUpdateList( this );
    
    setLabelPos(-32,-180, 0);
    setShowId( true );
    
    Ili9341::initialize();
}

Ili9341::~Ili9341(){}

void Ili9341::stamp()
{
    m_pinSck.changeCallBack( this ); // Register for Scl changes callback
    m_pinCS.changeCallBack( this ); // Register for CS changes callback
    m_pinRst.changeCallBack( this ); // Register for Rst changes callback
}

void Ili9341::initialize()
{
    clearDDRAM();
    reset();
    Ili9341::updateStep();
}

void Ili9341::updateStep()
{
    update();
}

void Ili9341::voltChanged()
{
    bool ret = false;
    if( !m_pinRst.getInpState() )            // Reset Pin is Low
    {
        reset();
        ret = true;
    }
    if( m_pinCS.getInpState() )            // Cs Pin High: Lcd not selected
    {
        m_rxReg = 0;                       // Initialize serial buffer
        m_inBit  = 0;
        ret = true;
    }
    updateClock();

    if( m_clkState != Clock_Rising ) ret = true;
    if( ret ) return;

    m_rxReg &= ~1; //Clear bit 0
    if( m_pinMosi.getInpState() ) m_rxReg |= 1;

    if( m_inBit >= 7 )
    {
        if( m_pinDC.getInpState() )       // Write Data
        {
            if( m_readBytes == 0 )        // Write DDRAM
            {
                m_data = (m_data<<8)+m_rxReg;
                m_inByte++;
                if( m_inByte >= m_dataBytes )       // 16/18 bits ready
                {
                    m_inByte = 0;

                    uint r,g,b;
                    if( m_dataBytes == 2 ) // 16 bits format: RRRRRGGGGGGBBBBB
                    {
                        r = (m_data & 0b1111100000000000)<<8;
                        g = (m_data & 0b0000011111100000)<<5;
                        b = (m_data & 0b0000000000011111)<<3;
                    }
                    else // 18 bits format: RRRRRR00GGGGGG00BBBBBB00
                    {
                        r = (m_data & 0b111111000000000000000000);
                        g = (m_data & 0b000000001111110000000000);
                        b = (m_data & 0b000000000000000011111100);
                    }
                    m_aDispRam[m_addrX][m_addrY] = r+g+b;
                    incrementPointer();
                    m_data = 0;
                }
            }
            else getParameter();       // Write Command Parameter
        }
        else proccessCommand();        // Write Command
        m_inBit = 0;
    }else{
        m_rxReg <<= 1;
        m_inBit++;
    }
}

void Ili9341::getParameter()
{
    if( m_readBytes > 0 )
    {
        switch( m_lastCommand )
        {
            case 0x2A:   // Column Address Set
            {
                if     ( m_readBytes == 4 ) m_startX = m_rxReg<<8;
                else if( m_readBytes == 3 )
                {
                    m_startX += m_rxReg;
                    if     ( m_startX < 0 )      m_startX = 0;
                    else if( m_startX > m_maxX ) m_startX = m_maxX;
                    m_addrX = m_startX;
                }
                else if( m_readBytes == 2 ) m_endX = m_rxReg<<8;
                else{
                    m_endX += m_rxReg;
                    if     ( m_endX < 0 )        m_endX = 0;
                    else if( m_endX > m_maxX   ) m_endX = m_maxX;
                    else if( m_endX < m_startX ) m_endX = m_startX;
                }
            }break;
            case 0x2B:   // Page Address Set
            {
                if     ( m_readBytes == 4 ) m_startY = m_rxReg<<8;
                else if( m_readBytes == 3 )
                {
                    m_startY += m_rxReg;
                    if     ( m_startY < 0      ) m_startY = 0;
                    else if( m_startY > m_maxY ) m_startY = m_maxY;
                    m_addrY = m_startY;
                }
                else if( m_readBytes == 2 ) m_endY = m_rxReg<<8;
                else{
                    m_endY += m_rxReg;
                    if     ( m_endY < 0        ) m_endY = 0;
                    else if( m_endY > m_maxY   ) m_endY = m_maxY;
                    else if( m_endY < m_startY )
                        m_endY = m_startY;
                }
            }break;
            case 0x33:   // Vertical Scrolling Definition
            {
                if     ( m_readBytes == 6 ) m_TFA  = (uint16_t)m_rxReg << 8; // TFA [15:8]
                else if( m_readBytes == 5 ) m_TFA |= m_rxReg;                // TFA [7:0]
                else if( m_readBytes == 4 ) m_VSA  = (uint16_t)m_rxReg << 8; // VSA [15:8]
                else if( m_readBytes == 3 ) m_VSA |= m_rxReg;                // VSA [7:0]
                else if( m_readBytes == 2 ) m_BFA  = (uint16_t)m_rxReg << 8; // BFA [15:8]
                else                        m_BFA |= m_rxReg;                // BFA [7:0]
                /// else;                       m_BFA |= m_rxReg;               // BFA [7:0] ???
            }break;
            case 0x37:   // Vertical Scrolling Start Address, Ignored if Partial Mode
            {
                if( m_readBytes == 2 ) m_VSP = (uint16_t)m_rxReg << 8; // VSP [15:8]
                else                   m_VSP |= m_rxReg;               // VSP [7:0]
                /// else;                  m_VSP |= m_rxReg;               // VSP [7:0] ???
            }break;
            case 0x3A:
            {
                int mode = (m_rxReg>>4) & 1;
                m_dataBytes = mode ? 2 : 3;
            }break;
            case 0x36:   // Memory Access Control
            {
                m_dirY = m_rxReg & (1<<7);
                m_dirX = m_rxReg & (1<<6);

                m_rowCol = m_rxReg & (1<<5);

                m_RGB = (m_rxReg & (1<<3)) ? false :true;
            }break;
        }
        m_readBytes--;
    }
}

void Ili9341::proccessCommand()
{
    m_lastCommand = m_rxReg;
    m_readBytes = 0;

    switch( m_rxReg )
    {
        case 0x01: //   Software Reset
        {
            reset();
            //Clear variables
        }break;
        /*case 0x04: m_readBytes = 4; break; // Read Display identification information
        case 0x09: m_readBytes = 5; break; // Read Display Status
        case 0x0A: m_readBytes = 2; break; // Read Display Power Mode
        case 0x0B: m_readBytes = 2; break; // Read Display MADCTL
        case 0x0C: m_readBytes = 2; break; // Read Display Pixel Format
        case 0x0D: m_readBytes = 2; break; // Read Display Image Format
        case 0x0E: m_readBytes = 2; break; // Read Display Signal Mode
        case 0x0F: m_readBytes = 2; break; // Read Display Sek-Diagnostic Result*/
        //case 0x10: // Enter Sleep Mode
        //case 0x11: // Sleep Out
        //case 0x12: // Partial Mode On
        //case 0x13: // Normal Mode On
        case 0x20: m_dispInv = false; break; // Display Inversion Off
        case 0x21: m_dispInv = true; break;  // Display Inversion On
        case 0x26: m_readBytes = 1; break;   // Gamma Set
        case 0x28: m_dispOn = false; break;  // Display Off
        case 0x29: m_dispOn = true; break;   // Display On
        case 0x2A: m_readBytes = 4; break;   // Column Address Set
        case 0x2B: m_readBytes = 4; break;   // Page Address Set
        //case 0x2C: // Memory Write
        case 0x2D: m_readBytes = 128; break; // Color Set
        //case 0x2E: // Memory Read
        case 0x30: m_readBytes = 4; break;   // Partial Area
        case 0x33: m_readBytes = 6; break;   // Vertical Scrolling Definition
        //case 0x34: // Tearing Effect Line Off
        case 0x35: m_readBytes = 1; break;   // Tearing Effect Line On
        case 0x36: m_readBytes = 1; break;   // Memory Access Control
        case 0x37: m_readBytes = 2; break;   // Vertical Scrolling Start Address
        //case 0x38: // Idle Mode Off
        //case 0x39: // Idle Mode On
        case 0x3A: m_readBytes = 1; break;   // COLMOD: Pixel Formay Set
        //case 0x3C: // Write Memory Continue
        //case 0x3E: // Read Memory Continue
        case 0x44: m_readBytes = 2; break;   // Set Tear Scanline
        case 0x45: m_readBytes = 3; break;   // Get Scanline
        case 0x51: m_readBytes = 1; break;   // Write Display Brightness
        case 0x52: m_readBytes = 2; break;   // Read Display Brightness
        case 0x53: m_readBytes = 1; break;   // Write CTRL Display
        case 0x54: m_readBytes = 2; break;   // Write CTRL Display
        case 0x55: m_readBytes = 1; break;   // Write Content Adaptive Brightness Control
        case 0x56: m_readBytes = 2; break;   // Read Content Adaptive Brightness Control
        case 0x5E: m_readBytes = 1; break;   // Write CABC Minimum Brightness
        case 0x5F: m_readBytes = 2; break;   // Read CABC Minimum Brightness

        case 0xB0: m_readBytes = 1; break;   // RGB Interface Signal Control
        case 0xB1: m_readBytes = 2; break;   // Frame Rate Control (In Normal Mode/Full Colors:
        case 0xB2: m_readBytes = 2; break;   // Frame Rate Control (In Idle Mode/8 colors:
        case 0xB3: m_readBytes = 2; break;   // Frame Rate control (In Partial Mode/Full Colors:
        case 0xB4: m_readBytes = 1; break;   // Display Inversion Control
        case 0xB5: m_readBytes = 4; break;   // Blanking Porch Control
        case 0xB6: m_readBytes = 4; break;   // Display Function Control
        case 0xB7: m_readBytes = 1; break;   // Entry Mode Set
        case 0xB8: m_readBytes = 1; break;   // Backlight Control 1
        case 0xB9: m_readBytes = 1; break;   // Backlight Control 2
        case 0xBA: m_readBytes = 1; break;   // Backlight Control 3
        case 0xBB: m_readBytes = 1; break;   // Backlight Control 4
        case 0xBC: m_readBytes = 1; break;   // Backlight Control 5
        case 0xBE: m_readBytes = 1; break;   // Backlight Control 7
        case 0xBF: m_readBytes = 1; break;   // Backlight Control 8
        case 0xC0: m_readBytes = 1; break;   // Power Control 1
        case 0xC1: m_readBytes = 1; break;   // Power Control 2
        case 0xC5: m_readBytes = 2; break;   // VCOM Control 1
        case 0xC7: m_readBytes = 1; break;   // VCOM Control 2
        case 0xD0: m_readBytes = 2; break;   // NV Memory Write
        case 0xD1: m_readBytes = 3; break;   // NV Memory Protection Key
        case 0xD2: m_readBytes = 3; break;   // NV Memory Status Read
        case 0xD3: m_readBytes = 4; break;   // Read ID4
        case 0xDA: m_readBytes = 2; break;   // Read ID1
        case 0xDB: m_readBytes = 2; break;   // Read ID2
        case 0xDC: m_readBytes = 2; break;   // Read ID3
        case 0xE0: m_readBytes = 15; break;  // Positive Gamma Correction
        case 0xE1: m_readBytes = 15; break;  // Negative Gamma Correction
        case 0xE2: m_readBytes = 16; break;  // Digital Gamma Control 1
        case 0xE3: m_readBytes = 64; break;  // Digital Gamma Control 2
        case 0xF6: m_readBytes = 3; break;   // Interface Control

        // Misterious Commands in Adafruit_ILI9341.cpp initcmd[]
        case 0xEF: m_readBytes = 3; break; //, 0x03, 0x80, 0x02,
        case 0xCF: m_readBytes = 3; break; //, 0x00, 0xC1, 0x30,
        case 0xED: m_readBytes = 4; break; //, 0x64, 0x03, 0x12, 0x81,
        case 0xE8: m_readBytes = 3; break; //, 0x85, 0x00, 0x78,
        case 0xCB: m_readBytes = 5; break; //, 0x39, 0x2C, 0x00, 0x34, 0x02,
        case 0xF7: m_readBytes = 1; break; //, 0x20,
        case 0xEA: m_readBytes = 2; break; //, 0x00, 0x00,
        case 0xF2: m_readBytes = 1; break; //, 0x00, 0x00,
    }
    //qDebug() << "Ili9341::proccessCommand: " << command;
}

void Ili9341::clearDDRAM() 
{
    for( int row=0; row<320; row++ )
        for( int col=0; col<240; col++ )
            m_aDispRam[col][row] = 0;
}

void Ili9341::incrementPointer() 
{
    m_addrX++;
    if( m_addrX > m_endX ){
        m_addrX = m_startX;
        m_addrY++;
        if( m_addrY > m_endY ) m_addrY = m_startY;
    }
}

void Ili9341::reset() 
{
    m_dirX = 0;
    m_addrX  = 0;
    m_startX = 0;
    m_endX   = 239;
    m_maxX   = 239;

    m_dirY = 0;
    m_addrY  = 0;
    m_startY = 0;
    m_endY   = 319;
    m_maxY   = 319;

    m_rowCol = 0;

    m_inBit  = 0;
    m_inByte = 0;
    m_data   = 0;

    m_startLin = 0;
    m_readBytes = 0;
    m_dataBytes = 2; //16bit mode

    m_dispOn   = false;
    //m_dispFull = false;
    m_dispInv  = false;

    m_scroll  = false;
    m_scrollR = false;
    m_scrollV = false;
    m_RGB = true;;

    m_TFA = 0;   // Top Fixed Area
    m_VSA = 320; // Vertical Scrolling Area
    m_BFA = 0;   // Bottom Fixed Area
    m_VSP = 0;   // Vertical Scrolling Pointer

    m_lastCommand = 0;

    //m_reset = true;
}

uint Ili9341::getPixel( int row, int col )
{
    if( m_dirY ) row = 319-row;
    if( !m_dirX ) col = 239-col; // ???????????????????????
    if( m_rowCol )
    {
        int r = row;
        row = col;
        col = r;
    }
    uint pixel = m_aDispRam[col][row];
    if( m_RGB )
    {
        uint r,g,b;

        r = (m_data & 0b111111110000000000000000);
        g = (m_data & 0b000000001111111100000000);
        b = (m_data & 0b000000000000000011111111);
        pixel = (b<<16) + g + (r>>16);
    }
    return pixel;
}

void Ili9341::paint( QPainter* p, const QStyleOptionGraphicsItem*, QWidget* )
{
    p->setRenderHint( QPainter::Antialiasing, true );
    QPen pen( Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
    p->setPen( pen );
    
    p->setBrush( QColor(50, 70, 100) );
    p->drawRoundedRect( m_area,2,2 );

    if( !m_dispOn ) p->fillRect(-120,-162, 240, 320, Qt::black ); // Display Off
    else{
        QImage img( 240*2, 320*2, QImage::Format_RGB32 );
        QPainter painter;
        painter.begin( &img );
        painter.setRenderHint( QPainter::Antialiasing, true );

        for( int row=0; row<320; ++row )
        {
            int y = row*2;
            int yRAM = row;
            if( m_VSP > 0 )
            {
                if( row >= m_TFA && row < 320-m_BFA )
                {
                    int srcollEnd = m_TFA+m_VSA-1;
                    yRAM = m_VSP+row-m_TFA;
                    if     ( yRAM > srcollEnd ) yRAM -= m_VSA;
                    //else if( yRAM < 0   ) yRAM += 320;
                }
            }
            if( m_rowCol && yRAM > 239 ) continue;
            for( int col=0; col<240; ++col )
            {
                uint pixel = getPixel( row, col );
                painter.fillRect( col*2, y, 2, 2, QColor(pixel).rgb() );
            }
        }

        painter.end();
        p->drawImage(QRectF(-120,-162, 240, 320), img );
    }

    Component::paintSelected( p );
}
