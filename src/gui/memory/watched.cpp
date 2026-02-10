/***************************************************************************
 *   Copyright (C) 2023 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "watched.h"
#include "watcher.h"

Watched::Watched()
{
    m_display = nullptr;
    m_watcher = nullptr;
}
Watched::~Watched()
{
    //if( m_watcher ) m_watcher->deleteLater();
}

void Watched::createWatcher( bool proxy )
{
    if( !m_watcher ) m_watcher = new Watcher( nullptr, this, proxy );
}
