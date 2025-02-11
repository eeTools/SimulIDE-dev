/***************************************************************************
 *   Copyright (C) 2022 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "e-reactive.h"
#include "e-pin.h"
#include "e-node.h"
#include "simulator.h"
#include "analogclock.h"

eReactive::eReactive( QString id )
         : eResistor( id )
{
    m_value    = 0;
    m_InitCurr = 0;
    m_InitVolt = 0;

    AnalogClock::self()->addClkElement( this );
}
eReactive::~eReactive()
{
    AnalogClock::self()->remClkElement( this );
}

void eReactive::stamp()
{
    eResistor::stamp();

    if( m_ePin[0]->isConnected() && m_ePin[1]->isConnected())
    {
        m_ePin[0]->createCurrent();
        m_ePin[1]->createCurrent();

        updtReactStep();

        m_volt = m_InitVolt;
        m_curSource = m_InitCurr;
        m_curSource = updtCurr();

        if( m_curSource )
        {
            m_ePin[0]->stampCurrent( m_curSource );
            m_ePin[1]->stampCurrent(-m_curSource );
        }
        //m_ePin[0]->changeCallBack( this );
        //m_ePin[1]->changeCallBack( this );
    }
    //m_running = false;
    //Simulator::self()->addEvent( m_timeStep, this );

    updtReactStep();
}

/*void eReactive::voltChanged()
{
    if( m_running ) return;
    m_running = true;
    //runEvent();
    //Simulator::self()->addEvent( m_timeStep, this );
}*/

void eReactive::runEvent()
{
    double volt = m_ePin[0]->getVoltage() - m_ePin[1]->getVoltage();

    if( m_volt != volt )
    {
        m_volt = volt;
        m_curSource = updtCurr();

        //qDebug() << "eReactive::runEvent " << volt << m_curSource;

        m_ePin[0]->stampCurrent( m_curSource );
        m_ePin[1]->stampCurrent(-m_curSource );

    }
    //else m_running = false;
    /// Simulator::self()->addEvent( m_timeStep, this );
}

void eReactive::updtReactStep()
{
    m_timeStep = AnalogClock::self()->getStep(); // Time in ps
    m_tStep = (double)m_timeStep/1e12;         // Time in seconds
    eResistor::setResistance( updtRes() );

    //m_running = false;
    //Simulator::self()->cancelEvents( this );
    //Simulator::self()->addEvent( m_timeStep, this );
}

