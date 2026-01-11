/***************************************************************************
 *   Copyright (C) 2023 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>
#include <math.h>

#include "display.h"
#include "simulator.h"

MyDisplay::MyDisplay( uint w, uint h, QString name, QWidget* parent )
       : QWidget( parent )
       , Updatable()
       , eElement( name )
{
    m_width  = m_newWidth  = w;
    m_height = m_newHeight = h;
    m_scale  = 1;

    m_background= 0;
    m_changed = false;
    m_embed   = true;
    m_bgr     = false;

    updtImageSize();

    Simulator::self()->addToUpdateList( this );
}
MyDisplay::~MyDisplay(){}

void MyDisplay::initialize()
{
    m_x = 0;
    m_y = 0;
}

void MyDisplay::updateStep()
{
    if( m_changed )
    {
        m_changed = false;
        m_width  = m_newWidth;
        m_height = m_newHeight;
        updtImageSize();
    }
    update();
}

void MyDisplay::setWidth( uint w )
{
    if( m_width == w || w < 1 ) return;
    m_newWidth = w;
    m_changed = true;
}

void MyDisplay::setHeight( uint h )
{
    if( m_height == h || h < 1 ) return;
    m_newHeight = h;
    m_changed = true;
}

void MyDisplay::setSize( uint w, uint h )
{
    if( w < 1 || h < 1 ) return;
    m_newWidth  = w;
    m_newHeight = h;
    m_changed = true;
}

void MyDisplay::setMonitorScale( double scale )
{
    if( scale <= 0 ) return;
    m_scale = scale;

    updtImageSize();

    show();
}

void MyDisplay::setEmbed( bool e )
{
    m_embed = e;
    this->setVisible( !e );
}

void MyDisplay::setBackground( int b )
{
    m_background = b;
}

void MyDisplay::fillData( int data )
{
    for( uint x=0; x<m_width; x++ )
        for( uint y=0; y<m_height; y++ )
            m_data[x][y] = data;
}

void MyDisplay::clear()
{
    fillData( m_background );
}

void MyDisplay::drawLine( int x0, int y0, int x1, int y1, int color )
{
    int dx = fabs( x1-x0 );
    int dy = fabs( y1-y0 );

    int stepX = x0 < x1 ? 1 : -1;
    int stepY = y0 < y1 ? 1 : -1;

    int err = dx - dy;

    while( true )
    {
        setPixel( x0, y0, color );

        if( x0 == x1 && y0 == y1 ) break;

        int e2 = 2 * err;

        if( e2 > -dy ){
            err -= dy;
            x0 += stepX;
        }
        if( e2 < dx ){
            err += dx;
            y0 += stepY;
        }
    }
}

void MyDisplay::setPixel( uint x, uint y, int color )
{
    if( x >= m_width || y >= m_height ) return;

    if( m_bgr )
    {
        int b = color &0x0000FF;
        int r = color>>16 & 0x0000FF;

        color &= 0x00FF00;
        color |= b<<16 | r;
    }
    m_data[x][y] = color;
}

void MyDisplay::updtImageSize()
{
    m_data.clear();
    m_data.resize( m_width, std::vector<int>(m_height, m_background) );
    this->setFixedSize( m_width*m_scale, m_height*m_scale );
}

void MyDisplay::paintEvent( QPaintEvent* )
{
    if( m_embed ) return;
    QPainter p(this);

    for( uint x=0; x<m_width; x++ )
        for( uint y=0; y<m_height; y++ )
            p.fillRect( QRectF( x*m_scale, y*m_scale, m_scale, m_scale ), QColor(m_data[x][y]) );
}

