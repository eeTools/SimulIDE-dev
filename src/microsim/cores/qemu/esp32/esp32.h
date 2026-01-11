/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "qemudevice.h"
#include "qemutwi.h"
#include "esp32pin.h"

class LibraryItem;
//class esp32Pin;
//struct funcPin;

class Esp32 : public QemuDevice
{
    public:
        Esp32( QString type, QString id, QString device );
        ~Esp32();

        void stamp() override;

    protected:
        Pin* addPin( QString id, QString type, QString label,
                    int n, int x, int y, int angle , int length=8, int space=0 ) override;

        bool createArgs() override;
        void doAction() override;
        void matrixFunc( uint8_t out );
        void createPins();
        void createMatrix();
        void createIoMux();
        void readInputs();


        uint64_t m_state;
        uint64_t m_direc;

        IoPin* m_dummyPin;
        esp32Pin* m_espPad[40];
        funcPin m_matrixIn[256];
        funcPin m_matrixOut[256];
};
