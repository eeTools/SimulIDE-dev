/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QMenu>

#include "i2cram.h"
#include "memtable.h"
#include "simulator.h"
#include "itemlibrary.h"
#include "iopin.h"
#include "label.h"

#include "stringprop.h"
#include "doubleprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("I2CRam",str)

Component* I2CRam::construct( QString type, QString id )
{ return new I2CRam( type, id ); }

LibraryItem* I2CRam::libraryItem()
{
    return new LibraryItem(
        tr("I2C Ram"),
        "Memory",
        "2to3.png",
        "I2CRam",
        I2CRam::construct );
}

I2CRam::I2CRam( QString type, QString id )
      : IoComponent( type, id )
      , TwiModule( id )
      , MemData()
{
    m_width  = 4;
    m_height = 4;
    
    init({         // Inputs:
            "IL01SDA", //type: Input, side: Left, pos: 01, label: "SDA"
            "IL03SCL",
            "IR01A0",
            "IR02A1",
            "IR03A2"
        });

    m_inpPin[0]->setPinMode( openCo );
    TwiModule::setSdaPin( m_inpPin[0] );

    m_inpPin[1]->setPinMode( openCo );
    TwiModule::setSclPin( m_inpPin[1] );

    m_persistent = false;
    m_cCode = 0b01010000; // 0x50, 80
    m_size  = 65536;
    m_ram.resize( m_size );

    IoComponent::initState();

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"), {
        new IntProp <I2CRam>("Size_bytes", tr("Size"),"_bytes"
                            , this, &I2CRam::rSize, &I2CRam::setRSize, propNoCopy,"uint" ),

        new IntProp <I2CRam>("Control_Code", tr("I2C Address"),""
                            , this, &I2CRam::cCode, &I2CRam::setCcode,0,"uint" ),

        new DoubProp<I2CRam>("Frequency", tr("I2C Frequency"),"_kHz"
                            , this, &I2CRam::freqKHz, &I2CRam::setFreqKHz ),

        new BoolProp<I2CRam>("Persistent", tr("Persistent"),""
                            , this, &I2CRam::persistent, &I2CRam::setPersistent ),
    }, groupNoCopy } );

    addPropGroup( {"Hidden", {
        new StrProp<I2CRam>("Mem","",""
                           , this, &I2CRam::getMem, &I2CRam::setMem )
    }, groupHidden} );
}
I2CRam::~I2CRam(){}

void I2CRam::initialize()
{
    TwiModule::initialize();

    if( !m_persistent ) m_ram.fill( 0 );

    m_addrPtr = 0;
    m_phase = 3;
}

void I2CRam::stamp()           // Called at Simulation Start
{
    TwiModule::setMode( TWI_SLAVE );

    for( int i=2; i<5; i++ )     // Initialize address pins
        m_inpPin[i]->changeCallBack( this );
}

void I2CRam::updateStep()
{
    IoComponent::updateStep();
    if( m_memTable ) m_memTable->updateTable( &m_ram );
}

void I2CRam::voltChanged()             // Some Pin Changed State, Manage it
{
    m_address = m_cCode;

    if( m_inpPin[2]->getInpState() ) m_address += 1;
    if( m_inpPin[3]->getInpState() ) m_address += 2;
    if( m_inpPin[4]->getInpState() ) m_address += 4;

    TwiModule::voltChanged();        // Run I2C Engine

    if( m_i2cState == I2C_STOP ) m_phase = 3;
}

void I2CRam::readByte() // Write to RAM
{
    if( m_phase == 0 )
    {
        m_phase++;
        m_addrPtr = m_rxReg<<8;
    }
    else if( m_phase == 1 )
    {
        m_phase++;
        if( m_size > 256 ) m_addrPtr += m_rxReg;
        else               m_addrPtr  = m_rxReg;
    }else{
        while( m_addrPtr >= m_size ) m_addrPtr -= m_size;
        m_ram[ m_addrPtr ] = m_rxReg;
        m_addrPtr++;
        
        if( m_addrPtr >= m_size ) m_addrPtr = 0;
    }
   TwiModule::readByte();
}

void I2CRam::startWrite()
{
    if( m_size > 256 ) m_phase = 0;
    else               m_phase = 1;
}

void I2CRam::writeByte() // Read from RAM
{
    while( m_addrPtr >= m_size ) m_addrPtr -= m_size;

    m_txReg = m_ram[ m_addrPtr ];
    if( ++m_addrPtr >= m_size ) m_addrPtr = 0;

    TwiModule::writeByte();
}

void I2CRam::setMem( QString m )
{
    if( m.isEmpty() ) return;
    MemData::setMem( &m_ram, m );
}

QString I2CRam::getMem()
{
    QString m;
    if( !m_persistent ) return m;

    return MemData::getMem( &m_ram );
}

void I2CRam::setRSize( int size )
{
    if( size > 65536 ) size = 65536;
    if( size < 1 ) size = 1;
    m_size = size;
    m_ram.resize( size );

    if( m_memTable ) m_memTable->setData( &m_ram );
}

void I2CRam::contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu )
{
    QAction* loadAction = menu->addAction( QIcon(":/load.svg"),tr("Load data") );
    QObject::connect( loadAction, &QAction::triggered, [=](){ loadData(); } );

    QAction* saveAction = menu->addAction(QIcon(":/save.png"), tr("Save data") );
    QObject::connect( saveAction, &QAction::triggered, [=](){ saveData(); } );

    QAction* showEepAction = menu->addAction(QIcon(":/save.png"), tr("Show Memory Table") );
    QObject::connect( showEepAction, &QAction::triggered, [=](){ slotShowTable(); } );

    menu->addSeparator();
    Component::contextMenu( event, menu );
}

void I2CRam::slotShowTable()
{
    MemData::showTable( m_size, 1 );
    if( m_persistent ) m_memTable->setWindowTitle( "I2C ROM: "+m_idLabel->toPlainText());
    else               m_memTable->setWindowTitle( "I2C RAM: "+m_idLabel->toPlainText() );
    m_memTable->setData( &m_ram );
}
