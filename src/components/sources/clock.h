/***************************************************************************
 *   Copyright (C) 2017 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "clock-base.h"

class LibraryItem;

class Clock : public ClockBase
{
    public:
        Clock( QString type, QString id );
        ~Clock();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        virtual void runEvent() override;

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;
};
