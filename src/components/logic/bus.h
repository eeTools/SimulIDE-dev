/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "component.h"
#include "e-element.h"

class LibraryItem;
class eNode;

class Bus : public Component, public eElement
{
    public:
        Bus( QString type, QString id );
        ~Bus();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();
        
        int numLines() { return m_numLines; }
        void setNumLines( int lines );

        int startBit() { return m_startBit; }
        void setStartBit( int bit );

        void registerEnode( eNode* enode, int n=-1 ) override;

        void paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget );
        
    protected:
        int m_height;

        int m_numLines;
        int m_startBit;
        
        Pin* m_busPin0;
        Pin* m_busPin1;
};
