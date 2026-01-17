/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "gate.h"
#include "component.h"

class LibraryItem;

class PlantModel : public Gate
{
    public:
        PlantModel( QString type, QString id );
        ~PlantModel();
        
 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

 void setNumOutputs( int pins );
 void setNumInputs( int pins );
 int numPinsInOutMax;

    protected:
        void updatePath() override;
};
