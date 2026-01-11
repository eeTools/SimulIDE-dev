/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QPointF>
#include <QStringView>

QPointF getPointF( QString p );

QString getStrPointF( QPointF p );


//---------------------------------------------------

struct propStr_t{
    QStringView name;
    QStringView value;
};

QVector<propStr_t> parseXmlProps( QStringView line );
QVector<propStr_t> parseProps( QStringView line );
propStr_t parseProp( QStringView token );
