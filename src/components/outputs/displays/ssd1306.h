/***************************************************************************
 *   Copyright (C) 2019 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "oledcontroller.h"

class LibraryItem;

class Ssd1306 : public OledController
{
    public:
        Ssd1306( QString type, QString id );
        ~Ssd1306();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();


    protected:
        void proccessCommand() override;
        void parameter() override;
        void configScroll( uint8_t command );
};
