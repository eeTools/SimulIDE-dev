/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "thermistorbase.h"

class LibraryItem;

class Thermistor : public ThermistorBase
{
    public:
        Thermistor( QString type, QString id );
        ~Thermistor();

        static Component* construct( QString type, QString id );
        static LibraryItem* libraryItem();

        int bVal() { return m_bVal; }
        void setBval( int bval );

        int r25() { return m_r25; }
        void setR25( int r25 );

        virtual void updateStep() override;

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget *w ) override;

    private:
        int m_bVal;
        int m_r25;
};
