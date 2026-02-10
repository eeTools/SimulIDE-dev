/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "pictimer.h"
#include "e_mcu.h"
#include "simulator.h"
#include "datautils.h"

McuTimer* PicTimer::createTimer( eMcu* mcu, QString name, int type  ) // Static
{
    if     ( type == 800 ) return new PicTimer0( mcu, name );
    else if( type == 820 ) return new PicTimer2( mcu, name );
    //else if( type == 821 ) return new AvrTimer821( mcu, name );
    else if( type == 160 ) return new PicTimer160( mcu, name );
    else if( type == 161 ) return new PicTimer161( mcu, name );
    return nullptr;
}

PicTimer::PicTimer(  eMcu* mcu, QString name )
        : McuTimer( mcu, name )
{
    m_sleepMode = 0xFF;
}
PicTimer::~PicTimer(){}

void PicTimer::initialize()
{
    McuTimer::initialize();

    m_ovfMatch  = m_maxCount;
    m_ovfPeriod = m_ovfMatch + 1;
}

/*void PicTimer::addOcUnit( McuOcUnit* ocUnit )
{
}

McuOcUnit* PicTimer::getOcUnit( QString name )
{
    return nullptr;
}*/

void PicTimer::configureA( uint8_t val )
{
}

void PicTimer::configureB( uint8_t val )
{
}

/*void PicTimer::configureClock()
{
    m_prescaler = m_prescList.at( m_mode );
    m_clkSrc = clkMCU;
}

void PicTimer::configureExtClock()
{
    m_prescaler = 1;
    m_clkSrc = clkEXT;
    /// if     ( m_mode == 6 ) m_clkEdge = Clock_Falling;
    /// else if( m_mode == 7 ) m_clkEdge = Clock_Rising;
}*/

//--------------------------------------------------
// TIMER 8 Bit--------------------------------------

PicTimer8bit::PicTimer8bit( eMcu* mcu, QString name )
            : PicTimer( mcu, name )
{
    m_maxCount = 0xFF;
}
PicTimer8bit::~PicTimer8bit(){}


//--------------------------------------------------
// TIMER 0 -----------------------------------------

PicTimer0::PicTimer0( eMcu* mcu, QString name)
         : PicTimer8bit( mcu, name )
{
}
PicTimer0::~PicTimer0(){}

void PicTimer0::setup()
{
    m_T0CS = getRegBits("T0CS", m_mcu );
    m_T0SE = getRegBits("T0SE", m_mcu );
    m_PSA  = getRegBits("PSA", m_mcu );
    m_PS   = getRegBits("PS0,PS1,PS2", m_mcu );
}

void PicTimer0::initialize()
{
    PicTimer::initialize();

    m_running = true;
    sheduleEvents();
}

void PicTimer0::configureA( uint8_t NewOPTION )
{
    uint8_t ps = getRegBitsVal( NewOPTION, m_PS );

    if( getRegBitsBool( NewOPTION, m_PSA ) )
         m_prescaler = 1;                    // Prescaler asigned to Watchdog
    else setPrescIndex( ps );                // Prescaler asigned to TIMER0

    m_psPerTick = m_prescaler*m_mcu->psInst();

    m_clkEdge = getRegBitsVal( NewOPTION, m_T0SE );

    uint8_t mode = getRegBitsVal( NewOPTION, m_T0CS );
    if( mode != m_mode )
    {
        m_mode = mode;
        //enable( mode==0 );
        enableExtClock( mode );
    }
    else sheduleEvents();
}

//--------------------------------------------------
// TIMER 2 -----------------------------------------

PicTimer2::PicTimer2( eMcu* mcu, QString name)
         : PicTimer8bit( mcu, name )
{
}
PicTimer2::~PicTimer2(){}

void PicTimer2::setup()
{
    m_TMR2ON = getRegBits("TMR2ON", m_mcu );
    m_T2CKPS = getRegBits("T2CKPS0,T2CKPS1", m_mcu );
    m_TOUTPS = getRegBits("TOUTPS0,TOUTPS1,TOUTPS2,TOUTPS3", m_mcu );
}

void PicTimer2::configureA( uint8_t NewT2CON )
{
    uint8_t presc = getRegBitsVal( NewT2CON, m_T2CKPS );
    uint8_t postc = getRegBitsVal( NewT2CON, m_TOUTPS );

    setPrescIndex( presc );
    m_psPerTick = m_prescaler* (postc+1)*m_mcu->psInst();

    bool en = getRegBitsBool( NewT2CON, m_TMR2ON );
    if( en != m_running ) enable( en );
}

void PicTimer2::configureB( uint8_t NewPR2 )
{
    m_ovfMatch  = NewPR2;
}

//--------------------------------------------------
// TIMER 16 Bit-------------------------------------


PicTimer16bit::PicTimer16bit( eMcu* mcu, QString name )
             : PicTimer( mcu, name )
{
    m_maxCount = 0xFFFF;
}
PicTimer16bit::~PicTimer16bit(){}

void PicTimer16bit::setup()
{
    m_T1CKPS = getRegBits("T1CKPS0,T1CKPS1", m_mcu );
    m_T1OSCEN = getRegBits("T1OSCEN", m_mcu );
    m_T1SYNC  = getRegBits("T1SYNC", m_mcu );

    m_TMR1ON = getRegBits("TMR1ON", m_mcu );
}

void PicTimer16bit::configureA( uint8_t NewT1CON )
{
    m_t1sync = getRegBitsVal( NewT1CON, m_T1SYNC ) ? 0 : 1; // Used for sleep mode

    uint8_t ps = getRegBitsVal( NewT1CON, m_T1CKPS );

    setPrescIndex( ps );

    m_mode  = getRegBitsVal(  NewT1CON, m_TMR1CS );
    m_t1Osc = getRegBitsBool( NewT1CON, m_T1OSCEN ) && m_mode; // T1 osc depends on TMR1CS

    configureClock();

    bool en = getRegBitsBool( NewT1CON, m_TMR1ON );
    if( en != m_running ) enable( en );
}

void PicTimer16bit::sheduleEvents()
{
    if( m_running && m_t1Osc ) /// TODO: RC oscillator // 32.768 KHz Oscillator
    {
        uint64_t circTime = Simulator::self()->circTime();
        m_psPerTick = 30517578; // Sim cycs per Timer tick for 32.768 KHz

        uint32_t ovfPeriod = m_ovfPeriod;
        if( m_countVal > m_ovfMatch ) ovfPeriod += m_maxCount; // OVF before counter: next OVF missed

        uint64_t time2ovf = (ovfPeriod-m_countVal)*m_psPerTick; // cycles in ps
        if( m_timeOffset ) time2ovf -= m_psPerTick-m_timeOffset;

        uint64_t ovfTime = circTime + time2ovf;// Absolute simulation time (ps) when OVF will occur

        if( m_ovfTime != ovfTime )
        {
            m_ovfTime = ovfTime;
            Simulator::self()->cancelEvents( this );
            Simulator::self()->addEvent( time2ovf, this );
        }
    }
    else McuTimer::sheduleEvents();
}

void PicTimer16bit::sleep( int mode )
{
    m_sleepMode = m_mode ? (m_t1sync & m_mode) : 0xFF;
    McuTimer::sleep( mode );
}

//--------------------------------------------------
// TIMER 1 -----------------------------------------

PicTimer160::PicTimer160( eMcu* mcu, QString name)
           : PicTimer16bit( mcu, name )
{
}
PicTimer160::~PicTimer160(){}

void PicTimer160::setup()
{
    PicTimer16bit::setup();

    m_TMR1CS = getRegBits("TMR1CS", m_mcu );
}

void PicTimer160::configureClock()
{
    m_psPerTick = m_prescaler*m_mcu->psInst();
    enableExtClock( m_mode == 1 );
}

//--------------------------------------------------
// TIMER 1 16f1826 ---------------------------------

PicTimer161::PicTimer161( eMcu* mcu, QString name)
           : PicTimer16bit( mcu, name )
{
}
PicTimer161::~PicTimer161(){}

void PicTimer161::setup()
{
    PicTimer16bit::setup();

    m_TMR1CS = getRegBits("TMR1CS0,TMR1CS1", m_mcu );
}

void PicTimer161::configureClock()
{
    switch( m_mode ) {
    case 0:
        m_psPerTick = m_prescaler*m_mcu->psInst();
        enableExtClock( false );
        break;
    case 1:
        m_psPerTick = m_prescaler*m_mcu->psInst()/4;
        enableExtClock( false );
        break;
    case 2:
        if( m_t1Osc ) ; /// TODO: If T1OSCEN = 1: Crystal oscillator on T1OSI/T1OSO pins
        else          enableExtClock( true );
        break;
    case 3: break;
        /// TODO: Timer1 clock source is Capacitive Sensing Oscillator (CAPOSC)
    }


}

