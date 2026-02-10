/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

//#include <QDebug>
#include <QPainter>

#include "tftcontroller.h"
#include "simulator.h"
#include "circuitwidget.h"

TftController::TftController( QString type, QString id )
             : Component( type, id )
{
    m_graphical = true;
    m_isILI = false;

    m_maxX = 0;
    m_maxY = 0;
    m_scale = 1;

    Simulator::self()->addToUpdateList( this );
}

void TftController::displayReset()
{
    m_mirrorX = 0;
    m_addrX  = 0;
    m_startX = 0;
    m_endX   = m_maxX;

    m_mirrorY = 0;
    m_addrY  = 0;
    m_startY = 0;
    m_endY   = m_maxY;

    m_swapXY = 0;
    m_icEor  = 0;
    //m_mirrorLine = 0;

    m_dispOn   = false;
    //m_dispFull = false;
    m_dispInv  = false;

    m_scroll  = false;
    m_scrollR = false;
    m_scrollV = false;
    m_BGR = false;

    m_TFA = 0;      // Top Fixed Area
    m_BFA = 0;      // Bottom Fixed Area
    m_VSP = 0;      // Vertical Scrolling Pointer
    m_VSA = m_maxY; // Vertical Scrolling Area

    m_dataIndex = 0;
    m_data   = 0;

    m_lastCommand = 0;
    m_readBytes = 0;

    //m_reset = true;
}
TftController::~TftController(){}


void TftController::updateStep()
{
    update();
}

void TftController::commandReceived()
{
    //qDebug() << "TftController::commandReceived" << QString::number( m_rxReg, 16 ).toUpper() << m_rxReg;
    m_lastCommand = m_rxReg;
    m_dataIndex = 0;
    m_readBytes = 0;

    switch( m_rxReg )
    {
    case 0x00: displayReset();           break; //   NOP
    case 0x01: displayReset();           break; //   Software Reset
    /*case 0x04: m_readBytes = 4; break; // Read Display identification information
        case 0x09: m_readBytes = 5; break; // Read Display Status
        case 0x0A: m_readBytes = 2; break; // Read Display Power Mode
        case 0x0B: m_readBytes = 2; break; // Read Display MADCTL
        case 0x0C: m_readBytes = 2; break; // Read Display Pixel Format
        case 0x0D: m_readBytes = 2; break; // Read Display Image Format
        case 0x0E: m_readBytes = 2; break; // Read Display Signal Mode
        case 0x0F: m_readBytes = 2; break; // Read Display Sek-Diagnostic Result*/
    case 0x10:                              break; /// TODO Enter Sleep Mode
    case 0x11:                              break; /// TODO Sleep Out
    case 0x12:                              break; /// TODO Partial Mode On
    case 0x13:                              break; /// TODO Normal Mode On
    case 0x20: m_dispInv = false;           break; // Display Inversion Off
    case 0x21: m_dispInv = true;            break; // Display Inversion On
    case 0x26: m_readBytes = 1;             break; // Gamma Set
    case 0x28: m_dispOn = false;            break; // Display Off
    case 0x29: m_dispOn = true;             break; // Display On
    case 0x2A: m_readBytes = 2*m_addrBytes; break; // Column Address Set
    case 0x2B: m_readBytes = 2*m_addrBytes; break; // Page Address Set
    case 0x2C: m_readBytes = -1;                   // Memory Write (until next command)
        m_addrX = m_startX;
        m_addrY = m_startY;
        break;
    case 0x2D: m_readBytes = 128;           break; /// FIXME // Color Set
    case 0x2E: // Memory Read
    case 0x30: m_readBytes = 2*m_addrBytes; break;  // Partial Area
    case 0x33: m_readBytes = 3*m_addrBytes; break;  // Vertical Scrolling Definition
    //case 0x34: // Tearing Effect Line Off
    case 0x35: m_readBytes = 1;             break;   // Tearing Effect Line On
    case 0x36: m_readBytes = 1;             break;   // Memory Access Control
    case 0x37: m_readBytes = 2;             break;   // Vertical Scrolling Start Address
    //case 0x38: // Idle Mode Off
    //case 0x39: // Idle Mode On
    case 0x3A: m_readBytes = 1;             break;   // COLMOD: Pixel Formay Set
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
    case 0xCB: m_readBytes = 5; break;   /// Power control A: 0x39, 0x2C, 0x00, 0x34, 0x02,
    case 0xCF: m_readBytes = 3; break;   /// Power control B: 0x00, 0xC1, 0x30,
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
    case 0xE8: m_readBytes = 3; break;   /// Driver timing control A: 0x85, 0x00, 0x78,
    case 0xE9: m_readBytes = 3; break;   // Equalize time control
    case 0xEA: m_readBytes = 2; break;   /// Driver timing control B: 0x00, 0x00,
    case 0xEC: m_readBytes = 1; break;   // Program Mode Control
    case 0xED: m_readBytes = 4; break;   /// Power on sequence control: 0x64, 0x03, 0x12, 0x81,
    case 0xEF: m_readBytes = 3; break;   /// 0x03, 0x80, 0x02, // Misterious Commands in Adafruit_ILI9341.cpp initcmd[]
    case 0xF2: m_readBytes = 1; break;   /// Enable 3G: 0x00, 0x00,
    case 0xF6: m_readBytes = 3; break;   // Interface Control
    case 0xF7: m_readBytes = 1; break;   /// Pump ratio control 0x20,
    case 0xFA: m_readBytes = 4; break;   // Program Mode Enable
    case 0xFC: m_readBytes = 2; break;   // NVM Setting
    case 0xFE: m_readBytes = 2; break;   // Program action
    default: qDebug() << "TftController::proccessCommand: Not implemented" << m_lastCommand;
    }
}

void TftController::dataReceived()
{
    //qDebug() << "TftController::dataReceived" << m_rxReg;
    uint16_t buffer = m_rxReg;

    if( !m_readBytes ) return;

    switch( m_lastCommand )
    {
        case 0x2A:   // Column Address Set
        {
            if( m_addrBytes == 1 )
            {
                if     ( m_readBytes == 2 ) setStartX( buffer );
                else if( m_readBytes == 1 ) setEndX( buffer );
            }else{
                if     ( m_readBytes == 4 ) m_data = buffer<<8;
                else if( m_readBytes == 3 ) setStartX( m_data | buffer );
                else if( m_readBytes == 2 ) m_data = buffer<<8;
                else if( m_readBytes == 1 ) setEndX( m_data | buffer );
            }
        }break;
        case 0x2B:   // Page Address Set
        {
            if( m_addrBytes == 1 )
            {
                if     ( m_readBytes == 2 ) setStartY( buffer );
                else if( m_readBytes == 1 ) setEndY( buffer );
            }else{
                if     ( m_readBytes == 4 ) m_data = buffer<<8;
                else if( m_readBytes == 3 ) setStartY( m_data | buffer );
                else if( m_readBytes == 2 ) m_data = buffer<<8;
                else if( m_readBytes == 1 ) setEndY( m_data | buffer );
            }
        }break;
        case 0x2C: writeRam(); break;  // RAMWR: Memory Write. Overriden by displays
        case 0x33:                     // Vertical Scrolling Definition
        {
            if     ( m_readBytes == 6 ) m_TFA  = buffer << 8; // TFA [15:8]
            else if( m_readBytes == 5 ) m_TFA |= buffer;      // TFA [7:0]
            else if( m_readBytes == 4 ) m_VSA  = buffer << 8; // VSA [15:8]
            else if( m_readBytes == 3 ) m_VSA |= buffer;      // VSA [7:0]
            else if( m_readBytes == 2 ) m_BFA  = buffer << 8; // BFA [15:8]
            else if( m_readBytes == 1 ) m_BFA |= buffer;      // BFA [7:0]
        }break;
        case 0x37:   // Vertical Scrolling Start Address, Ignored if Partial Mode
        {
            if( m_readBytes == 2 ) m_VSP = buffer << 8; // VSP [15:8]
            else                   m_VSP |= buffer;     // VSP [7:0]
        }break;
        case 0x3A: setPixelMode(); break; // COLMOD: Pixel Format Set
        case 0x36:{                       // MADCTL: Memory Access Control
            m_BGR     = buffer & 1<<3;
            m_swapXY  = buffer & 1<<5;
            m_mirrorX = buffer & 1<<6;
            m_mirrorY = buffer & 1<<7;
            //qDebug() << "MX"<<m_mirrorX<<"MY"<<m_mirrorY<<"MV"<<m_swapXY;

            if(  m_isILI )  // ILI9341 seems to work this way
            {
                m_BGR     = (buffer & 1<<3) == 0;

                // m_dataBytes == 2
                //if( m_swapXY ) m_mirrorY = (buffer & 1<<7) == 0;
                //m_mirrorX = (buffer & 1<<6) == 0;
            }
        }break;
        case 0xF6:   // Interface Control m_readBytes = 3;
        {
            if(  m_isILI ) // value of MADCTL is derived as exclusive OR
            {
                if( m_readBytes == 3 ){
                    m_icEor = buffer;
                }
            }

        }break;
    }
    m_readBytes--;
}

void TftController::writeRam() // Memory Write. Overriden by displays
{
    uint32_t addrX = m_swapXY ? m_addrY : m_addrX;
    uint32_t addrY = m_swapXY ? m_addrX : m_addrY;

    addrX = m_mirrorX ? m_maxX-addrX : addrX;
    addrY = m_mirrorY ? m_maxY-addrY : addrY;

    if( addrX > m_maxX ) addrX -= m_maxX+1;
    if( addrY > m_maxY ) addrY -= m_maxY+1;

    m_DDRAM[addrX][addrY] = m_data;

    m_addrX++;
    if( m_addrX > m_endX ){
        m_addrX = m_startX;
        m_addrY++;
        if( m_addrY > m_endY ) m_addrY = m_startY;
    }
}

void TftController::setStartX( uint16_t sx )
{
    m_startX = sx;
    if( m_startX > m_maxX ) m_startX = m_maxX;
    m_addrX = m_startX;
}

void TftController::setEndX( uint16_t ex )
{
    m_endX = ex;
    if     ( m_endX > m_maxX   ) m_endX = m_maxX;
    else if( m_endX < m_startX ) m_endX = m_startX;
}

void TftController::setStartY( uint16_t sy )
{
    m_startY = sy;
    if( m_startY > m_maxY ) m_startY = m_maxY;
    m_addrY = m_startY;
}

void TftController::setEndY( uint16_t ey )
{
    m_endY = ey;
    if     ( m_endY > m_maxY   ) m_endY = m_maxY;
    else if( m_endY < m_startY ) m_endY = m_startY;
}

uint32_t TftController::getPixel( int col, int row )
{
    uint32_t pixel = m_DDRAM[col][row];
    if( m_BGR ){
        uint32_t r = (pixel & 0xFF0000);
        uint32_t g = (pixel & 0x00FF00);
        uint32_t b = (pixel & 0x0000FF);
        pixel = (b<<16) | g | (r>>16);
    }
    return pixel;
}

void TftController::clearDDRAM()
{
    for( int row=0; row<=m_maxY; row++ )
        for( int col=0; col<=m_maxX; col++ )
            m_DDRAM[col][row] = 0;
}

void TftController::setRamSize( int x, int y )
{
    if( x > 256 || y > 256 ) m_addrBytes = 2;
    else                     m_addrBytes = 1;

    if( m_maxX == 0 ) m_DDRAM.resize( x, std::vector<uint32_t>(y, 0) ); // Resize only first time

    m_maxX = x-1;
    m_maxY = y-1;
}

void TftController::setDisplaySize( int x, int y )
{
    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    m_image = QImage( x*2, y*2, QImage::Format_RGB32 );

    m_width  = x;
    m_height = y;
    m_scaledWidth = (double)m_width * m_scale;
    m_scaledHeight = (double)m_height * m_scale;

    setRamSize( x, y );
}

void TftController::printImage()
{
    QPainter painter;
    painter.begin( &m_image );
    painter.setRenderHint( QPainter::Antialiasing, true );

    for( int row=0; row<m_height; ++row )
    {
        int y = row*2;
        int yRAM = row;
        if( m_VSP > 0 )
        {
            if( row >= m_TFA && row < m_width-m_BFA )
            {
                int srcollEnd = m_TFA+m_VSA-1;
                yRAM = m_VSP+row-m_TFA;
                if     ( yRAM > srcollEnd ) yRAM -= m_VSA;
                //else if( yRAM < 0   ) yRAM += 320;
            }
        }
        for( int col=0; col<m_width; ++col )
        {
            uint32_t pixel = getPixel( col, row );
            painter.fillRect( col*2, y, 2, 2, QColor(pixel).rgb() );
        }
    }
    painter.end();
}

void TftController::paint( QPainter* p, const QStyleOptionGraphicsItem*, QWidget* )
{
    p->setRenderHint( QPainter::Antialiasing, true );
    QPen pen( Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
    p->setPen( pen );

    p->setBrush( QColor(50, 70, 100) );
    p->drawRoundedRect( m_area,2,2 );

    p->setPen( QColor( 10, 10, 10 ) );
    p->drawEllipse( -m_scaledWidth/2-3,-m_scaledHeight/2-3, 2, 2);

    QRectF imgRect = QRectF(-m_scaledWidth/2,-m_scaledHeight/2, m_scaledWidth, m_scaledHeight );

    if( !m_dispOn ) p->fillRect( imgRect, Qt::black ); // Display Off
    else{
        printImage();

        p->drawImage( imgRect, m_image );
    }
    Component::paintSelected( p );
}
