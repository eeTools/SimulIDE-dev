/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "mcu8bits.h"

enum {
    C=0,DC,Z,PD,TO,RP0,RP1,IRP
};

class PicMrCore : public Mcu8bits
{
    public:
        PicMrCore( eMcu* mcu );
        ~PicMrCore();

        virtual void reset();
        virtual void runStep() override;

        virtual uint RET_ADDR() override { return m_stack[m_sp]; }

    protected:
        virtual void decode( uint16_t instr );
        uint8_t* m_Wreg;
        uint8_t* m_OPTION;

        uint8_t* m_FSR;
        uint8_t m_WregHidden;

        regBits_t m_bankBits;
        uint16_t  m_bank;

        uint16_t m_PCLaddr;
        uint16_t m_PCHaddr;

        uint32_t m_stack[8];
        uint8_t  m_sp;
        uint8_t  m_stackSize;

        virtual void setBank( uint8_t bank );

        void incDefault()
        {
            setPC( m_PC+1 );
            m_mcu->cyclesDone += 1;
        }

        virtual void PUSH_STACK( uint32_t addr ) override // Harware circular Stack
        {
            m_stack[m_sp] = addr;
            m_sp++;
            if( m_sp == m_stackSize ) m_sp = 0;
        }
        virtual uint32_t POP_STACK() override // Hardware circular Stack
        {
            if( m_sp == 0 ) m_sp = m_stackSize-1;
            else            m_sp--;
            return m_stack[m_sp];
        }

        virtual void setPC( uint32_t pc ) override
        {
            m_PC = pc & 0x00001FFF;
            m_dataMem[ m_PCLaddr] = m_PC & 0xFF;
        }

        void setValue( uint8_t newV, uint8_t f, uint8_t d )
        {
            if( d ) SET_RAM( f, newV );
            else    *m_Wreg = newV;
        }
        void setValueZ( uint8_t newV, uint8_t f, uint8_t d )
        {
            setValue( newV, f, d );
            write_S_Bit( Z, newV==0 );
        }

        uint8_t add( uint8_t val1, uint8_t val2 );
        uint8_t sub( uint8_t val1, uint8_t val2 );

        virtual void exitSleep() override;

        // Miscellaneous instructions
        void RETURN();
        void RETFIE();
        void OPTION();
        void SLEEP();
        void CLRWDT();
        //void TRIS( uint8_t f );

        // ALU operations: dest ← OP(f,W)
        void MOVWF( uint8_t f );
        void CLR( uint8_t f , uint8_t d );
        void SUBWF( uint8_t f, uint8_t d );
        void DECF( uint8_t f, uint8_t d );
        void IORWF( uint8_t f, uint8_t d );
        void ANDWF( uint8_t f, uint8_t d );
        void XORWF( uint8_t f, uint8_t d );
        void ADDWF( uint8_t f, uint8_t d );
        void MOVF( uint8_t f, uint8_t d );
        void COMF( uint8_t f, uint8_t d );
        void INCF( uint8_t f, uint8_t d );
        void DECFSZ( uint8_t f, uint8_t d );
        void RRF( uint8_t f, uint8_t d );
        void RLF( uint8_t f, uint8_t d );
        void SWAPF( uint8_t f, uint8_t d );
        void INCFSZ( uint8_t f, uint8_t d );

        // Bit operations
        void BCF( uint8_t f, uint8_t b );
        void BSF( uint8_t f, uint8_t b );
        void BTFSC( uint8_t f, uint8_t b );
        void BTFSS( uint8_t f, uint8_t b );

        // Control transfers
        void CALL( uint16_t k );
        void GOTO( uint16_t k );

        // Operations with W and 8-bit literal: W ← OP(k,W)
        void MOVLW( uint8_t k );
        void RETLW( uint8_t k );
        void IORLW( uint8_t k );
        void ANDLW( uint8_t k );
        void XORLW( uint8_t k );
        void SUBLW( uint8_t k );
        void ADDLW( uint8_t k );
};
