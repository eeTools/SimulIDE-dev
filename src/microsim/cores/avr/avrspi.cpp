/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "avrspi.h"
#include "datautils.h"
#include "iopin.h"
#include "e_mcu.h"
#include "mcuinterrupts.h"

AvrSpi::AvrSpi( eMcu* mcu, QString name )
      : McuSpi( mcu, name )
{
}
AvrSpi::~AvrSpi(){}

void AvrSpi::setup()
{
    QString n = m_name.right(1);
    bool ok = false;
    n.toInt( &ok );
    if( !ok ) n = "";

    m_SPR   = getRegBits( "SPR"+n+"0,SPR"+n+"1", m_mcu );
    m_SPE   = getRegBits( "SPE"+n, m_mcu );
    m_DODR  = getRegBits( "DODR"+n, m_mcu );
    m_MSTR  = getRegBits( "MSTR"+n, m_mcu );
    m_CPOL  = getRegBits( "CPOL"+n, m_mcu );
    m_CPHA  = getRegBits( "CPHA"+n, m_mcu );
    m_SPI2X = getRegBits( "SPI2X"+n, m_mcu );
}

void AvrSpi::setMode( spiMode_t mode )
{
    if     ( mode == m_mode ) return;
    else if( mode == SPI_OFF )
    {
        m_MOSI->controlPin( false, false );
        m_MISO->controlPin( false, false );
        m_clkPin->controlPin( false, false );
        m_SS->controlPin( false, false );
    }
    else if( mode == SPI_MASTER )
    {
        m_MOSI->controlPin( true, false );
        m_MISO->setPinMode( input );
        m_MISO->controlPin( true, true );
        m_clkPin->controlPin( true, false );
        //m_SS->controlPin( true, false );
    }
    else if( mode == SPI_SLAVE )
    {
        m_MOSI->setPinMode( input );
        m_MOSI->controlPin( true, true );
        m_clkPin->setPinMode( input );
        m_clkPin->controlPin( true, true );
        m_SS->setPinMode( input );
        m_SS->controlPin( true, true );
        ssChanged( !m_SS->getInpState() ); // SS active LOW
    }
    SpiModule::setMode( mode );
}

void AvrSpi::ssChanged( bool enable )
{
    if( enable ) m_MISO->controlPin( true, false );
    else{
        m_MISO->setPinMode( input );
        m_MISO->controlPin( true, true );
    }
}

void AvrSpi::configureA( uint8_t newSPCR ) // SPCR is being written
{
    bool enable = getRegBitsBool( newSPCR, m_SPE );
    if( !enable )                 /// Disable SPI
    { setMode( SPI_OFF ); return; }

    bool master = getRegBitsBool( newSPCR, m_MSTR );
    spiMode_t mode = master ? SPI_MASTER : SPI_SLAVE;
    setMode( mode );

    m_lsbFirst  = getRegBitsBool( newSPCR, m_DODR ); // Data order

    bool clkPol = getRegBitsBool( newSPCR, m_CPOL ); // Clock polarity
    m_leadEdge = clkPol ? Clock_Falling : Clock_Rising;
    m_tailEdge = clkPol ? Clock_Rising  : Clock_Falling;
    m_clkPin->setOutState( clkPol );
    updateClock();

    bool clkPha = getRegBitsBool( newSPCR, m_CPHA ); // Clock phase
    m_sampleEdge = ( clkPol == clkPha ) ? Clock_Rising : Clock_Falling; // This shows up in the truth table

    uint8_t spr = getRegBitsVal( newSPCR, m_SPR );
    setPrescIndex( spr );
    updateSpeed();
}

void AvrSpi::writeStatus( uint8_t newSPSR ) // SPSR is being written
{
    uint8_t spi2x = getRegBitsVal( newSPSR, m_SPI2X );
    m_speed2x = spi2x > 0;
    updateSpeed();
    m_mcu->m_regOverride = (*m_statReg & ~m_SPI2X.mask) | spi2x; // Preserve Status bits
}

void AvrSpi::writeSpiReg( uint8_t newSPDR ) // SPDR is being written
{
    m_srReg = newSPDR;

    /// SPIF is cleared by first reading the SPI Status Register with SPIF set,
    /// then accessing the SPI Data Register (SPDR).
    m_interrupt->clearFlag();          // Clear Iterrupt flag

    if( m_mode == SPI_MASTER ) StartTransaction();
}

void AvrSpi::endTransaction()
{
    SpiModule::endTransaction();
    *m_dataReg = m_srReg;
    m_interrupt->raise();
}

void AvrSpi::updateSpeed()
{
    uint64_t div = m_speed2x ? 4 : 2;
    m_clockPeriod = m_mcu->psInst()*m_prescaler/div;
}
