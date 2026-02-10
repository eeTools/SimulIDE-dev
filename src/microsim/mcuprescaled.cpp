/***************************************************************************
 *   Copyright (C) 2022 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "mcuprescaled.h"

McuPrescaled::McuPrescaled( eMcu* mcu, QString name )
            : McuModule( mcu, name )
{

}
McuPrescaled::~McuPrescaled( ){}

void McuPrescaled::setPrescIndex( uint16_t p )
{
    if( p >= m_prescList.size() ) p = 0;
    m_prIndex = p;
    m_prescaler = m_prescList.at( m_prIndex );
}
