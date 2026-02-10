/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "twimodule.h"
#include "iopin.h"
#include "simulator.h"

TwiModule::TwiModule( QString name )
         : eClockedDevice( name )
{
    m_sda = nullptr;
    m_scl = nullptr;
    m_addrBits = 7;
    m_address = m_cCode = 0;
    m_enabled = true;

    setFreqKHz( 100 );
}
TwiModule::~TwiModule( ){}

void TwiModule::initialize()
{
    m_mode      = TWI_OFF;
    m_twiState  = TWI_NO_STATE;
    m_i2cState  = I2C_IDLE;
    m_lastState = I2C_IDLE;

    m_toggleScl  = false;
    m_genCall    = false;

    m_lastSDA = true; // SDA High = inactive
    m_masterACK = true;
}

void TwiModule::stamp()      // Called at Simulation Start
{ /* We are just avoiding eClockedDevice::stamp() call*/ }

void TwiModule::runEvent()
{
    if( m_mode != TWI_MASTER ) return;

    updateClock();
    bool clkLow = ((m_clkState == Clock_Low) || (m_clkState == Clock_Falling));

    if( m_toggleScl )
    {
        setSCL( clkLow );     // High if is LOW, LOW if is HIGH
        m_toggleScl = false;
        Simulator::self()->addEvent( m_clockPeriod/2, this );
        return;
    }
    getSdaState();               // Update state of SDA pin

    switch( m_i2cState )
    {
    case I2C_IDLE: break;

        case I2C_STOP:           // Send Stop Condition
        {
            if     (  m_sdaState &&  clkLow ) setSDA( false ); // Step 1: Lower SDA
            else if( !m_sdaState &&  clkLow ) setSCL( true );  // Step 2: Raise Clock
            else if( !m_sdaState && !clkLow ) setSDA( true );  // Step 3: Raise SDA
            else if(  m_sdaState && !clkLow )                  // Step 4: Operation Finished
            {
                setTwiState( TWI_NO_STATE ); // Set State first so old m_i2cState is still avilable
                m_i2cState = I2C_IDLE;
                return;                     // Stop clock
            }
        } break;

        case I2C_START :         // Send Start Condition
        {
            if     ( clkLow )     setSCL( true );  // Step 1: Clock Low, Raise Clock
            else if( m_sdaState ) setSDA( false ); // Step 2: SDA is High, Lower it
            else if( !clkLow )                     // Step 3: SDA Already Low, Lower Clock
            {
                setSCL( false ); //m_toggleScl = true;
                if( m_i2cState == I2C_WRITE ) setTwiState( TWI_REP_START );
                else                          setTwiState( TWI_START );
                m_i2cState = I2C_IDLE;
            }
        }break;

        case I2C_READ:            // We are Reading data
        {
            if( !clkLow )         // Read bit while clk is high
            {
                readBit();
                if( m_bitPtr == 8 ) readByte();
            }
            m_toggleScl = true;
        }break;

        case I2C_WRITE :          // We are Writting data
        {
            if( clkLow )  // Set SDA while clk is Low
            {
                writeBit();
                if( m_i2cState == I2C_READACK ) bufferEmpty(); // Data sent (before ACK)
            }
            m_toggleScl = true;
        }break;

        case I2C_ACK:             // Send ACK
        {
            if( m_masterACK ) // PICs send Master ACK as separate action
            {
                if( clkLow )
                {
                    if( m_sendACK ) setSDA( false);
                    m_i2cState = I2C_ENDACK;
                }
                m_toggleScl = true;
            }
        }break;

        case I2C_ENDACK:         // We sent ACK, release SDA
        {
            if( clkLow )
            {
                setSDA( true ); //if( m_lastState == I2C_READ )

                twiState_t twiState = m_sendACK ? TWI_MRX_DATA_ACK : TWI_MRX_DATA_NACK ;
                setTwiState( twiState );
                m_i2cState = I2C_IDLE;
            }
            else m_toggleScl = true;
        }break;

        case I2C_READACK:         // Read ACK
        {
            if( clkLow )
            {
                setTwiState( m_nextState );
                m_i2cState = I2C_IDLE;
            }
            else{
                if( m_isAddr ) // ACK after sendind Slave address
                {
                    if( m_write ) m_nextState = m_sdaState ? TWI_MTX_ADR_NACK : TWI_MTX_ADR_ACK; // Transmition started
                    else          m_nextState = m_sdaState ? TWI_MRX_ADR_NACK : TWI_MRX_ADR_ACK; // Reception started
                }
                else           // ACK after sendind data
                    m_nextState = m_sdaState ? TWI_MTX_DATA_NACK : TWI_MTX_DATA_ACK;
                m_toggleScl = true;
            }
        }break;
    }
    uint64_t time = m_toggleScl ? m_clockPeriod/2 : m_clockPeriod;
    Simulator::self()->addEvent( time, this );
}

void TwiModule::voltChanged() // Used by slave
{
    if( m_mode != TWI_SLAVE ) return;

    updateClock();
    getSdaState();                             // State of SDA pin

    if( m_clkState == Clock_High && m_i2cState != I2C_ACK )
    {
        if( m_lastSDA && !m_sdaState ) {       // We are in a Start Condition
            m_bitPtr = 0;
            m_rxReg = 0;
            m_i2cState = I2C_START;
        }
        else if( !m_lastSDA && m_sdaState ) {  // We are in a Stop Condition
           I2Cstop();
    }   }
    else if( m_clkState == Clock_Rising )      // We are in a SCL Rissing edge
    {
        if( m_i2cState == I2C_START )          // Get Transaction Info
        {
            readBit();
            if( m_bitPtr > m_addrBits )
            {
                bool rw = m_rxReg % 2;         //Last bit is R/W
                m_rxReg >>= 1;

                m_addrMatch = m_rxReg == m_address;
                bool genCall = m_genCall && (m_rxReg == 0);

                if( m_addrMatch || genCall )   // Address match or General Call
                {
                    if( m_enabled )
                    {
                        m_sendACK = true;
                        if( rw )                   // Master is Reading
                        {
                            m_nextState = TWI_STX_ADR_ACK;
                            m_i2cState = I2C_READ;
                            writeByte();
                        }
                        else{                      // Master is Writting
                            m_nextState = m_addrMatch ? TWI_SRX_ADR_ACK : TWI_SRX_GEN_ACK;
                            m_i2cState = I2C_WRITE;
                            m_bitPtr = 0;
                            startWrite();          // Notify posible child class
                        }
                        ACK();
                    }
                }
                else {
                    m_i2cState = I2C_STOP;
                    m_rxReg = 0;
            }   }
        }else if( m_i2cState == I2C_WRITE ){
            readBit();
            if( m_bitPtr == 8 )
            {
                if( m_addrMatch )
                     m_nextState = m_sendACK ? TWI_SRX_ADR_DATA_ACK : TWI_SRX_ADR_DATA_NACK;
                else m_nextState = m_sendACK ? TWI_SRX_GEN_DATA_ACK : TWI_SRX_GEN_DATA_NACK;
                readByte();
        }   }
        else if( m_i2cState == I2C_READACK )      // We wait for Master ACK
        {
            setTwiState( m_sdaState ? TWI_STX_DATA_NACK : TWI_STX_DATA_ACK );
            if( !m_sdaState ) {                // ACK: Continue Sending
                m_i2cState = m_lastState;
                writeByte();
            }
            else m_i2cState = I2C_IDLE;
        }
        else if( m_i2cState == I2C_ENDACK )      // We sent ACK, set State
        {
            setTwiState( m_nextState );
        }
    }
    else if( m_enabled && m_clkState == Clock_Falling )
    {
        if( m_i2cState == I2C_ACK ) {             // Send ACK
            scheduleSDA( !m_sendACK );
            m_i2cState = I2C_ENDACK;
        }
        else if( m_i2cState == I2C_ENDACK )      // We sent ACK, release SDA
        {
            m_i2cState = m_lastState;
            if( m_i2cState != I2C_READ ) scheduleSDA( true );
            m_rxReg = 0;
        }
        if( m_i2cState == I2C_READ ) writeBit();
    }
    m_lastSDA = m_sdaState;
}

void TwiModule::setMode( twiMode_t mode )
{
    m_scl->changeCallBack( this, mode == TWI_SLAVE );
    m_sda->changeCallBack( this, mode == TWI_SLAVE );

    if( mode > TWI_OFF )
    {
        m_scl->scheduleState( true, 10000 /*m_clockPeriod/4*/ ); // Avoid false stop condition
        setSDA( true );
    }
    m_mode = mode;
    m_i2cState = I2C_IDLE;
    m_toggleScl  = false;
}

void TwiModule::setSCL( bool st ) { m_scl->scheduleState( st, 0 ); }
void TwiModule::setSDA( bool st ) { m_sda->scheduleState( st, 0 ); }
void TwiModule::getSdaState() { m_sdaState = m_sda->getInpState(); }

void TwiModule::scheduleSDA( bool state )
{
    m_sda->scheduleState( state, 10000 );
}

void TwiModule::readBit()
{
    if( m_bitPtr > 0 ) m_rxReg <<= 1;
    m_rxReg += m_sdaState;            //Read one bit from sda
    m_bitPtr++;
}

void TwiModule::writeBit()
{
    if( m_bitPtr < 0 ) { waitACK(); return; }

    bool bit = m_txReg>>m_bitPtr & 1;
    m_bitPtr--;

    if( m_mode == TWI_MASTER ) setSDA( bit );
    else                       scheduleSDA( bit );
}

void TwiModule::readByte()
{
    m_bitPtr = 0;
    ACK();
}

void TwiModule::waitACK()
{
    setSDA( true );
    m_lastState = m_i2cState;
    m_i2cState = I2C_READACK;
}

void TwiModule::ACK()
{
    m_lastState = m_i2cState;
    m_i2cState = I2C_ACK;
}

void TwiModule::masterStart()
{
    m_i2cState = I2C_START;
    Simulator::self()->cancelEvents( this );
    runEvent();               // Start Clock
}

void TwiModule::masterWrite( uint8_t data , bool isAddr, bool write )
{
    m_isAddr = isAddr;
    m_write  = write;

    m_i2cState = I2C_WRITE;
    m_txReg = data;
    writeByte();
}

void TwiModule::masterRead( bool ack )
{
    m_sendACK = ack;

    setSDA( true );
    m_bitPtr = 0;
    m_rxReg = 0;
    m_i2cState = I2C_READ;

    Simulator::self()->cancelEvents( this );
    Simulator::self()->addEvent( m_clockPeriod, this );
}

void TwiModule::masterStop()
{
    m_i2cState = I2C_STOP;
    setSDA( false );
}

void TwiModule::slaveWrite()
{
    m_bitPtr = 7;// Start Slave transmission
    writeBit();
}

void TwiModule::setFreqKHz( double f )
{
    m_freq = f*1e3;
    double stepsPerS = 1e12;
    m_clockPeriod = stepsPerS/m_freq/2;
}
void TwiModule::setSdaPin( IoPin* pin ) { m_sda = pin; }
void TwiModule::setSclPin( IoPin* pin )
{
    m_scl = pin;
    m_clkPin = pin;
}

void TwiModule::setPins( IoPin* scl, IoPin* sda )
{
    setSclPin( scl );
    setSdaPin( sda );
}

uint64_t TwiModule::getTrasferTime()
{
    return 9*m_clockPeriod;
}
