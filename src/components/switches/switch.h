/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "switch_base.h"

class LibraryItem;

class Switch : public SwitchBase
{
    public:
        Switch( QString type, QString id );
        ~Switch();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        void stamp() override;

        bool checked();
        void setChecked( bool c );

        void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

        virtual void keyEvent( QString key, bool pressed ) override;
        void setLinkedValue( double v, int i=0 ) override;
        bool evaluateCompoundCondition(const QString& rawCondition, int value);
};
