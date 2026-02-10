/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "tftcontroller.h"
#include "spi3pins.h"
#include "iopin.h"

class LibraryItem;

class PCF8833 : public TftController, public Spi3Pins
{
    public:
        PCF8833( QString type, QString id );
        ~PCF8833();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        void initialize() override;

        void endTransaction() override;

    protected:
        void displayReset() override;
        void setPixelMode() override;
        void writeRam() override;
};
