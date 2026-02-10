/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "st77xx.h"
#include "circuit.h"
#include "simulator.h"

#include "intprop.h"
#include "doubleprop.h"

#define tr(str) simulideTr("St77xx",str)

St77xx::St77xx( QString type, QString id )
      : TftController( type, id )
      , Spi5Pins( id, this )
{
    m_graphical = true;

    m_pin.resize( 6 );
    m_pin[0] = &m_pinDC;
    m_pin[1] = &m_pinCS;
    m_pin[2] = &m_pinDI;
    m_pin[3] = &m_pinCK;
    m_pin[4] = &m_pinRS;
    m_pin[5] = &m_pinDO;

    setLabelPos(-m_width/2+16,-m_height/2-20, 0);
    setShowId( true );

    addPropGroup( { tr("Main"), {
        new IntProp<St77xx>("Width", tr("Width"), "_px"
                            , this, &St77xx::width, &St77xx::setWidth, propNoCopy,"uint" ),

        new IntProp<St77xx>("Height", tr("Height"), "_px"
                            ,this,&St77xx::height, &St77xx::setHeight, propNoCopy,"uint" ),

        new DoubProp<St77xx>("Scale" , tr("Scale"),""
                             , this, &St77xx::scale, &St77xx::setScale )
    }, 0} );
}
St77xx::~St77xx(){}


void St77xx::initialize()
{
    SpiModule::initialize();
    clearDDRAM();
    displayReset();
    updateStep();
}

void St77xx::endTransaction()
{
    Spi5Pins::endTransaction();
    m_rxReg = m_buffer;
    if( m_isData ) dataReceived();
    else           commandReceived();
}

void St77xx::displayReset()
{
    TftController::displayReset();
    Spi5Pins::reset();
    //m_dataBytes = 2; //16bit mode
}

void St77xx::setPixelMode()
{
    m_pixelMode = m_rxReg & 0b111;

    switch( m_pixelMode )
    {
    case 3: m_dataBytes = 3; break; // 4/4/4
    case 5: m_dataBytes = 2; break; // 5/6/5
    case 6: m_dataBytes = 3; break; // 6/6/6
    }
}

void St77xx::writeRam()
{
    m_dataIndex++;
    if( m_dataIndex > m_dataBytes ) m_dataIndex = 1;

    uint32_t buffer = m_rxReg;
    switch( m_pixelMode )
    {
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
        case 5:  // RRRRRGGG GGGBBBBB 5/6/5
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
                    m_data |= (buffer & 0b111)<<(8+2);    // ---GGG
                    TftController::writeRam();
                }break;
            }
        }break;
        case 6:  //  RRRRRR--GGGGGG--BBBBBB-- 6/6/6
        {
            switch( m_dataIndex ) {
                case 1:{
                    m_data = (buffer & 0b11111100) << 16;   // RRRRRR--
                }break;
                case 2:{
                    m_data = (buffer & 0b11111100) <<  8;   // GGGGGG--
                }break;
                case 3:{
                    m_data = (buffer & 0b11111100) <<  0;   // BBBBBB--
                    TftController::writeRam();
                }break;
            }
        }break;
    }
}

void St77xx::setWidth( int w )
{
    if     ( w > m_maxWidth ) w = m_maxWidth;
    else if( w <  32        ) w = 32;
    if( m_width == w ) return;

    setDisplaySize( w, m_height );
    updateSize();
}

void St77xx::setHeight( int h )
{
    if     ( h > m_maxHeight ) h = m_maxHeight;
    else if( h <  16         ) h = 16;

    if( m_height == h ) return;

    //m_rows = h/8;
    setDisplaySize( m_width, h );
    updateSize();
}

void St77xx::setScale( double s )
{
    m_scale = s;
    m_scaledWidth = (double)m_width * m_scale;
    m_scaledHeight = (double)m_height * m_scale;
    updateSize();
}

void St77xx::updateSize()
{
    double y = m_scaledHeight/2 + 22;

    m_pinDC.setY( y );
    m_pinRS.setY( y );
    m_pinCS.setY( y );
    m_pinDI.setY( y );
    m_pinCK.setY( y );
    m_pinDO.setY( y );

    m_pinDC.setX(-20 );
    m_pinRS.setX(-12 );
    m_pinCS.setX( -4 );
    m_pinDI.setX(  4 );
    m_pinCK.setX( 12 );
    m_pinDO.setX( 20 );

    double h = m_scaledHeight/2+12+10;
    double space = m_pinDC.y() - h + 0.1;

    m_pinDC.setSpace( space );
    m_pinRS.setSpace( space );
    m_pinCS.setSpace( space );
    m_pinDI.setSpace( space );
    m_pinCK.setSpace( space );
    m_pinDO.setSpace( space );

    m_pinDC.isMoved();
    m_pinRS.isMoved();
    m_pinCS.isMoved();
    m_pinDI.isMoved();
    m_pinCK.isMoved();
    m_pinDO.isMoved();

    m_area = QRectF(-m_scaledWidth/2-6,-m_scaledHeight/2-6, m_scaledWidth+12, m_scaledHeight+12+10);

    Circuit::self()->update();
}
