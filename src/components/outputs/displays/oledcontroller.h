/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <vector>

#include "twimodule.h"
#include "component.h"

#define HORI_ADDR_MODE 0
#define VERT_ADDR_MODE 1
#define PAGE_ADDR_MODE 2
#define COLU_ADDR_MODE 3

class IoPin;

class OledController : public Component, public TwiModule
{
    public:
        OledController( QString type, QString id );
        ~OledController();

        QString colorStr() { return m_dColor; }
        void setColorStr( QString color );

        int width() { return m_width; }
        void setWidth( int w );

        int height() { return m_height; }
        void setHeight( int h );

        bool imgRotated() { return m_rotate; }
        void setImgRotated( bool r ) { m_rotate = r; }

        virtual void initialize() override;
        virtual void stamp() override;
        virtual void updateStep() override;

        virtual void startWrite() override;
        virtual void readByte() override;
        
        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

    protected:
        virtual void proccessCommand(){;}
        virtual void parameter(){;}
        virtual void writeData();
        void reset();
        void clearDDRAM();
        void setSize( int w, int h );
        void updateSize();

        QString m_dColor;
        QColor m_foreground;

        IoPin* m_pinSda;

        bool m_rotate;

        uint8_t m_Co;
        uint8_t m_start;
        uint8_t m_data;
        uint8_t m_lastCommand;
        uint8_t m_readBytes;
        uint8_t m_readIndex;

        uint8_t m_width;
        uint8_t m_height;
        uint8_t m_maxWidth;
        uint8_t m_maxHeight;
        uint8_t m_rows;

        uint8_t m_addrX;     // X RAM address
        uint8_t m_addrY;     // Y RAM address
        uint8_t m_startX;
        uint8_t m_endX;
        uint8_t m_startY;
        uint8_t m_endY;
        uint8_t m_lineMask;
        uint8_t m_rowMask;

        uint8_t m_dispOffset;
        uint8_t m_ramOffset;
        uint8_t m_addrMode;

        uint8_t m_scroll;
        uint8_t m_scrollDir;
        uint8_t m_scrollV;
        uint8_t m_scrollStartY;
        uint8_t m_scrollEndY;
        uint8_t m_scrollTop;
        uint8_t m_scrollRows;
        uint8_t m_vScrollOffset;
        uint8_t m_scrollSingle;
        uint16_t m_scrollStep;
        uint16_t m_scrollCount;

        uint8_t m_dispOn;
        uint8_t m_dispFull;
        uint8_t m_dispInv;
        uint8_t m_scanInv;
        uint8_t m_remap;

        uint8_t m_mr;      // Multiplex Ratio
        //int m_cdr;       // Clock Divide Ratio
        //int m_fosc;      // Oscillator Frequency
        //int m_frm;       // Frame Frequency

        std::vector<std::vector<uint8_t>> m_DDRAM; //128x128 DDRAM
};
