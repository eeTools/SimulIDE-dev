/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "oledcontroller.h"

class LibraryItem;

class Sh1107 : public OledController
{
    public:
        Sh1107( QString type, QString id );
        ~Sh1107();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        bool xoffset() { return m_xOffset; }
        void setXoffset( bool o ) { m_xOffset = o; }

    protected:
        void proccessCommand() override;
        void parameter() override;
        void writeData() override;

        bool m_xOffset;
};
