/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "pic14ecore.h"
#include "datautils.h"
#include "regwatcher.h"

Pic14eCore::Pic14eCore( eMcu* mcu )
          : PicMrCore( mcu )
{
    m_stackSize = 16;

    m_OPTION = nullptr;

    m_Wreg  = mcu->getReg( "WREG" );
    m_FSR0L = mcu->getReg( "FSR0L" );
    m_FSR0H = mcu->getReg( "FSR0H" );
    m_FSR1L = mcu->getReg( "FSR1L" );
    m_FSR1H = mcu->getReg( "FSR1H" );

    m_BSR = mcu->getReg( "BSR" );
    m_bankBits = getRegBits( "BSR0,BSR1,BSR2,BSR3,BSR4", mcu );
    watchBitNames( "BSR0,BSR1,BSR2,BSR3,BSR4", R_WRITE, this, &Pic14eCore::setBank, mcu );
}
Pic14eCore::~Pic14eCore() {}

// Miscellaneous instructions

/*inline void Pic14eCore::RESET()
{

}*/

inline void Pic14eCore::CALLW()
{
    CALL_ADDR( *m_Wreg | ((uint16_t)(m_dataMem[m_PCHaddr] & 0b00011000)<<8) );
}

inline void Pic14eCore::BRW()
{
    setPC( *m_Wreg | ((uint16_t)(m_dataMem[m_PCHaddr] & 0b00011000)<<8) );
    m_mcu->cyclesDone = 2;
}

inline void Pic14eCore::MOVIW_iF( uint8_t n )
{
    if( n == 0 ){
        setFSR0( getFSR0()+1 );
        *m_Wreg = GET_RAM( 0 );
    }else{
        setFSR1( getFSR1()+1 );
        *m_Wreg = GET_RAM( 1 );
    }
    write_S_Bit( Z, *m_Wreg==0 );
}

inline void Pic14eCore::MOVIW_dF( uint8_t n )
{
    if( n == 0 ){
        setFSR0( getFSR0()-1 );
        *m_Wreg = GET_RAM( 0 );
    }else{
        setFSR1( getFSR1()-1 );
        *m_Wreg = GET_RAM( 1 );
    }
    write_S_Bit( Z, *m_Wreg==0 );
}

inline void Pic14eCore::MOVIW_Fi( uint8_t n )
{
    if( n == 0 ){
        *m_Wreg = GET_RAM( 0 );
        setFSR0( getFSR0()+1 );
    }else{
        *m_Wreg = GET_RAM( 1 );
        setFSR1( getFSR1()+1 );
    }
    write_S_Bit( Z, *m_Wreg==0 );
}

inline void Pic14eCore::MOVIW_Fd( uint8_t n )
{
    if( n == 0 ){
        *m_Wreg = GET_RAM( 0 );
        setFSR0( getFSR0()-1 );
    }else{
        *m_Wreg = GET_RAM(1);
        setFSR1( getFSR1()-1 );
    }
    write_S_Bit( Z, *m_Wreg==0 );
}

inline void Pic14eCore::MOVWI_iF( uint8_t n )
{
    if( n == 0 ){
        setFSR0( getFSR0()+1 );
        SET_RAM( 0, *m_Wreg );
    }else{
        setFSR1( getFSR1()+1 );
        SET_RAM( 1, *m_Wreg );
    }
}

inline void Pic14eCore::MOVWI_dF( uint8_t n )
{
    if( n == 0 ){
        setFSR0( getFSR0()-1 );
        SET_RAM( 0, *m_Wreg );
    }else{
        setFSR1( getFSR1()-1 );
        SET_RAM( 1, *m_Wreg );
    }
}

inline void Pic14eCore::MOVWI_Fi( uint8_t n )
{
    if( n == 0 ){
        SET_RAM( 0, *m_Wreg );
        setFSR0( getFSR0()+1 );
    }else{
        SET_RAM( 1, *m_Wreg );
        setFSR1( getFSR1()+1 );
    }
}

inline void Pic14eCore::MOVWI_Fd( uint8_t n )
{
    if( n == 0 ){
        SET_RAM( 0, *m_Wreg );
        setFSR0( getFSR0()-1 );
    }else{
        SET_RAM( 1, *m_Wreg );
        setFSR1( getFSR1()-1 );
    }
}

inline void Pic14eCore::MOVLB( uint8_t k )
{
    *m_BSR = k & 0x0F;
    setBank( *m_BSR );
}

// ALU operations: dest ← OP(f,W)

inline void Pic14eCore::LSLF( uint8_t f, uint8_t d )
{
    uint8_t newV = GET_RAM( f ) << 1;
    setValue( newV, f, d );
}

inline void Pic14eCore::LSRF( uint8_t f, uint8_t d )
{
    uint8_t newV = GET_RAM( f ) >> 1;
    setValue( newV, f, d );
}

inline void Pic14eCore::ASRF( uint8_t f, uint8_t d )
{
    int8_t oldV = GET_RAM( f ) ;
    int8_t newV = oldV >> 1;
    setValue( newV, f, d );
}

inline void Pic14eCore::SUBWFB( uint8_t f, uint8_t d )
{
    uint8_t carry = ( *m_STATUS & 1<<C ) ? 1 : 0;
    uint8_t newV = add( GET_RAM( f )+carry, *m_Wreg );
    setValue( newV, f, d );
}

inline void Pic14eCore::ADDWFC( uint8_t f, uint8_t d )
{
    uint8_t carry = ( *m_STATUS & 1<<C ) ? 1 : 0;
    uint8_t newV = sub( GET_RAM( f )+carry, *m_Wreg );
    setValue( newV, f, d );
}

// Operations with literal k

inline void Pic14eCore::ADDFSR( uint8_t n, uint8_t k )
{
    if( n == 0 ) setFSR0( getFSR0()+k );
    else         setFSR1( getFSR1()+k );
}

inline void Pic14eCore::MOVLP( uint8_t k )
{
    m_dataMem[m_PCHaddr] = k;
}

inline void Pic14eCore::BRA( uint8_t k )
{
    setPC( k | ((uint16_t)(m_dataMem[m_PCHaddr] & 0b00011000)<<8) );
    m_mcu->cyclesDone = 2;
}

inline void Pic14eCore::MOVIW( uint8_t n, uint8_t k )
{
    if( n == 0 ){
        setFSR0( getFSR0()+k );
        *m_Wreg = GET_RAM( 0 );
    }else{
        setFSR1( getFSR1()+k );
        *m_Wreg = GET_RAM( 1 );
    }
    write_S_Bit( Z, *m_Wreg==0 );
}

inline void Pic14eCore::MOVWI( uint8_t n, uint8_t k )
{
    if( n == 0 ){
        *m_Wreg = GET_RAM( 0 );
        setFSR0( getFSR0()+k );
    }else{
        *m_Wreg = GET_RAM( 1 );
        setFSR1( getFSR1()+k );
    }
}

void Pic14eCore::decode( uint16_t instr )
{
    if( (instr & 0x3FC0) == 0 )  // Miscellaneous instrs
    {
        if( (instr & 0x0030) == 0 ){
            if     ( instr == 0x0001 ) reset(); // RESET 00 0000 0000 0001
            else if( instr == 0x000A ) CALLW(); // CALLW 00 0000 0000 1010
            else if( instr == 0x000B ) BRW();   // BRW   00 0000 0000 1011
        }
        else if( (instr & 0x0030) == 0x10 )
        {
            uint8_t n = instr & 1<<2;
            if( (instr & 0x0008) == 0 ){
                switch( instr & 0x0003) {
                    case 0: MOVIW_iF( n ); return; // MOVIW ++FSRn 00 0000 0001 0n00
                    case 1: MOVIW_dF( n ); return; // MOVIW −−FSRn 00 0000 0001 0n01
                    case 2: MOVIW_Fi( n ); return; // MOVIW FSRn++ 00 0000 0001 0n10
                    case 3: MOVIW_Fd( n ); return; // MOVIW FSRn−− 00 0000 0001 0n11
                }
            }else{
                switch( instr & 0x0003) {
                    case 0: MOVWI_iF( n ); return; // MOVWI ++FSRn 00 0000 0001 1n00
                    case 1: MOVWI_dF( n ); return; // MOVWI −−FSRn 00 0000 0001 1n01
                    case 2: MOVWI_Fi( n ); return; // MOVWI FSRn++ 00 0000 0001 1n10
                    case 3: MOVWI_Fd( n ); return; // MOVWI FSRn−− 00 0000 0001 1n11
                }
            }
        }
        else if( (instr & 0x0020) > 0 ) MOVLB( instr & 0x1F ); // MOVLB k 00 0000 001k kkkk
    }
    else if( (instr & 0x3000) == 0x3000 ){
        uint8_t d = instr & 1<<7;
        uint8_t f = instr & 0x007F;
        // ALU operations: dest ← OP(f,W)
        switch( instr & 0x3F00 ) {
            case 0x3500: LSLF( f, d );   return; // LSLF   f,d 11 0101 dfff ffff
            case 0x3600: LSRF( f, d );   return; // LSRF   f,d 11 0110 dfff ffff
            case 0x3700: ASRF( f, d );   return; // ASRF   f,d 11 0111 dfff ffff
            case 0x3B00: SUBWFB( f, d ); return; // SUBWFB f,d 11 1011 dfff ffff
            case 0x3D00: ADDWFC( f, d ); return; // ADDWFC f,d 11 1101 dfff ffff
        }
        uint8_t n = instr & 1<<6;
        // Operations with literal k
        switch( instr & 0x3F80 ) {
            case 0x3100: ADDFSR( n, instr & 0x7F ); return; // ADDFSR FSRn,k 11 0001 0nkk kkkk
            case 0x3180: MOVLP( instr & 0x1F );     return; // MOVLP       k 11 0001 1kkk kkkk
            case 0x3F00: MOVIW( n, instr & 0x7F );  return; // MOVIW k[FSRn] 11 1111 0nkk kkkk
            case 0x3F80: MOVWI( n, instr & 0x7F );  return; // MOVWI k[FSRn] 11 1111 1nkk kkkk
        }
        if( (instr & 0x3C00) == 0x3200 ){ BRA( instr & 0x1FF); return; }// BRA k 11 001k kkkk kkkk
    }
    PicMrCore::decode( instr );
}

