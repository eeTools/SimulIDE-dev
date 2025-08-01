/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "qemudevice.h"
#include "qemutwi.h"

class LibraryItem;

class Esp32 : public QemuDevice
{
    public:
        Esp32( QString type, QString id );
        ~Esp32();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        void stamp() override;

    protected:
        bool createArgs() override;
        void doAction() override;
        void createPins();
        void readInputs();
        void setPinFunction( IoPin* ioPin, int func );

        uint64_t m_state;
        uint64_t m_direc;

        uint64_t m_pullUps;
        uint64_t m_pullDown;
        uint64_t m_inputEn;

        QemuTwi m_i2c[2];
};
