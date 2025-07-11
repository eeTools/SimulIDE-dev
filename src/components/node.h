/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "component.h"
#include "pin.h"

class Node : public Component
{
    public:
        QRectF boundingRect() const override { return QRect(-4,-4, 8, 8 ); }

        Node( QString type, QString id );
        ~Node();

        Pin* getPin( int pin ) const { return m_pin[pin]; }

        virtual void registerEnode( eNode* enode, int n=-1 ) override;

        virtual void setHidden( bool hid, bool hidArea=false, bool hidLabel=false ) override;

        void pinMessage( int rem=1 ) override;
        bool checkRemove();

        bool hasCurrents();

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget ) override;

    protected:
        void contextMenuEvent( QGraphicsSceneContextMenuEvent* ) override {;}

    private:
        void joinConns( int co0, int c1);

        bool m_isBus;
};
