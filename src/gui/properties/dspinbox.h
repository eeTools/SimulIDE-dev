/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QDoubleSpinBox>

#include "scripthelper.h"

class DSpinBox : public QDoubleSpinBox
{
    public:
        DSpinBox( QWidget* parent );
        ~DSpinBox();

        double valueFromText(const QString& text) const;
        QString textFromValue( double value ) const;

        QValidator::State validate( QString& text, int& ) const;

    private:
 static double evaluate( QString exp );
 static asIScriptEngine* m_aEngine;
};
