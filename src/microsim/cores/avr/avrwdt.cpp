/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "avrwdt.h"
#include "e_mcu.h"
#include "cpu8bits.h"
#include "simulator.h"
#include "datautils.h"

AvrWdt* AvrWdt::createWdt( eMcu* mcu, QString name, int type )
{
    switch ( type ){
    case 01: return new AvrWdt01( mcu, name ); break;
    default: return new AvrWdt00( mcu, name ); break;
    }
}

AvrWdt::AvrWdt( eMcu* mcu, QString name )
      : McuWdt( mcu, name )
{
}
AvrWdt::~AvrWdt(){}

void AvrWdt::setup()
{
    m_WDCE = getRegBits("WDCE", m_mcu );
    m_WDE  = getRegBits("WDE", m_mcu );
    m_WDP02 = getRegBits("WDP0,WDP1,WDP2", m_mcu );

    m_WDRF = getRegBits("WDRF", m_mcu );
}

void AvrWdt::initialize()
{
    m_ovfInter = false;
    m_ovfReset = false;
    m_disabled = true;
    m_allowChanges = false;
    m_prescaler = 1;
    setPrescIndex( 0 );
    m_ovfPeriod = m_clkPeriod/m_prescaler;
}

void AvrWdt::runEvent()
{
    if( !m_wdtFuse && m_disabled ) return;

    if( m_allowChanges )
    {
        clearRegBits( m_WDCE );
        m_allowChanges = false;
    }
    else // McuWdt::runEvent();
    {
        if( m_ovfInter ) m_interrupt->raise();
        else if( m_ovfReset ) // No interrupt, just Reset
        {
            qDebug() << "AvrWdt::runEvent - Watchdog Reset\n";
            m_mcu->cpu()->reset();
        }
        Simulator::self()->addEvent( m_ovfPeriod, this );
    }
}

void AvrWdt::callBack() // WDT Overflow Interrupt just executed
{
    if( !m_wdtFuse && m_disabled ) return;
    qDebug() << "AvrWdt::callBack - Watchdog Reset\n";
    m_mcu->cpu()->reset();
}

void AvrWdt::configureA( uint8_t newWDTCSR ) // WDTCSR Written
{
    bool WDE  = getRegBitsBool( newWDTCSR, m_WDE );
    bool WDCE = getRegBitsBool( newWDTCSR, m_WDCE );

    if( WDCE && WDE )     // Allow WDP & WDE changes for next 4 cycles
    {
        m_allowChanges = true;
        Simulator::self()->cancelEvents( this );
        Simulator::self()->addEvent( 4*m_mcu->psInst(), this );
    }
    else if( m_allowChanges && !WDCE ) // WDP & WDE changes allowed
    {
        updtPrescaler( newWDTCSR );
        m_ovfPeriod  = m_clkPeriod/m_prescaler;
        m_ovfReset   = WDE;
        wdtEnable();
        runEvent();
        return;
    }
    if( m_ovfInter && !m_allowChanges ) wdtEnable();
}

void AvrWdt::wdtEnable()
{
    m_disabled = !(m_ovfInter || m_ovfReset);
    Simulator::self()->cancelEvents( this );
    if( m_wdtFuse || !m_disabled )
    {
        Simulator::self()->addEvent( m_ovfPeriod, this );
        // In Iterrupt + reset, first execute Interrupt, then reset
        if( m_interrupt ) m_interrupt->exitCallBack( this, m_ovfInter && m_ovfReset );
    }
}

void AvrWdt::reset()
{
    setRegBits( m_WDRF ); // MCUSR.WDRF
    Simulator::self()->cancelEvents( this );
    if( m_wdtFuse || !m_disabled ) Simulator::self()->addEvent( m_ovfPeriod, this );
}

//------------------------------------------------------
//-- AVR WDT Type 00 -----------------------------------

AvrWdt00::AvrWdt00( eMcu* mcu, QString name )
        : AvrWdt( mcu, name )
{
}
AvrWdt00::~AvrWdt00(){}

void AvrWdt00::setup()
{
    AvrWdt::setup();

    m_clkPeriod = 8.192*1e12; // 1048576 cycles * 7812500 ps (128 KHz)

    m_WDIF = getRegBits("WDIF", m_mcu );
    m_WDIE = getRegBits("WDIE", m_mcu );
    m_WDP3 = getRegBits("WDP3", m_mcu );
}

void AvrWdt00::configureA( uint8_t newWDTCSR ) // WDTCSR Written
{
    bool clearWdif = getRegBitsVal( newWDTCSR, m_WDIF );
    if( clearWdif )  /// Writting 1 to WDIF clears the flag
    {
        newWDTCSR &= ~m_WDIF.mask; // Clear WDIF flag
    }
    m_ovfInter = getRegBitsVal( newWDTCSR, m_WDIE );

    AvrWdt::configureA( newWDTCSR );

    if( !m_allowChanges ) // WDP & WDE changes not allowed, keep old values
    {
        newWDTCSR = overrideBits( newWDTCSR, m_WDE );   // Keep old WDE
        newWDTCSR = overrideBits( newWDTCSR, m_WDP02 ); // Keep old WDP
        newWDTCSR = overrideBits( newWDTCSR, m_WDP3 );  // Keep old WDP
        m_mcu->m_regOverride = newWDTCSR;
    }
}

void AvrWdt00::updtPrescaler( uint8_t newWDTCSR )
{
    uint8_t prIndex = getRegBitsVal( newWDTCSR, m_WDP02 );
    prIndex |= getRegBitsVal( newWDTCSR, m_WDP3 ) << 3;

    setPrescIndex( prIndex );
}

//------------------------------------------------------
//-- AVR WDT Type 01 -----------------------------------

AvrWdt01::AvrWdt01( eMcu* mcu, QString name )
        : AvrWdt( mcu, name )
{
    m_clkPeriod = 2.097152*1e12; // 2097152 cycles * 1000000 ps (1 MHz)
}
AvrWdt01::~AvrWdt01(){}

void AvrWdt01::configureA( uint8_t newWDTCSR ) // WDTCSR Written
{
    AvrWdt::configureA( newWDTCSR );

    if( !m_allowChanges ) // WDP & WDE changes not allowed, keep old values
    {
        newWDTCSR = overrideBits( newWDTCSR, m_WDE );   // Keep old WDE
        newWDTCSR = overrideBits( newWDTCSR, m_WDP02 ); // Keep old WDP
        m_mcu->m_regOverride = newWDTCSR;
    }
}

void AvrWdt01::updtPrescaler( uint8_t newWDTCSR )
{
    setPrescIndex( getRegBitsVal( newWDTCSR, m_WDP02 ) );
}
