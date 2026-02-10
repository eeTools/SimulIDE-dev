/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "avrtwi.h"
#include "mcupin.h"
#include "e_mcu.h"
#include "mcuinterrupts.h"
#include "datautils.h"

AvrTwi::AvrTwi( eMcu* mcu, QString name )
      : McuTwi( mcu, name )
{
}
AvrTwi::~AvrTwi(){}

void AvrTwi::setup()
{
    QString n = m_name.right(1);
    bool ok = false;
    n.toInt( &ok );
    if( !ok ) n = "";

    m_TWCR = m_mcu->getReg("TWCR"+n );
    //m_TWSR = m_mcu->getReg( "TWSR" );

    m_TWEN  = getRegBits("TWEN"+n , m_mcu );
    m_TWWC  = getRegBits("TWWC"+n , m_mcu );
    m_TWSTO = getRegBits("TWSTO"+n, m_mcu );
    m_TWSTA = getRegBits("TWSTA"+n, m_mcu );
    m_TWEA  = getRegBits("TWEA"+n , m_mcu );
    m_TWINT = getRegBits("TWINT"+n, m_mcu );
}

void AvrTwi::initialize()
{
    McuTwi::initialize();
    m_bitRate = 0;
}

void AvrTwi::configureA( uint8_t newTWCR ) // TWCR is being written
{
    bool oldEn  = getRegBitsBool( *m_TWCR, m_TWEN );
    bool enable = getRegBitsBool( newTWCR, m_TWEN );

    if( oldEn && !enable )                 /// Disable TWI
    {
        setMode( TWI_OFF );
        m_sda->controlPin( false, false ); // Release control of MCU PIns
        m_scl->controlPin( false, false );
    }
    if( !enable ) return;                  // Not enabled, do nothing

    if( !oldEn )                           /// Enable TWI if it was disabled
    {
        m_sda->setPinMode( openCo );
        m_sda->controlPin( true, true ); // Get control of MCU PIns
        m_scl->setPinMode( openCo );
        m_scl->controlPin( true, true );
    }

    bool oldStart = getRegBitsBool( *m_TWCR, m_TWSTA );
    bool newStart = getRegBitsBool( newTWCR, m_TWSTA );
    if( newStart && !oldStart )            /// Generate Start Condition
    {
        if( m_mode != TWI_MASTER ) setMode( TWI_MASTER );
        masterStart();
    }

    bool oldStop = getRegBitsBool( *m_TWCR, m_TWSTO );
    bool newStop = getRegBitsBool( newTWCR, m_TWSTO );
    if( newStop && !oldStop )              /// Generate Stop Condition
    {
        if( m_mode == TWI_MASTER ) // Master: Stop if I2C was started
        {/// DONE if Stop and Start at same time, then Start Condition should be sheduled
            if( !newStart && getStaus() < TWI_NO_STATE ) masterStop();//I2Cstop();
        }
        else setMode( TWI_SLAVE ); // Slave: Stop Cond restarts Slave mode (can be used to recover from an error condition)
    }

    bool clearTwint = getRegBitsBool( newTWCR, m_TWINT );
    bool twea = getRegBitsBool( newTWCR, m_TWEA );
    if( !newStop && !newStart && !clearTwint )
    {
        if( m_mode != TWI_SLAVE ) setMode( TWI_SLAVE );
        m_enabled = twea;
    }

    bool data = clearTwint && !newStop && !newStart; // No start or stop and TWINT cleared, receive data
    if( !data ) return;

    if( m_mode == TWI_MASTER )
    {
        if( m_twiState == TWI_MRX_ADR_ACK    // We sent Slave Address + R and received ACK
         || m_twiState == TWI_MRX_DATA_ACK ) // We sent data and received ACK
            masterRead( twea );              // Read a byte and send ACK/NACK
    }
    else if( m_mode == TWI_SLAVE )
    {
        if( m_twiState == TWI_STX_ADR_ACK )   // Own Slave Address + R received and ACK sent
            slaveWrite();                     // Start Slave transmission
    }
}

void AvrTwi::configureB( uint8_t val )       // TWBR is being written
{
    if( m_bitRate == val ) return;
    m_bitRate = val;
    updateFreq();
}

void AvrTwi::writeAddrReg( uint8_t newTWAR ) // TWAR is being written
{
    m_genCall = newTWAR & 1;
    m_address = newTWAR >> 1;
}

void AvrTwi::writeStatus( uint8_t newTWSR ) // TWSR Status Register is being written
{
    newTWSR &= 0b00000011;
    if( m_prIndex != newTWSR ) { setPrescIndex( newTWSR ); updateFreq(); }
    /// Done by masking //m_mcu->m_regOverride = newTWSR | (*m_statReg & 0b11111100); // Preserve Status bits
}

void AvrTwi::writeTwiReg( uint8_t newTWDR ) // TWDR is being written
{
    if( m_mode == TWI_SLAVE ) m_txReg = newTWDR;
    if( m_mode != TWI_MASTER ) return;

    bool twint = getRegBitsBool( *m_TWCR, m_TWINT ); // Check if TWINT is set
    if( twint )                  // If set clear TWWC
    {
        clearRegBits( m_TWWC );  // Clear Write Collision bit TWWC
    }
    else                         // If not, the access will be discarded
    {
        setRegBits( m_TWWC );    // set Write Collision bit TWWC
        return;
    }
    bool write = false;
    bool isAddr = (getStaus() == TWI_START
                || getStaus() == TWI_REP_START); // We just sent Start, so this must be slave address

    if( isAddr ) write = ((newTWDR & 1) == 0);   // Sending address for Read or Write?

    masterWrite( newTWDR, isAddr, write );       /// Write data or address to Slave
}

void AvrTwi::setTwiState( twiState_t state )     // Set new AVR Status value
{
    TwiModule::setTwiState( state );

    *m_statReg &= 0b00000111;      // Clear old state
    *m_statReg |= state;           // Write new state

    if( state == TWI_NO_STATE && m_i2cState == I2C_STOP ) // Stop Condition sent
    {
        clearRegBits( m_TWSTO );   // Clear TWSTO bit
    }
    else{
        m_interrupt->raise();

        if( m_mode == TWI_MASTER )
        {
            if( state == TWI_MRX_DATA_ACK || state == TWI_MRX_DATA_NACK ) // Data received
                *m_dataReg = m_rxReg; // Save data received into TWDR
        }
        else{ // Slave
            if( state == TWI_SRX_ADR_DATA_ACK || state == TWI_SRX_ADR_DATA_NACK
             || state == TWI_SRX_GEN_DATA_ACK || state == TWI_SRX_GEN_DATA_NACK )
                *m_dataReg = m_rxReg; // Save data received into TWDR
}   }   }

void AvrTwi::updateFreq()
{
    double freq = m_mcu->freq()/(16+2*m_bitRate*m_prescaler);
    setFreqKHz( freq/1e3 );
}


void AvrTwi::writeByte() // Read from Data Register
{
    if( m_mode == TWI_SLAVE ) m_txReg = *m_dataReg;
    TwiModule::writeByte();
}

void AvrTwi::I2Cstop()
{
    TwiModule::I2Cstop();
    if( m_mode != TWI_SLAVE ) return;
    if (m_addrMatch) setTwiState( TWI_SRX_STOP_RESTART );
}
