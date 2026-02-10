/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "picmrcore.h"
#include "datautils.h"
#include "regwatcher.h"

PicMrCore::PicMrCore( eMcu* mcu )
         : Mcu8bits( mcu )
{
    m_sp = 0;
    m_bank = 0;

    m_PCLaddr = mcu->getRegAddress("PCL");
    m_PCHaddr = mcu->getRegAddress("PCLATH");
}
PicMrCore::~PicMrCore() {}

void PicMrCore::reset()
{
    Cpu8bits::reset();

    *m_Wreg = 0;
}

void PicMrCore::setBank( uint8_t bank )
{
    m_bank = getRegBitsVal( bank, m_bankBits );
    m_bank <<= 7;
}

uint8_t PicMrCore::add( uint8_t val1, uint8_t val2 )
{
    uint16_t newV = val1 + val2;
    write_S_Bit( Z, (newV & 0xFF)==0 );
    write_S_Bit( C, newV & 0x100 );
    write_S_Bit( DC, ((val1&0xF)+(val2 & 0xF)) > 0xF );
    return newV;
}

uint8_t PicMrCore::sub( uint8_t val1, uint8_t val2 )
{
    int16_t newV = val1 - val2;
    write_S_Bit( Z, (newV & 0xFF)==0 );
    write_S_Bit( C, newV >= 0 );
    write_S_Bit( DC, ((int16_t)((val1&0xF)-(val2&0xF)) >= 0) ); // Carry and Half carry set when doing 0-0
    return newV;
}

// Miscellaneous instructions

void PicMrCore::RETURN() { RET(); }

void PicMrCore::RETFIE() { RETI(); }

void PicMrCore::OPTION() { *m_OPTION = *m_Wreg; }

void PicMrCore::SLEEP()
{
    write_S_Bit( PD, false );
    write_S_Bit( TO, true );
    m_mcu->sleep( true );
}

void PicMrCore::exitSleep() { write_S_Bit( TO, false ); }

void PicMrCore::CLRWDT()
{
    write_S_Bit( PD, true );
    write_S_Bit( TO, true );
    m_mcu->wdr();
}

// ALU operations: dest ← OP(f,W)

void PicMrCore::MOVWF( uint8_t f )
{
    SET_RAM( f, *m_Wreg);
}

void PicMrCore::CLR( uint8_t f, uint8_t d )
{
    setValue( 0, f, d );
    write_S_Bit( Z, true );
}

void PicMrCore::SUBWF( uint8_t f, uint8_t d )
{
    uint8_t newV = sub( GET_RAM( f ), *m_Wreg );
    setValue( newV, f, d );
}

void PicMrCore::DECF( uint8_t f, uint8_t d )
{
    uint8_t newV = GET_RAM( f );
    setValueZ( --newV, f, d );
}

void PicMrCore::IORWF( uint8_t f, uint8_t d )
{
    uint8_t oldV = GET_RAM( f ) ;
    uint8_t newV = oldV | *m_Wreg;
    setValueZ( newV, f, d );
}

void PicMrCore::ANDWF( uint8_t f, uint8_t d )
{
    uint8_t oldV = GET_RAM( f ) ;
    uint8_t newV = oldV & *m_Wreg;
    setValueZ( newV, f, d );
}

void PicMrCore::XORWF( uint8_t f, uint8_t d )
{
    uint8_t oldV = GET_RAM( f ) ;
    uint8_t newV = oldV ^ *m_Wreg;
    setValueZ( newV, f, d );
}

void PicMrCore::ADDWF( uint8_t f, uint8_t d )
{
    uint8_t newV = add( GET_RAM( f ), *m_Wreg );
    setValue( newV, f, d );
}

void PicMrCore::MOVF( uint8_t f, uint8_t d )
{
    uint8_t newV = GET_RAM( f );
    setValueZ( newV, f, d );
}

void PicMrCore::COMF( uint8_t f, uint8_t d )
{
    uint8_t newV = GET_RAM( f ) ^ 0xFF;
    setValueZ( newV, f, d );
}

void PicMrCore::INCF( uint8_t f, uint8_t d )
{
    uint8_t newV = GET_RAM( f );
    setValueZ( ++newV, f, d );
}

void PicMrCore::DECFSZ( uint8_t f, uint8_t d )
{
    uint8_t newV = GET_RAM( f ) - 1;
    setValue( newV, f, d );
    if( newV == 0 ) incDefault();
}

void PicMrCore::RRF( uint8_t f, uint8_t d )
{
    uint8_t oldV = GET_RAM( f ) ;
    uint8_t newV = oldV >> 1;
    if( *m_STATUS & 1<<C ) newV |= 1<<7; // Carry In
    write_S_Bit( C, oldV & 1 );          // Carry Out
    setValue( newV, f, d );
}

void PicMrCore::RLF( uint8_t f, uint8_t d )
{
    uint8_t oldV = GET_RAM( f ) ;
    uint8_t newV = oldV << 1;
    if( *m_STATUS & 1<<C ) newV |= 1; // Carry In
    write_S_Bit( C, oldV & 1<<7 );    // Carry Out
    setValue( newV, f, d );
}

void PicMrCore::SWAPF( uint8_t f, uint8_t d )
{
    uint8_t oldV = GET_RAM( f );
    uint8_t newV = ((oldV >> 4) & 0x0F) | ((oldV << 4) & 0xF0);
    setValue( newV, f, d );
}

void PicMrCore::INCFSZ( uint8_t f, uint8_t d )
{
    uint8_t newV = GET_RAM( f ) + 1;
    setValue( newV, f, d );
    if( newV == 0 ) incDefault();
}

// Bit operations

void PicMrCore::BCF( uint8_t f, uint8_t b )
{
    uint8_t newV = GET_RAM( f );
    newV &= ~(1<<b);
    SET_RAM( f, newV );
}

void PicMrCore::BSF( uint8_t f, uint8_t b )
{
    uint8_t newV = GET_RAM( f );
    newV |= 1<<b;
    SET_RAM( f, newV );
}

void PicMrCore::BTFSC( uint8_t f, uint8_t b )
{
    uint8_t oldV = GET_RAM( f );
    uint8_t bitMask = 1<<b;
    if( (oldV & bitMask) == 0 ) incDefault();
}

void PicMrCore::BTFSS( uint8_t f, uint8_t b )
{
    uint8_t oldV = GET_RAM( f );
    if( oldV & 1<<b  ) incDefault();
}

// Control transfers

void PicMrCore::CALL( uint16_t k )
{
    CALL_ADDR( k | ((uint16_t)(m_dataMem[m_PCHaddr] & 0b00011000)<<8) );
}

void PicMrCore::GOTO( uint16_t k )
{
    setPC( k | ((uint16_t)(m_dataMem[m_PCHaddr] & 0b00011000)<<8) );
    m_mcu->cyclesDone = 2;
}

// Operations with W and 8-bit literal: W ← OP(k,W)

void PicMrCore::MOVLW( uint8_t k )
{
    *m_Wreg = k;
}

void PicMrCore::RETLW( uint8_t k )
{
    *m_Wreg = k;
    RETURN();
}

void PicMrCore::IORLW( uint8_t k )
{
    *m_Wreg |= k;
    write_S_Bit( Z, *m_Wreg==0 );
}

void PicMrCore::ANDLW( uint8_t k )
{
    *m_Wreg &= k;
    write_S_Bit( Z, *m_Wreg==0 );
}

void PicMrCore::XORLW( uint8_t k )
{
    *m_Wreg ^= k;
    write_S_Bit( Z, *m_Wreg==0 );
}

void PicMrCore::SUBLW( uint8_t k ) //// C,DC,Z
{
    *m_Wreg = sub( k, *m_Wreg );
}

void PicMrCore::ADDLW( uint8_t k ) //// C,DC,Z
{
    *m_Wreg = add( k, *m_Wreg );
}

void PicMrCore::runStep()
{
    uint16_t instr = m_progMem[m_PC] & 0x3FFF;

    m_mcu->cyclesDone = 0;
    incDefault();
    m_RET_ADDR = m_PC;

    decode( instr );
}

void PicMrCore::decode( uint16_t instr )
{
    if( (instr & 0x3F80) == 0 )  // Miscellaneous instrs
    {
        switch( instr & 0x000C)
        {
            case 0x0008: {
                if     ( instr == 0x0008 ) RETURN(); // RETURN 00 0000 0000 1000
                else if( instr == 0x0009 ) RETFIE(); // RETFIE 00 0000 0000 1001
            } return;
            case 0x0000: {
                if     ( instr == 0x0062 ) OPTION(); // OPTION 00 0000 0110 0010
                else if( instr == 0x0063 ) SLEEP();  // SLEEP  00 0000 0110 0011
            } return;
            case 0x0004: {
                if     ( instr == 0x0064 ) CLRWDT(); // CLRWDT 00 0000 0110 0100
            } return;
        }
    }
    else if( (instr & 0x3000) == 0 ) // ALU operations: dest ← OP(f,W)
    {
        uint8_t f = instr & 0x7F;
        uint8_t d = instr>>7 & 1;

        if( (instr & 0x3800) == 0 ) {
            switch( instr & 0x0700) {
                case 0x0000: MOVWF( f );    return; // MOVWF f   00 0000 1fff ffff
                case 0x0100: CLR( f, d );   return; // CLR   f   00 0001 dfff ffff
                case 0x0200: SUBWF( f, d ); return; // SUBWF f,d 00 0010 dfff ffff
                case 0x0300: DECF( f, d );  return; // DECF  f,d 00 0011 dfff ffff
                case 0x0400: IORWF( f, d ); return; // IORWF f,d 00 0100 dfff ffff
                case 0x0500: ANDWF( f, d ); return; // ANDWF f,d 00 0101 dfff ffff
                case 0x0600: XORWF( f, d ); return; // XORWF f,d 00 0110 dfff ffff
                case 0x0700: ADDWF( f, d ); return; // ADDWF f,d 00 0111 dfff ffff
           }
        } else {
            switch( instr & 0x0700) {
                case 0x0000: MOVF( f, d );   return; // MOVF   f,d 00 1000 dfff ffff
                case 0x0100: COMF( f, d );   return; // COMF   f,d 00 0001 dfff ffff
                case 0x0200: INCF( f, d );   return; // INCF   f,d 00 0010 dfff ffff
                case 0x0300: DECFSZ( f, d ); return; // DECFSZ f,d 00 0011 dfff ffff
                case 0x0400: RRF( f, d );    return; // RRF    f,d 00 0100 dfff ffff
                case 0x0500: RLF( f, d );    return; // RLF    f,d 00 0101 dfff ffff
                case 0x0600: SWAPF( f, d );  return; // SWAPF  f,d 00 0110 dfff ffff
                case 0x0700: INCFSZ( f, d ); return; // INCFSZ f,d 00 0111 dfff ffff
            }
        }
    } else {
        if( (instr & 0x3000) == 0x1000 ) // Bit operations
        {
            uint8_t f = instr & 0x7F;
            uint8_t b = instr>>7 & 7;

            switch( instr & 0x3C00){
                case 0x1000: BCF( f, b );   return; // BCF   f,b 01 00bb bkkk kkkk
                case 0x1400: BSF( f, b );   return; // BSF   f,b 01 01bb bkkk kkkk
                case 0x1800: BTFSC( f, b ); return; // BTFSC f,b 01 10bb bkkk kkkk
                case 0x1C00: BTFSS( f, b ); return; // BTFSS f,b 01 11bb bkkk kkkk
            }
        }
        else if( (instr & 0x3000) == 0x2000 ) // Control transfers
        {
            uint16_t k = instr & 0x07FF;

            if( (instr & 0x0800) == 0 ) CALL( k ); // CALL k 10 0kkk kkkk kkkk
            else                        GOTO( k ); // GOTO k 10 1kkk kkkk kkkk
        }
        else if( (instr & 0x3000) == 0x3000 ) // Operations with W and 8-bit literal: W ← OP(k,W)
        {
            uint8_t k = instr & 0xFF;

            switch( instr & 0x3C00){
                case 0x3000: MOVLW( k ); return; // MOVLW k 11 00xx kkkk kkkk
                case 0x3400: RETLW( k ); return; // RETLW k 11 01xx kkkk kkkk
                case 0x3800: {
                    switch( instr & 0x3F00) {
                        case 0x3800: IORLW( k ); return; // IORLW k 11 1000 kkkk kkkk
                        case 0x3900: ANDLW( k ); return; // ANDLW k 11 1001 kkkk kkkk
                        case 0x3A00: XORLW( k ); return; // XORLW k 11 1010 kkkk kkkk
                    }
                } return;
                case 0x3C00: {
                    if((instr & 0x0200)==0 ) SUBLW( k ); // SUBLW k 11 110x kkkk kkkk
                    else                     ADDLW( k ); // ADDLW k 11 111x kkkk kkkk
                }
            }
        }
    }
}
