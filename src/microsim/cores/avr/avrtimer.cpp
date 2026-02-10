/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "avrtimer.h"
#include "datautils.h"
#include "avrocunit.h"
#include "avricunit.h"
#include "e_mcu.h"
#include "mcupin.h"
#include "simulator.h"
#include "regwatcher.h"

McuTimer* AvrTimer::createTimer( eMcu* mcu, QString name, int type ) // Static
{
    if     ( type == 800 ) return new AvrTimer800( mcu, name );
    else if( type == 801 ) return new AvrTimer801( mcu, name );
    else if( type == 810 ) return new AvrTimer810( mcu, name );
    else if( type == 820 ) return new AvrTimer820( mcu, name );
    else if( type == 821 ) return new AvrTimer821( mcu, name );
    else if( type == 822 ) return new AvrTimer822( mcu, name );
    else if( type == 160 ) return new AvrTimer16bit( mcu, name );

    return nullptr;
}

AvrTimer::AvrTimer(  eMcu* mcu, QString name )
        : McuTimer( mcu, name )
{
    m_OCA = nullptr;
    m_OCB = nullptr;
    m_OCC = nullptr;
}
AvrTimer::~AvrTimer(){}

void AvrTimer::initialize()
{
    McuTimer::initialize();

    m_ovfMatch  = m_maxCount;
    m_ovfPeriod = m_ovfMatch + 1;

    m_wgmMode = wgmNORM;
    m_wgm10Val = 0;
    m_wgm32Val = 0;
}

void AvrTimer::addOcUnit( McuOcUnit* ocUnit )
{
    m_ocUnit.emplace_back( ocUnit );

    if     ( ocUnit->getId().endsWith("A") ) m_OCA = (AvrOcUnit*)ocUnit;
    else if( ocUnit->getId().endsWith("B") ) m_OCB = (AvrOcUnit*)ocUnit;
    else if( ocUnit->getId().endsWith("C") ) m_OCC = (AvrOcUnit*)ocUnit;
    else                                     m_OCA = (AvrOcUnit*)ocUnit;
}

McuOcUnit* AvrTimer::getOcUnit( QString name )
{
    if     ( name.endsWith("A") ) return m_OCA;
    else if( name.endsWith("B") ) return m_OCB;
    else if( name.endsWith("C") ) return m_OCC;
    else                          return m_OCA;
}

void AvrTimer::configureA( uint8_t newTCCRXA ) // TCCRXA  // WGM00,WGM01
{
    if( m_OCA ) m_OCA->configure( newTCCRXA ); // Done in ocunits
    if( m_OCB ) m_OCB->configure( newTCCRXA );
    if( m_OCC ) m_OCC->configure( newTCCRXA );

    m_wgm10Val = getRegBitsVal( newTCCRXA, m_WGM10 ); // WGMX1,WGMX0
    updtWgm();
}

void AvrTimer::configureB( uint8_t newTCCRXB ) // TCCRXB
{
    updtPrescaler( newTCCRXB );

    uint8_t WGM32 = (getRegBitsVal( newTCCRXB, m_WGM32 ))<<2; // WGMX3,WGMX2
    if( m_wgm32Val != WGM32 ){ m_wgm32Val = WGM32; updtWgm(); }
}

void AvrTimer::updtPrescaler( uint8_t val )
{
    uint8_t prIndex = getRegBitsVal( val, m_prSelBits ); // CSX0-n

    if( prIndex != m_prIndex )
    {
        setPrescIndex( prIndex );
        if( m_prIndex ) configureClock();
        enable( m_prIndex );
    }
}

void AvrTimer::configureClock()
{
    m_prescaler = m_prescList.at( m_prIndex );

    if( m_prescaler >= 0x8000 )      // Ext clock
    {
        m_clkEdge = m_prescaler & 1; // 0 = falling edge, 1 = rising edge
        m_prescaler = 1;
        enableExtClock( true );
    }
    else enableExtClock( false );

    m_psPerTick = m_prescaler*m_mcu->psInst();
}

void AvrTimer::configureOcUnits( bool wgm3 )
{
    m_bidirec = false;

    ocAct_t comActA, comActB, comActC;
    ocAct_t tovActA = ocNON;
    ocAct_t tovActB = ocNON;
    ocAct_t tovActC = ocNON;

    if( m_OCA ) comActA = (ocAct_t)m_OCA->getMode(); // Default modes
    if( m_OCB ) comActB = (ocAct_t)m_OCB->getMode();
    if( m_OCC ) comActC = (ocAct_t)m_OCC->getMode();

    if( m_wgmMode == wgmPHAS )  // Phase Correct PWM
    {
        if( m_OCA ) { if((comActA == ocTOG) && wgm3 ) comActA = ocNON; }
        if( m_OCB ) { if( comActB == ocTOG ) comActB = ocNON; }
        if( m_OCC ) { if( comActC == ocTOG ) comActC = ocNON; }
        m_bidirec = true;
    }
    else  if( m_wgmMode == wgmFAST )  // Fast PWM
    {
        if( m_OCA ) {
            if((comActA == ocTOG) && wgm3 ) comActA = ocNON;
            if     ( comActA == ocCLR ) tovActA = ocSET;
            else if( comActA == ocSET ) tovActA = ocCLR;
        }
        if( m_OCB ) {
            if     ( comActB == ocTOG ) comActB = ocNON;
            else if( comActB == ocCLR ) tovActB = ocSET;
            else if( comActB == ocSET ) tovActB = ocCLR;
        }
        if( m_OCC ) {
            if     ( comActC == ocTOG ) comActC = ocNON;
            else if( comActC == ocCLR ) tovActC = ocSET;
            else if( comActC == ocSET ) tovActC = ocCLR;
    }   }
    if( m_OCA ) m_OCA->setOcActs( comActA, tovActA );
    if( m_OCB ) m_OCB->setOcActs( comActB, tovActB );
    if( m_OCC ) m_OCC->setOcActs( comActC, tovActC );

    if( m_bidirec ) m_ovfPeriod = m_ovfMatch;
    else            m_ovfPeriod = m_ovfMatch+1;
}

//--------------------------------------------------
// TIMER 8 Bit--------------------------------------

AvrTimer8bit::AvrTimer8bit( eMcu* mcu, QString name )
            : AvrTimer( mcu, name )
{
    m_maxCount = 0xFF;
}
AvrTimer8bit::~AvrTimer8bit(){}

void AvrTimer8bit::updtWgm()
{
    m_wgmMode = (wgmMode_t)m_wgm10Val;
    if( m_wgmMode != wgmPHAS ) m_reverse = false;

    configureOcUnits( !m_wgm32Val );
    topReg0Changed( *m_topReg0L );
}

void AvrTimer8bit::topReg0Changed( uint8_t val )
{
    updtCount();
    
    *m_topReg0L = val;

    uint16_t ovf = 0xFF;
    if( m_wgmMode == wgmCTC
      ||((m_wgm32Val) && ( (m_wgmMode == wgmPHAS)
                         ||(m_wgmMode == wgmFAST)) ) )
    { ovf = val; } // Top = OCRA

    if( m_ovfMatch != ovf ){
        m_ovfMatch = ovf;

        if( m_bidirec ) m_ovfPeriod = m_ovfMatch;
        else            m_ovfPeriod = m_ovfMatch+1;

        m_OCA->ocrWriteL( val );
        sheduleEvents();
    }
    else m_OCA->ocrWriteL( val );
}

//--------------------------------------------------
// TIMER 0 -----------------------------------------

AvrTimer800::AvrTimer800( eMcu* mcu, QString name)
           : AvrTimer8bit( mcu, name )
{
}
AvrTimer800::~AvrTimer800(){}

void AvrTimer800::setup()
{
    QString n = m_name.right(1);
    m_FOCA  = getRegBits("FOC"+n+"A", m_mcu );
    m_FOCB  = getRegBits("FOC"+n+"B", m_mcu );

    m_WGM10 = getRegBits("WGM"+n+"0,WGM"+n+"1", m_mcu );
    m_WGM32 = getRegBits("WGM"+n+"2", m_mcu );
}

void AvrTimer800::configureB( uint8_t newTCCRXB ) // TCCRXB
{
    AvrTimer::configureB( newTCCRXB );

    if( m_OCA && getRegBitsBool( newTCCRXB, m_FOCA ) )
    {
        m_OCA->comMatch();
        newTCCRXB &= ~m_FOCA.mask;
    }
    if( m_OCB && getRegBitsBool( newTCCRXB, m_FOCB ) )
    {
        m_OCB->comMatch();
        newTCCRXB &= ~m_FOCB.mask;
    }

    m_mcu->m_regOverride = newTCCRXB;
}

//--------------------------------------------------
// TIMER 0 -----------------------------------------

AvrTimer801::AvrTimer801( eMcu* mcu, QString name)
           : McuTimer( mcu, name )
{
    m_maxCount = 0xFF;
}
AvrTimer801::~AvrTimer801(){}

void AvrTimer801::initialize()
{
    McuTimer::initialize();

    m_ovfMatch  = m_maxCount;
    m_ovfPeriod = m_ovfMatch + 1;
}

void AvrTimer801::configureA(uint8_t newTCCRX )
{
    uint8_t prIndex = getRegBitsVal( newTCCRX, m_prSelBits ); // CSX0-n

    if( prIndex != m_prIndex )
    {
        setPrescIndex( prIndex );
        if( m_prIndex ) configureClock();
        enable( m_prIndex );
    }
}

void AvrTimer801::configureClock() // This Timer is not derived from AvrTimer
{
    m_prescaler = m_prescList.at( m_prIndex );

    if( m_prescaler >= 0x8000 )      // Ext clock
    {
        m_clkEdge = m_prescaler & 1; // 0 = falling edge, 1 = rising edge
        m_prescaler = 1;
        enableExtClock( true );
    }
    else enableExtClock( false );

    m_psPerTick = m_prescaler*m_mcu->psInst();
}

//--------------------------------------------------
// TIMER 1 (8 bits) ------- Tinyx5 T1

AvrTimer810::AvrTimer810( eMcu* mcu, QString name)
           : AvrTimer( mcu, name )
{
}
AvrTimer810::~AvrTimer810(){}

void AvrTimer810::setup()
{
    m_maxCount = 0xFF;
    QString n = m_name.right(1);
    m_FOCA  = getRegBits("FOC"+n+"A", m_mcu );
    m_FOCB  = getRegBits("FOC"+n+"B", m_mcu );

    m_CTC1  = getRegBits( "CTC1" , m_mcu );
    m_PWM1A = getRegBits( "PWM1A", m_mcu );
    m_PWM1B = getRegBits( "PWM1B", m_mcu );
    m_PSR1  = getRegBits( "PSR1" , m_mcu );

    m_oc1AiPin = m_mcu->getMcuPin("PORTB0");
    m_oc1BiPin = m_mcu->getMcuPin("PORTB3");
}

void AvrTimer810::configureA( uint8_t newTCCR1 ) // TCCR1
{
    uint8_t mode = m_mode & ~0b00000101; // Clear CTC and PWMA in mode
    if( getRegBitsBool( newTCCR1, m_CTC1 ) )  mode |= 1<<2;

    bool pwm =  getRegBitsBool( newTCCR1, m_PWM1A );
    if( pwm ) mode |= 1;

    if( m_OCA ){
        m_OCA->configure( newTCCR1 ); // Done in ocunits
        updateOcUnit( m_OCA, pwm );
    }
    if( mode != m_mode ) { m_mode = mode; updateMode(); }

    updtPrescaler( newTCCR1 );
}

void AvrTimer810::configureB( uint8_t newGTCCR ) // GTCCR
{
    uint8_t mode = m_mode & ~0b00000010; // Clear PWMB in mode
    bool pwm = getRegBitsBool( newGTCCR, m_PWM1B );
    if( pwm ) mode |= 1<<1;

    if( m_OCB ){
        m_OCB->configure( newGTCCR ); // Done in ocunits
        updateOcUnit( m_OCB, pwm );

        if( getRegBitsBool( newGTCCR, m_FOCB ) )
        {
            m_OCB->comMatch();
            newGTCCR &= ~m_FOCB.mask;
        }
    }
    if( m_OCA && getRegBitsBool( newGTCCR, m_FOCA ) )
    {
        m_OCA->comMatch();
        newGTCCR &= ~m_FOCA.mask;
    }
    if( mode != m_mode ){ m_mode = mode; updateMode(); }

    m_mcu->m_regOverride = newGTCCR & ~m_PSR1.mask; // PSR1 always read as 0
}

void AvrTimer810::updateOcUnit( McuOcUnit* ocUnit, bool pwm )
{
    McuPin* iPin = (ocUnit == m_OCA) ? m_oc1AiPin : m_oc1BiPin;

    ocAct_t comAct = (ocAct_t)ocUnit->getMode();  // Default mode
    ocAct_t tovAct = ocNON;

    if( comAct == ocTOG && pwm )
    {
        comAct = ocCLR;
        if( iPin )
        {
            iPin->controlPin( true, false );
            iPin->setOutState( false );
            ocUnit->setPinInnv( iPin );
        }
    }else if( iPin ){
        iPin->controlPin( false, false );
        ocUnit->setPinInnv( nullptr );
    }

    if     ( comAct == ocCLR ) tovAct = ocSET;
    else if( comAct == ocSET ) tovAct = ocCLR;
    ocUnit->setOcActs( comAct, tovAct );
}

void AvrTimer810::updateMode()
{
    if( m_mode ) m_ovfMatch = *m_topReg0L;// Top = OCR1C
    else         m_ovfMatch = 0xFF;

    m_ovfPeriod = m_ovfMatch + 1;
}

void AvrTimer810::topReg0Changed( uint8_t val )
{
    if( *m_topReg0L == val ) return;
    *m_topReg0L = val;

    updateMode();
}

//--------------------------------------------------
// TIMER 2 -----------------------------------------

AvrTimer820::AvrTimer820( eMcu* mcu, QString name)
           : AvrTimer8bit( mcu, name )
{
}
AvrTimer820::~AvrTimer820(){}

void AvrTimer820::setup()
{
    QString n = m_name.right(1);

    m_FOCA  = getRegBits("FOC"+n+"A", m_mcu );
    m_FOCB  = getRegBits("FOC"+n+"B", m_mcu );

    m_WGM10 = getRegBits( "WGM"+n+"0,WGM"+n+"1", m_mcu );
    m_WGM32 = getRegBits( "WGM"+n+"2", m_mcu );
}

void AvrTimer820::configureB( uint8_t newTCCRXB ) // TCCRXB
{
    AvrTimer::configureB( newTCCRXB );

    if( m_OCA && getRegBitsBool( newTCCRXB, m_FOCA ) )
    {
        m_OCA->comMatch();
        newTCCRXB &= ~m_FOCA.mask;
    }
    if( m_OCB && getRegBitsBool( newTCCRXB, m_FOCB ) )
    {
        m_OCB->comMatch();
        newTCCRXB &= ~m_FOCB.mask;
    }

    m_mcu->m_regOverride = newTCCRXB;
}

//--------------------------------------------------
// TIMER 0 -----------------------------------------

AvrTimer821::AvrTimer821( eMcu* mcu, QString name)
           : AvrTimer8bit( mcu, name )
{
}
AvrTimer821::~AvrTimer821(){}

void AvrTimer821::setup()
{
    m_wgm32Val = 0;

    QString n = m_name.right(1);
    m_FOCA = getRegBits("FOC"+n, m_mcu );
}

void AvrTimer821::configureA( uint8_t newTCCRx )
{
    if( m_OCA ) m_OCA->configure( newTCCRx ); // COM20 COM21 Done in ocunits

    updtPrescaler( newTCCRx );

    m_wgm10Val = ((( newTCCRx & 1<<6) >> 6)
                | (( newTCCRx & 1<<3) >> 2)); // WGM20 WGM21

    updtWgm();

    if( m_OCA && getRegBitsBool( newTCCRx, m_FOCA ) )
    {
        m_OCA->comMatch();
        m_mcu->m_regOverride = newTCCRx & ~m_FOCA.mask;
    }
}

//--------------------------------------------------
// TIMER 2 -----------------------------------------

AvrTimer822::AvrTimer822( eMcu* mcu, QString name) // mega32 Timer2
           : AvrTimer821( mcu, name )
{
    m_AS2 = getRegBits("AS2", mcu );
}
AvrTimer822::~AvrTimer822(){}

void AvrTimer822::voltChanged()  // External Clock Pin changed voltage
{
    if( m_sleeping ) return;
    McuTimer::voltChanged();
    if( m_outputClk ) m_outputClk->setOutState( !m_clkState ); // Opposite state than inputClk
}

void AvrTimer822::configureB( uint8_t newASSR )
{
    bool async = getRegBitsBool( newASSR, m_AS2 );
    if( m_async == async ) return;
    m_async = async;

    if( m_clockPin )
    {
        m_clockPin->setPinMode( input );
        m_clockPin->controlPin( async, async );
        enableExtClock( async );
    }
    if( m_outputClk )
    {
        m_outputClk->setPinMode( output );
        m_outputClk->setOutState( !m_clkState ); // Opposite state than inputClk
        m_outputClk->controlPin( async, async );
    }
}

//--------------------------------------------------
// TIMER 16 Bit-------------------------------------

#define OCRXA16 *m_topReg0L+(*m_topReg0H<<8)
#define ICRX16  *m_topReg1L+(*m_topReg1H<<8)

AvrTimer16bit::AvrTimer16bit( eMcu* mcu, QString name )
             : AvrTimer( mcu, name )
{
}
AvrTimer16bit::~AvrTimer16bit(){}

void AvrTimer16bit::setup()
{
    m_maxCount = 0xFFFF;

    QString n = m_name.right(1);
    m_FOCA  = getRegBits("FOC"+n+"A", m_mcu );
    m_FOCB  = getRegBits("FOC"+n+"B", m_mcu );
    m_FOCC  = getRegBits("FOC"+n+"C", m_mcu );

    m_WGM10 = getRegBits( "WGM"+n+"0,WGM"+n+"1", m_mcu );
    m_WGM32 = getRegBits( "WGM"+n+"2,WGM"+n+"3", m_mcu );

    setICRX( "ICR"+n+"L,ICR"+n+"H" );
}

void AvrTimer16bit::runEvent()            // Overflow
{
    if( !m_running ) return;

    if( m_wgmMode == wgmFAST && m_useICR && m_ICunit ) m_ICunit->getInterrupt()->raise();
    McuTimer::runEvent();
}

void AvrTimer16bit::configureC( uint8_t newTCCRXC )
{
    /// TODO: optimize?
    if( m_OCA && getRegBitsBool( newTCCRXC, m_FOCA ) )
    {
        m_OCA->comMatch();
        newTCCRXC &= ~m_FOCA.mask;
    }
    if( m_OCB && getRegBitsBool( newTCCRXC, m_FOCB ) )
    {
        m_OCB->comMatch();
        newTCCRXC &= ~m_FOCB.mask;
    }
    if( m_OCC && getRegBitsBool( newTCCRXC, m_FOCC ) )
    {
        m_OCC->comMatch();
        newTCCRXC &= ~m_FOCC.mask;
    }
    m_mcu->m_regOverride = newTCCRXC;
}

void AvrTimer16bit::updtWgm()
{
    m_wgmVal = m_wgm32Val + m_wgm10Val;

    wgmMode_t mode = wgmNORM;
    uint16_t  ovf  = 0xFFFF;
    uint16_t mask = 0xFFFF;
    m_useICR = false;

    switch( m_wgmVal ){
        case 1:  mode = wgmPHAS; ovf = mask = 0x00FF; break; // PWM, Phase Correct, 8-bit
        case 2:  mode = wgmPHAS; ovf = mask = 0x01FF; break; // PWM, Phase Correct, 9-bit
        case 3:  mode = wgmPHAS; ovf = mask = 0x03FF; break; // PWM, Phase Correct, 10-bit
        case 4:  mode = wgmCTC;  ovf = OCRXA16;       break; // CTC
        case 5:  mode = wgmFAST; ovf = mask = 0x00FF; break; // Fast PWM, 8-bit
        case 6:  mode = wgmFAST; ovf = mask = 0x01FF; break; // Fast PWM, 9-bit
        case 7:  mode = wgmFAST; ovf = mask = 0x03FF; break; // Fast PWM, 10-bit
        case 8:  mode = wgmPHAS; m_useICR = true;     break; // PWM, Phase and Frequency Correct
        case 9:  mode = wgmPHAS; ovf = OCRXA16;       break; // PWM, Phase and Frequency Correct
        case 10: mode = wgmPHAS; m_useICR = true;     break; // PWM, Phase Correct
        case 11: mode = wgmPHAS; ovf = OCRXA16;       break; // PWM, Phase Correct
        case 12: mode = wgmCTC;  m_useICR = true;     break; // CTC
        case 13:                                      break; // (Reserved)
        case 14: mode = wgmFAST; m_useICR = true;     break; // Fast PWM ICRX
        case 15: mode = wgmFAST; ovf = OCRXA16;       break; // Fast PWM OCRXA
    }
    if( m_useICR ) ovf = ICRX16;
    if( m_ICunit ) m_ICunit->enable( !m_useICR );

    m_wgmMode = mode;
    if( m_wgmMode != wgmPHAS ) m_reverse = false;

    if( m_OCA ) m_OCA->setOcrMask( mask );
    if( m_OCB ) m_OCB->setOcrMask( mask );
    if( m_OCC ) m_OCC->setOcrMask( mask );

    bool shedule = (m_ovfMatch != ovf);
    m_ovfMatch = ovf;
    bool wgm3 = (m_wgm32Val & 1<<3)==0;
    configureOcUnits( wgm3 );

    if( shedule ) sheduleEvents();
}

void AvrTimer16bit::topReg0Changed( uint8_t val )
{
    if( *m_topReg0L == val ) return;
    *m_topReg0L = val;
    updtWgm();


    m_OCA->ocrWriteL( val );
}

void AvrTimer16bit::ICRXLchanged( uint8_t val )
{
    if( *m_topReg1L == val ) return;
    *m_topReg1L = val;
    updtWgm();
}

/*void AvrTimer16bit::ICRXHchanged( uint8_t val )
{
    if( *m_topReg1H == val ) return;
    *m_topReg1H = val;
    updtWgm();
}*/

void AvrTimer16bit::setICRX( QString reg )
{
    QStringList list = reg.split(",");

    reg = list.takeFirst();
    m_topReg1L = m_mcu->getReg( reg );
    watchRegNames( reg, R_WRITE, this, &AvrTimer16bit::ICRXLchanged, m_mcu );

    reg = list.takeFirst();
    m_topReg1H = m_mcu->getReg( reg );
    /// Low byte triggers red/write operations
    /// watchRegNames( reg, R_WRITE, this, &AvrTimer16bit::ICRXLchanged, m_mcu );
}
