/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "ui_numval.h"
#include "propval.h"

class Component;
class PropDialog;

class NumVal : public PropVal, private Ui::NumVal
{
    Q_OBJECT
    
    public:
        NumVal( PropDialog* parent, CompBase* comp, ComProperty* prop );
        ~NumVal();

        virtual void setup( bool isComp ) override;
        virtual void updtValues() override;

        virtual QString getValWithUnit() override;

    public slots:
        void on_showVal_toggled( bool checked );
        void on_valueBox_valueChanged( double val );
        void on_unitBox_currentTextChanged( QString unit );

    private:
        QString m_type;

        bool m_useMult;
};
