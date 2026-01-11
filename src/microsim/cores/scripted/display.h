/***************************************************************************
 *   Copyright (C) 2023 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QWidget>

#include "updatable.h"
#include "e-element.h"

#include "scriptarray.h"

class MyDisplay : public QWidget, public Updatable, public eElement
{
    public:
        MyDisplay( uint width, uint height, QString name, QWidget* parent );
        ~MyDisplay();

        virtual void initialize() override;
        virtual void updateStep() override;

        void setWidth( uint w );
        void setHeight( uint h );
        void setSize( uint w, uint h );
        void setBackground( int b );
        void setBGR( bool bgr ) { m_bgr = bgr; }

        void setPixel( uint x, uint y, int color );
        void fillData( int data );
        void clear();
        void drawLine( int x0, int y0, int x1, int y1, int color );

        void setMonitorScale( double scale );
        void setEmbed( bool e );

        std::vector<std::vector<int>>* getBackData() { return &m_data; }

    protected:
        virtual void paintEvent( QPaintEvent* e ) override;

        void updtImageSize();

        bool m_changed;
        bool m_embed;
        bool m_bgr;

        uint m_width;
        uint m_height;
        uint m_newWidth;
        uint m_newHeight;

        int m_x;
        int m_y;

        double m_scale;

        int m_background;

        std::vector<std::vector<int>> m_data;
        QRectF  m_area;
};
