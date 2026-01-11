/***************************************************************************
 *   Copyright (C) 2024 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "logicfamily.h"
#include "mainwindow.h"
#include "utils.h"
#include "proputils.h"

QMap<QString, logicFamily_t> LogicFamily::m_families;

LogicFamily::LogicFamily()
{
    m_blocked = false;
    m_enableSupply = true;

    m_supplyV  = 5.0; // Power Supply
    m_inpHighVp = 0.5;
    m_inpHighV  = 2.5;
    m_inpLowVp  = 0.5;
    m_inpLowV   = 2.5;
    m_outHighVp = 1;
    m_outHighV  = 5;
    m_outLowVp  = 0;
    m_outLowV   = 0;

    m_inpImp = 1e9;
    m_outImp = 40;

    m_delayMult = 1;
    m_delayBase = 10*1000; // 10 ns
    m_timeLH    = 3000;
    m_timeHL    = 4000;

    m_family = "Custom";
    if( m_families.isEmpty() ) getFamilies();
}
LogicFamily::~LogicFamily(){}

void LogicFamily::setInpHighV( double v )
{
    if( m_blocked ) return;
    m_inpHighVp = v/m_supplyV;
}

void LogicFamily::setInpLowV( double v )
{
    if( m_blocked ) return;
    m_inpLowVp = v/m_supplyV;
}

void LogicFamily::setOutHighV( double v )
{
    if( m_blocked ) return;
    m_outHighVp = v/m_supplyV;
}

void LogicFamily::setOutLowV( double v )
{
    if( m_blocked ) return;
    m_outLowVp = v/m_supplyV;
}

void LogicFamily::setPropDelay( double pd )
{
    if( pd < 0 ) pd = 0;
    if( pd > 1e6   ) pd = 1e6;
    m_delayBase = pd*1e12;
}

void LogicFamily::setRiseTime( double time )
{
    if( time < 1e-12 ) time = 1e-12;
    if( time > 1e6   ) time = 1e6;
    m_timeLH = time*1e12;
}

void LogicFamily::setFallTime( double time )
{
    if( time < 1e-12 ) time = 1e-12;
    if( time > 1e6   ) time = 1e6;
    m_timeHL = time*1e12;
}

void LogicFamily::setFamily( QString f )
{
    m_family = f;
    if( f == "Custom") setSupplyV( m_supplyV );
    else               setFamilyData( m_families.value( f ) );
}

void LogicFamily::setFamilyData( logicFamily_t lf )
{
    double supplyV = lf.supply;
    m_enableSupply = supplyV < 0;
    if( !m_enableSupply ) m_supplyV = supplyV;

    setSupplyV( m_supplyV );

    m_inpHighVp = lf.inpLHp;
    m_inpLowVp  = lf.inpHLp;
    m_inpImp    = lf.inpImp;

    m_outHighVp = lf.outHip;
    m_outLowVp  = lf.outLop;
    m_outImp    = lf.outImp;

    updateData();
}

void LogicFamily::updateData()
{
    m_blocked = true;

    setInpHighV( m_supplyV * m_inpHighVp );
    setInpLowV(  m_supplyV * m_inpLowVp );
    setInputImp( m_inpImp );
    setInpPullups( m_inpPullups );

    setOutHighV( m_supplyV * m_outHighVp);
    setOutLowV(  m_supplyV * m_outLowVp );
    setOutImp(   m_outImp );
    setOutPullups( m_outPullups );
    m_blocked = false;
}

void LogicFamily::getFamilies() // Static
{
    m_families["Default"] = { -1, 0.5, 0.5, 1e9, 0, 1, 0, 40, 0 };
    m_families["Custom"]  = { -1, 0.5, 0.5, 1e9, 0, 1, 0, 40, 0 };

    QString modelsFile = MainWindow::self()->getDataFilePath("logic_families.model");
    if( !QFile::exists( modelsFile ) ) return;

QString doc = fileToString( modelsFile, "LogicFamily::getFamilies()" );
auto docLines = doc.split(QChar('\n'));
for( QStringView line : docLines )
{
    QVector<propStr_t> properties = parseProps( line );
    if( properties.isEmpty() ) break;
    QString familyName = properties.takeFirst().name.toString();
    logicFamily_t family;

    for (const propStr_t& property : properties)
    {
        QStringView propName = property.name;
        double propValue = property.value.toDouble();

        if     ( propName == u"supply") family.supply = propValue;
        else if( propName == u"inpLHp") family.inpLHp = propValue;
        else if( propName == u"inpHLp") family.inpHLp = propValue;
        else if( propName == u"inpImp") family.inpImp = propValue;
        else if( propName == u"inpPul") family.inpPul = propValue;
        else if( propName == u"outHip") family.outHip = propValue;
        else if( propName == u"outLop") family.outLop = propValue;
        else if( propName == u"outImp") family.outImp = propValue;
        else if( propName == u"outPul") family.outPul = propValue;
    }
    m_families[familyName] = family;
}
}

