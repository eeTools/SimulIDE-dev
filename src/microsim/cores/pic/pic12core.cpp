/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "pic12core.h"
#include "datautils.h"
#include "regwatcher.h"

Pic12Core::Pic12Core( eMcu* mcu )
         : PicMrCore( mcu )
{
    m_stackSize = 8;

    m_Wreg = &m_WregHidden;

    m_FSR    = m_mcu->getReg( "FSR" );
    m_OPTION = m_mcu->getReg( "OPTION" );
    m_TRISaddr = m_mcu->getRegAddress( "TRISGPIO" );
}
Pic12Core::~Pic12Core() {}

void Pic12Core::TRIS()
{
    m_mcu->writeReg( m_TRISaddr, *m_Wreg );
}

void Pic12Core::runStep()
{
    uint16_t instr = m_progMem[m_PC] & 0x0FFF;

    m_mcu->cyclesDone = 0;
    incDefault();

    if( instr != 0 ) decode( instr );
}

void Pic12Core::decode( uint16_t instr )
{
    if( (instr & 0xFF8) == 0 )  // Miscellaneous instrs
    {
        if     ( instr == 0x02 ) OPTION(); // OPTION 0000 0000 0010 Load W to OPTION register
        else if( instr == 0x03 ) SLEEP();  // SLEEP  0000 0000 0011
        else if( instr == 0x04 ) CLRWDT(); // CLRWDT 0000 0000 0100
        else if( instr == 0x06 ) TRIS();   // TRIS   0000 0000 0110 Load W to TRIS register
    }
    else if( (instr & 0xC00) == 0 ) // ALU operations: dest ← OP(f,W)
    {
        uint8_t f = instr & 0x1F;
        uint8_t d = instr>>5 & 1;

        if( (instr & 0xE00) == 0 ) {
            switch( instr & 0x1C0) {
            case 0x000: MOVWF( f );    return; // MOVWF f   0000 001f ffff
            case 0x040: CLR( f, 1 );   return; // CLR   f   0000 011f ffff
            case 0x080: SUBWF( f, d ); return; // SUBWF f,d 0000 10df ffff
            case 0x0C0: DECF( f, d );  return; // DECF  f,d 0000 11df ffff
            case 0x100: IORWF( f, d ); return; // IORWF f,d 0001 00df ffff
            case 0x140: ANDWF( f, d ); return; // ANDWF f,d 0001 01df ffff
            case 0x180: XORWF( f, d ); return; // XORWF f,d 0001 10df ffff
            case 0x1C0: ADDWF( f, d ); return; // ADDWF f,d 0001 11df ffff
            }
        } else {
            switch( instr & 0x1C0) {
            case 0x000: MOVF( f, d );   return; // MOVF   f,d 0010 00df ffff
            case 0x040: COMF( f, d );   return; // COMF   f,d 0010 01df ffff
            case 0x080: INCF( f, d );   return; // INCF   f,d 0010 10df ffff
            case 0x0C0: DECFSZ( f, d ); return; // DECFSZ f,d 0010 11df ffff
            case 0x100: RRF( f, d );    return; // RRF    f,d 0011 00df ffff
            case 0x140: RLF( f, d );    return; // RLF    f,d 0011 01df ffff
            case 0x180: SWAPF( f, d );  return; // SWAPF  f,d 0011 10df ffff
            case 0x1C0: INCFSZ( f, d ); return; // INCFSZ f,d 0011 11df ffff
            }
        }
    } else {
        if( (instr & 0xC00) == 0x400 ) // Bit operations
        {
            uint8_t f = instr & 0x1F;
            uint8_t b = instr>>5 & 7;

            switch( instr & 0xF00){
            case 0x400: BCF( f, b );   return; // BCF   f,b 0100 bbbf ffff
            case 0x500: BSF( f, b );   return; // BSF   f,b 0101 bbbf ffff
            case 0x600: BTFSC( f, b ); return; // BTFSC f,b 0110 bbbf ffff
            case 0x700: BTFSS( f, b ); return; // BTFSS f,b 0111 bbbf ffff
            }
        }
        else if( (instr & 0xE00) == 0x800 ) // Control transfers
        {
            uint16_t k = instr & 0x0FF;
            if( (instr & 0x100) == 0x100 ) CALL( k );  // CALL  k 1001 kkkk kkkk
            else                           RETLW( k ); // RETLW k 1000 kkkk kkkk
        }
        else if( (instr & 0xE00) == 0xA00 )
        {
            uint16_t k = instr & 0x1FF;
            GOTO( k ); // GOTO  k 101k kkkk kkkk
        }
        else if( (instr & 0xC00) == 0xC00 ) // Operations with W and 8-bit literal: W ← OP(k,W)
        {
            uint8_t k = instr & 0x0FF;

            switch( instr & 0xF00){
                case 0xC00: MOVLW( k ); return; // MOVLW k 1100 kkkk kkkk
                case 0xD00: IORLW( k ); return; // IORLW k 1101 kkkk kkkk
                case 0xE00: ANDLW( k ); return; // ANDLW k 1110 kkkk kkkk
                case 0xF00: XORLW( k ); return; // XORLW k 1111 kkkk kkkk
            }
        }
    }
    //while( m_PC >= m_progSize ) m_PC -= m_progSize;
}
