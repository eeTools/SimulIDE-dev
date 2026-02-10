/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "picwdt.h"
#include "e_mcu.h"
#include "cpu8bits.h"
#include "simulator.h"
#include "datautils.h"

PicWdt* PicWdt::createWdt( eMcu* mcu, QString name, int type )
{
    switch( type ){
    case 01: return new PicWdt01( mcu, name ); break;
    default: return new PicWdt00( mcu, name ); break;
    }
}

PicWdt::PicWdt( eMcu* mcu, QString name )
      : McuWdt( mcu, name )
{
    m_clkPeriod = 18*1e9; // 18 ms

    m_sleepMode = 0xFF;
}
PicWdt::~PicWdt(){}

void PicWdt::initialize()
{
    m_wdtSoft = false;
    m_ovfInter = false;
    m_ovfReset = false;
    m_prescaler = 0;
    m_ovfPeriod = m_clkPeriod/m_prescList[ m_prescaler ];
    m_ovfReset = true;
}

void PicWdt::runEvent()
{
    if( m_sleeping )
    {
        m_mcu->sleep( false );
        m_mcu->cpu()->exitSleep(); // TO bit in the Status register is cleared
    }
    else
    {
        if( !(m_wdtFuse || m_wdtSoft) ) return;

        if( m_ovfInter ) m_interrupt->raise();
        if( m_ovfReset )
        {
            qDebug() << "McuWdt::runEvent - Watchdog Reset\n";
            m_mcu->cpu()->reset();
        }
        if( m_ovfPeriod ) Simulator::self()->addEvent( m_ovfPeriod, this );
    }
}

void PicWdt::reset()
{
    Simulator::self()->cancelEvents( this );

    if( !m_ovfPeriod ) return;
    if( m_wdtFuse || m_wdtSoft )
        Simulator::self()->addEvent( m_ovfPeriod, this );
}

void PicWdt::sleep( int mode )
{
    McuModule::sleep( mode );
    reset();
}

//------------------------------------------------------
//-- PIC Wdt Type 00 -----------------------------------

PicWdt00::PicWdt00( eMcu* mcu, QString name )
        : PicWdt( mcu, name )
{
}
PicWdt00::~PicWdt00(){}

void PicWdt00::setup()
{
    m_PS  = getRegBits("PS0, PS1, PS2", m_mcu );
    m_PSA = getRegBits("PSA", m_mcu );
}

void PicWdt00::configureA( uint8_t newOPTION ) // OPTION Written
{
    if( !m_wdtFuse ) return;

    uint8_t prIndex = m_prIndex;

    if( getRegBitsVal( newOPTION, m_PSA ) )
        prIndex = getRegBitsVal( newOPTION, m_PS );  // Prescaler asigned to Watchdog
    else prIndex = 0;                                // Prescaler asigned to TIMER0

    setPrescIndex( prIndex );
    m_ovfPeriod = m_clkPeriod*m_prescaler;
}

//------------------------------------------------------
//-- PIC Wdt Type 01 -----------------------------------

PicWdt01::PicWdt01( eMcu* mcu, QString name )
    : PicWdt( mcu, name )
{
}
PicWdt01::~PicWdt01(){}

void PicWdt01::setup()
{
    m_SWDTEN = getRegBits("SWDTEN", m_mcu );
    m_WDTPS  = getRegBits("WDTPS0,WDTPS1,WDTPS2,WDTPS3", m_mcu );
}

void PicWdt01::configureA( uint8_t newWDTCON ) // WDTCON Written
{
    m_wdtSoft   = getRegBitsBool( newWDTCON, m_SWDTEN );
    setPrescIndex( getRegBitsVal( newWDTCON, m_WDTPS ) );
    m_ovfPeriod = m_clkPeriod*m_prescaler;
}
