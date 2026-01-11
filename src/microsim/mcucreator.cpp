/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QDomDocument>
#include <QFileInfo>
#include <QDebug>
#include <QObject>
#include <math.h>

#include "mcucreator.h"
#include "circuitwidget.h"
#include "circuit.h"
#include "subcircuit.h"
#include "datautils.h"
#include "regwatcher.h"
#include "e_mcu.h"
#include "mcu.h"
#include "mcuport.h"
#include "ioport.h"
#include "mcupin.h"
#include "usarttx.h"
#include "usartrx.h"
#include "watcher.h"
#include "console.h"

// Cores
#include "avrcore.h"
#include "avrport.h"
#include "avrtimer.h"
#include "avrocunit.h"
#include "avrocm.h"
#include "avricunit.h"
#include "avrinterrupt.h"
#include "avrusart.h"
#include "avradc.h"
#include "avrtwi.h"
#include "avrspi.h"
#include "avrusi.h"
#include "avrwdt.h"
#include "avreeprom.h"
#include "avrcomparator.h"
#include "avrsleep.h"
#include "avrintosc.h"

#include "pic12core.h"
#include "pic14core.h"
#include "pic14ecore.h"
#include "picport.h"
#include "pictimer.h"
#include "picccpunit.h"
#include "picicunit.h"
#include "picinterrupt.h"
#include "pic14einterrupt.h"
#include "picusart.h"
#include "picmssp.h"
#include "pictwi.h"
#include "picspi.h"
#include "picadc.h"
#include "picdac.h"
#include "piccomparator.h"
#include "picvref.h"
#include "picwdt.h"
#include "piceeprom.h"
#include "picconfigword.h"
#include "picintosc.h"
#include "picsleep.h"

#include "i51core.h"
#include "i51timer.h"
#include "i51usart.h"
#include "i51port.h"

#include "mcs65core.h"

#include "z80core.h"
#include "ula_zx48k.h"

#include "scriptcpu.h"
#include "scriptport.h"
#include "scriptusart.h"
#include "scriptspi.h"
#include "scripttwi.h"
#include "scripttcp.h"
#include "scriptprop.h"
#include "scriptdisplay.h"

#include "utils.h"

QList<MyDisplay*> McuCreator::m_displays;
bool McuCreator::m_console;

QString McuCreator::m_core = "";
QString McuCreator::m_CompName = "";
QString McuCreator::m_basePath = "";
QString McuCreator::m_txRegName = "";
Mcu*    McuCreator::m_mcuComp = nullptr;
eMcu*   McuCreator::mcu = nullptr;
McuTwi* McuCreator::m_twi = nullptr;
McuSpi* McuCreator::m_spi = nullptr;
bool    McuCreator::m_newStack;
QDomElement McuCreator::m_stackEl;
std::vector<ScriptPerif*> McuCreator::m_scriptPerif;


int McuCreator::createMcu( Mcu* mcuComp, QString name )
{
    m_CompName = name;
    m_mcuComp  = mcuComp;
    m_newStack = false;
    m_console  = false;
    m_scriptPerif.clear();
    m_displays.clear();

    m_twi = nullptr;
    m_spi = nullptr;

    mcu = &(mcuComp->m_eMcu);
    QString dataFile = mcuComp->m_dataFile;
    m_basePath = QFileInfo( dataFile ).absolutePath();
    dataFile   = QFileInfo( dataFile ).fileName();

    int error = processFile( dataFile );

    if( error == 0 ) m_mcuComp->setupMcu();

    return error;
}

int McuCreator::processFile( QString fileName )
{
    fileName = m_basePath+"/"+fileName;
    QDomDocument domDoc = fileToDomDoc( fileName, "McuCreator::processFile" );
    if( domDoc.isNull() ) return 1;

    QDomElement root = domDoc.documentElement();

    QString tagName = root.tagName();

    if( tagName != "mcu" && tagName != "iou" && tagName != "mpu" && tagName != "parts")
    {
        QDomNode node = root.firstChild();

        while( !node.isNull() )
        {
            root = node.toElement();
            tagName = root.tagName();
            if( tagName == "mcu" || tagName == "mpu" || tagName == "iou" || tagName == "parts" ) break;
            node = node.nextSibling();
        }
    }

    if( root.hasAttribute("core") )
    {
        m_core = root.attribute("core");

        if( root.hasAttribute("data") )       createDataMem( root.attribute("data").toUInt(0,0) );
        if( root.hasAttribute("prog") )       createProgMem( root.attribute("prog").toUInt(0,0) );
        if( root.hasAttribute("progword") )   mcu->m_wordSize = root.attribute("progword").toUInt(0,0);
        if( root.hasAttribute("progpage") )   mcu->m_pgmPage = root.attribute("progpage").toUInt(0,0);
        if( root.hasAttribute("eeprom") )     createRomMem( root.attribute("eeprom").toUInt(0,0) );
        if( root.hasAttribute("inst_cycle") ) mcu->setInstCycle( root.attribute("inst_cycle").toDouble() );
        if( root.hasAttribute("cpu_cycle") )  mcu->m_cPerTick = root.attribute("cpu_cycle").toDouble();
        if( root.hasAttribute("freq") )       m_mcuComp->m_uiFreq = root.attribute("freq").toDouble();
    }

    int error = 0;
    QDomNode node = root.firstChild();
    while( !node.isNull() )
    {
        QDomElement el = node.toElement();
        QString   part = el.tagName();

        if     ( part == "regblock" )   createRegisters( &el );
        else if( part == "datablock" )  createDataBlock( &el );
        else if( part == "progblock" )  createProgBlock( &el );
        else if( part == "stack" )      { m_stackEl = el; m_newStack = true; }
        else if( part == "interrupts" ) createInterrupts( &el );
        else if( part == "port" )       createMcuPort( &el );
        else if( part == "mcuport" )    createMcuPort( &el );
        else if( part == "ioport" )     createIoPort( &el );
        else if( part == "timer" )      createTimer( &el );
        else if( part == "ocm" )        createOcm( &el );
        else if( part == "ccpunit" )    createCcpUnit( &el );
        else if( part == "msspunit" )   createMsspUnit( &el );
        else if( part == "usart" )      createUsart( &el );
        else if( part == "twi" )        createTwi( &el );
        else if( part == "spi" )        createSpi( &el );
        else if( part == "tcp" )        createTcp( &el );
        else if( part == "usi" )        createUsi( &el );
        else if( part == "adc" )        createAdc( &el );
        else if( part == "dac" )        createDac( &el );
        else if( part == "comp" )       createAcomp( &el );
        else if( part == "vref" )       createVref( &el );
        else if( part == "wdt" )        createWdt( &el );
        else if( part == "rom" )        createEeprom( &el );
        else if( part == "sleep" )      createSleep( &el );
        else if( part == "configwords") createCfgWord( &el );
        else if( part == "intosc")      createIntOsc( &el );
        else if( part == "display" )    createDisplay( &el );
        else if( part == "console" )    m_console = true;

        else if( part == "include" )
        {
            error = processFile( el.attribute("file") );
            if( error ) return error;
        }
        node = node.nextSibling();
    }
    if( root.hasAttribute("core") )
    {
        if     ( m_core == "AVR" )      mcu->m_cpu = new AvrCore( mcu );
        else if( m_core == "Pic12" )    mcu->m_cpu = new Pic12Core( mcu );
        else if( m_core == "Pic14" )    mcu->m_cpu = new Pic14Core( mcu );
        else if( m_core == "Pic14e" )   mcu->m_cpu = new Pic14eCore( mcu );
        else if( m_core == "8051" )     mcu->m_cpu = new I51Core( mcu );
        else if( m_core == "6502" )     mcu->m_cpu = new Mcs65Cpu( mcu );
        else if( m_core == "Z80" )      mcu->m_cpu = new Z80Core( mcu );
        else if( m_core == "Z80ULA" )   mcu->m_cpu = new ULA_ZX48k( mcu );
        else if( m_core == "scripted" )
        {
            ScriptCpu* cpu = new ScriptCpu( mcu );
            mcu->m_cpu = cpu;
            m_mcuComp->m_scripted = true;

            if( root.attribute("linkable") == "true"
             || root.attribute("linker"  ) == "true" )
                m_mcuComp->setScriptLinker( cpu );

            node = root.firstChild();
            while( !node.isNull() )
            {
                QDomElement e = node.toElement();
                QString  part = e.tagName();

                if( part == "propertygroup" )  // Properties in .mcu file
                {
                    SubCircuit* subC = Circuit::self()->getSubcircuit();
                    QString group = e.attribute("name");

                    QList<ComProperty*> mcuProps;
                    QList<ComProperty*> subProps;

                    QDomNode node = e.firstChild();
                    while( !node.isNull() )
                    {
                        QDomElement el = node.toElement();

                        if( el.tagName() == "property" )
                        {
                            QString name  = el.attribute("name");
                            QString label = el.attribute("label");
                            QString type  = el.attribute("type");
                            QString unit  = el.attribute("unit");

                            if( label.isEmpty() ) label = name;

                            ComProperty* p = cpu->addProperty( name, label, type, unit );

                            if( el.attribute("add") == "true" ) subProps.append( p );
                            else                                mcuProps.append( p );
                        }
                        node = node.nextSibling();
                    }
                    propGroup* pg = m_mcuComp->getPropGroup( group );
                    if( pg ){
                        for( ComProperty* p : mcuProps ) m_mcuComp->addProperty( group, p );
                    }
                    else m_mcuComp->addPropGroup( {group, mcuProps, 0} );

                    if( subC ){
                        pg = subC->getPropGroup( group );
                        if( pg ){
                            for( ComProperty* p : subProps ) subC->addProperty( group, p/*, false*/ ); // If false properties are saved but not set at circuit open
                        }
                        else subC->addPropGroup( {group, subProps, 0}/*, false*/ );
                    }
                }
                node = node.nextSibling();
            }
            cpu->setPeriferals( m_scriptPerif );

            cpu->setScriptFile( m_basePath+"/"+root.attribute("script") );
        }
        else mcu->m_cpu = new Mcu8bits( mcu );

        if( m_displays.size() )
        {
            mcu->m_cpu->createWatcher();
            for( MyDisplay* display : m_displays )
            {
                mcu->m_cpu->m_display = display;
                mcu->m_cpu->getWatcher()->addWidget( display );
            }
        }
        if( m_console )
        {
            mcu->m_cpu->createWatcher();
            mcu->m_cpu->getWatcher()->addConsole();
        }
        if( m_newStack ) createStack( &m_stackEl );
    }

    if( root.hasAttribute("clkpin") )
    {
        mcu->m_clkPin = mcu->getIoPin( root.attribute("clkpin") );
    }
    return 0;
}

void McuCreator::createCfgWord( QDomElement* e )
{
    QString name = e->attribute("name");
    QString type = e->attribute("type");

    ConfigWord* cfgWord = nullptr;
    if( m_core.startsWith("Pic14") ) cfgWord = PicConfigWord::createCfgWord( mcu, name, type );
    if( !cfgWord ) return;

    mcu->m_cfgWord = cfgWord;

    QDomNode node = e->firstChild();
    while( !node.isNull() )
    {
        QDomElement el = node.toElement();
        if( el.tagName() == "word" )
        {
            //QString name = el.attribute("name");
            uint16_t addr  = el.attribute("address").toUInt( nullptr, 16 );
            uint16_t value = el.attribute("value").toUInt( nullptr, 16 );
            cfgWord->m_cfgWords.insert( addr, value );
        }
        node = node.nextSibling();
    }
}

void McuCreator::createProgMem( uint32_t size )
{
    mcu->m_flashSize = size;
    if     ( m_core == "Pic12" )  mcu->m_progMem.resize( size, 0x0FFF );
    else if( m_core == "Pic14" )  mcu->m_progMem.resize( size, 0x3FFF );
    else if( m_core == "Pic14e" ) mcu->m_progMem.resize( size, 0x3FFF );
    else                          mcu->m_progMem.resize( size, 0xFFFF );
}

void McuCreator::createDataMem( uint32_t size )
{
    mcu->m_ramSize = size;
    mcu->m_dataMem.resize( size, 0 );
    mcu->m_addrMap.resize( size, 0xFFFF ); // Not Maped values = 0xFFFF -> don't exist
}

void McuCreator::createRomMem( uint32_t size )
{
    if( size == 0 ) return;

    mcu->m_romSize = size;
    mcu->m_eeprom.resize( size );
    mcu->m_eeprom.fill( 0xFF );
}

void McuCreator::createEeprom( QDomElement* e )
{
    McuEeprom* eeprom = nullptr;
    QString eepromName = e->attribute("name");

    if     ( m_core == "AVR" )    eeprom = new AvrEeprom( mcu, eepromName );
    else if( m_core == "Pic14" )  eeprom = new PicEeprom( mcu, eepromName );
    else if( m_core == "Pic14e" ) eeprom = new PicEeprom( mcu, eepromName );
    else return;

    mcu->m_modules.emplace_back( eeprom );

    setConfigRegs( e, eeprom );

    if( e->hasAttribute("dataregs") ){
        QStringList regs = e->attribute("dataregs").split(",");
        eeprom->m_dataReg = mcu->getReg( regs.value(0) );
    }

    if( e->hasAttribute("addressreg") )
    {
        QStringList regs = e->attribute("addressreg").split(",");
        QString lowByte  = regs.value(0);
        QString highByte = regs.value(1);

        if( !lowByte.isEmpty() ){
            eeprom->m_addressL = mcu->getReg( lowByte );
            watchRegNames( lowByte, R_WRITE, eeprom, &McuEeprom::addrWriteL, mcu );
        }
        if( !highByte.isEmpty() ){
            eeprom->m_addressH = mcu->getReg( highByte );
            watchRegNames( highByte, R_WRITE, eeprom, &McuEeprom::addrWriteH, mcu );
    }   }
    setInterrupt( e->attribute("interrupt"), eeprom );
}

void McuCreator::createDataBlock( QDomElement* d )
{
    uint16_t datStart = d->attribute("start").toUInt(0,0);
    uint16_t datEnd   = d->attribute("end").toUInt(0,0);
    uint16_t mapTo    = datStart;

    if( datEnd >= mcu->m_ramSize )
    {
        qDebug() << "McuCreator::createDataBlock  ERROR creating DataBlock";
        qDebug() << "dataMemSize  = " << mcu->m_ramSize;
        qDebug() << "dataBlockEnd = " << datEnd;
        return;
    }
    if( d->hasAttribute("mapto") ) mapTo = d->attribute("mapto").toUInt(0,0);

    for( int i=datStart; i<=datEnd; ++i )
    {
        if( mcu->m_addrMap.at(i) == 0xFFFF ) mcu->m_addrMap.at(i) = mapTo; // Don't overwrite previous values
        mapTo++;
    }
    getRegisters( d );
}

void McuCreator::createRegisters( QDomElement* e )
{
    uint16_t regStart = e->attribute("start").toUInt(0,0);
    uint16_t regEnd   = e->attribute("end").toUInt(0,0);
    uint16_t offset   = e->attribute("offset").toUInt(0,0);

    if( regEnd >= mcu->m_ramSize )
    {
        qDebug() << "McuCreator::createRegisters  ERROR creating Registers";
        qDebug() << "dataMemSize  = " << mcu->m_ramSize;
        qDebug() << "RegistersEnd = " << regEnd;
        return;
    }
    if( regStart < mcu->m_regStart ) mcu->m_regStart = regStart;
    if( regEnd   > mcu->m_regEnd )
    {
        mcu->m_regEnd = regEnd;
        mcu->m_regMask.resize( regEnd+1, 0xFF );
    }
    for( int i=regStart; i<=regEnd; ++i )
        if( mcu->m_addrMap.at(i) == 0xFFFF ) mcu->m_addrMap.at(i) = i; // Don't overwrite previous values

    getRegisters( e, offset );
}

void McuCreator::getRegisters( QDomElement* e, uint16_t offset )
{
    QString stReg;
    if( e->hasAttribute("streg") ) stReg = e->attribute("streg");

    QDomNode node = e->firstChild();
    while( !node.isNull() ) // Create Registers
    {
        QDomElement el = node.toElement();

        if( el.tagName() == "register" )
        {
            QString  regName = el.attribute("name");
            uint16_t regAddr = el.attribute("addr").toUInt(0,0)+offset;

            if( regAddr >= mcu->m_ramSize )
            {
                qDebug() << "McuCreator::getRegisters  ERROR creating Register"<< regName ;
                qDebug() << "dataMemSize  = " << mcu->m_ramSize;
                qDebug() << "Register Address = " << regAddr<<"\n";
            }
            else{
                uint8_t resetVal = el.attribute("reset").toUInt(0,2);
                QString    wMask = el.attribute("mask");
                QString     bits = el.attribute("bits");

                if( !wMask.isEmpty() )
                {
                    if( regAddr >= mcu->m_regMask.size() )
                    {
                        qDebug() << "McuCreator::getRegisters  ERROR creating Register Mask"<< regName ;
                        qDebug() << "regMask Size  = " << mcu->m_regMask.size();
                        qDebug() << "Register Address = " << regAddr<<"\n";
                    }
                    else mcu->m_regMask.at(regAddr) = wMask.toUInt(0,2);
                }
                mcu->m_addrMap.at(regAddr) = regAddr;

                regInfo_t regInfo = { regAddr, resetVal/*, writeMask*/ };
                mcu->m_regInfo.insert( regName, regInfo );

                if( !bits.isEmpty() )                    // Create bitMasks
                {
                    QString bitName;
                    QStringList bitList = bits.split(",");
                    for( int i=0; i<bitList.size(); ++i )
                    {
                        bitName = bitList.value( i );
                        if( bitName == "0" ) continue;

                        for( QString alias : bitName.split("|") ) // Bit name variations: alias first used bit name
                        {                                         // Example tiny WDTCR.WDTIE is 328 WDTCSR.WDIE
                            mcu->m_bitMasks.insert( alias, 1<<i );
                            mcu->m_bitRegs.insert( alias, regAddr );
                    }   }
                    if( !stReg.isEmpty() && ( regName == stReg ) ) // Status Register
                    {
                        mcu->m_sregAddr = regAddr;
                        mcu->setStatusBits( bitList );
                    }
                }
            }
        }
        else if( el.tagName() == "mapped" )
        {
            uint16_t regAddr = el.attribute("addr").toUInt(0,0)+offset;
            uint16_t mapTo   = el.attribute("mapto").toUInt(0,0);
            mcu->m_addrMap.at(regAddr) = mapTo;
        }
        node = node.nextSibling();
}   }

void McuCreator::createProgBlock( QDomElement* p )
{
    QDomNode node = p->firstChild();
    while( !node.isNull() )
    {
        QDomElement el = node.toElement();
        if( el.tagName() == "progval" )
        {
            uint16_t addr  = el.attribute("addr").toUInt(0,0);
            uint16_t value = el.attribute("value").toUInt(0,0);

            if( addr >= mcu->m_flashSize )
            {
                qDebug() << "McuCreator::createProgBlock  ERROR writing Program Memory";
                qDebug() << "Address:" << addr << "> PMG End:" << mcu->m_flashSize;
            }
            else mcu->setFlashValue( addr, value );
        }
        node = node.nextSibling();
}   }

void McuCreator::createIntOsc( QDomElement* p )
{
    QString name = p->attribute("name");
    QString type = p->attribute("type");

    McuIntOsc* intOsc = nullptr;
    if( m_core.startsWith("Pic14") ) intOsc = PicIntOsc::createIntOsc( mcu, name, type );
    else if( m_core == "AVR")        intOsc = new AvrIntOsc( mcu, "intOsc");
    else                             intOsc = new McuIntOsc( mcu, "intOsc");

    mcu->m_modules.emplace_back( intOsc );
    mcu->m_intOsc = intOsc;

    setConfigRegs( p, intOsc );

    if( p->hasAttribute("clockpins") )
    {
        QStringList pins = p->attribute("clockpins").split(",");
        for( int i=0; i<pins.size(); ++i )
            mcu->m_intOsc->setPin( i, mcu->getMcuPin( pins.value(i) ) );
    }
    if( p->hasAttribute("clockoutpin") )
    {
        mcu->m_intOsc->setPin( 2, mcu->getMcuPin( p->attribute("clockoutpin") ) );
    }    
}

void McuCreator::createIoPort( QDomElement* p )
{
    QString name = p->attribute("name");

    IoPort* port = new IoPort( name );
    mcu->m_ioPorts.insert( name, port );

    port->createPins( m_mcuComp, p->attribute("pins"), 0xFFFFFFFF );
}

void McuCreator::createMcuPort( QDomElement* p )
{
    QString name = p->attribute("name");

    McuPort* port;
    if     ( m_core == "AVR" )    port = new AvrPort( mcu, name );
    else if( m_core == "Pic12" )  port = new PicPort( mcu, name );
    else if( m_core == "Pic14" )  port = new PicPort( mcu, name );
    else if( m_core == "Pic14e" ) port = new PicPort( mcu, name );
    else if( m_core == "8051" )   port = new I51Port( mcu, name );
    else{
        if( p->hasAttribute("type") && ( p->attribute("type") == "scripted" ))
        {
            port = new ScriptPort( mcu, name );
            ScriptPort* sPort = (ScriptPort*)port;
            sPort->setScriptFile( m_basePath+"/"+p->attribute("script"));
        }
        else port = new McuPort( mcu, name );
    }
    mcu->m_mcuPorts.insert( name, port );
    mcu->m_modules.emplace_back( port );

    uint32_t pinMask=0xFFFFFFFF;
    if( p->hasAttribute("pinmask") ) pinMask = p->attribute("pinmask").toUInt( 0, 2 );
    port->createPins( m_mcuComp, p->attribute("pins"), pinMask );

    setConfigRegs( p, port );

    if( p->hasAttribute("resetpin") )
        m_mcuComp->m_portRstPin = port->getPin( p->attribute("resetpin") );

    QString Oreg = "";
    if( p->hasAttribute( "outreg" ) )
    {
        QStringList outRegList = p->attribute( "outreg" ).split(",");
        Oreg = outRegList.first();
        for( QString outReg : outRegList )
        {
            uint16_t addr = mcu->getRegAddress( outReg );
            port->m_outAddr = addr;
            port->m_outReg  = mcu->getReg( outReg );
            watchRegNames( outReg, R_WRITE, port, &McuPort::outChanged, mcu );
    }   }
    if( p->hasAttribute( "inreg" ) ) // Connect to PORT In Register
    {
        QString inReg = p->attribute( "inreg" );
        uint16_t inAddr = mcu->getRegAddress( inReg );
        port->m_inAddr = inAddr;
        port->m_inReg  = mcu->getReg( inReg );
        if( m_core == "AVR" ) watchRegister( inAddr, R_WRITE, (AvrPort*)port, &AvrPort::pinRegChanged, mcu );
    }
    else if( !Oreg.isEmpty() ) watchRegNames( Oreg, R_READ, port, &McuPort::readPort, mcu ); // No Input register, read pin states

    if( p->hasAttribute( "dirreg" ) ) // Connect to PORT Dir Register
    {
        QString dirreg = p->attribute( "dirreg" );
        if( dirreg.startsWith("!") )
        {
            port->m_dirInv = true;
            dirreg.remove( 0, 1 );
        }
        uint16_t addr = mcu->getRegAddress( dirreg );
        port->m_dirAddr = addr;
        port->m_dirReg  = mcu->getReg( dirreg );
        watchRegNames( dirreg, R_WRITE, port, &McuPort::dirChanged, mcu );
    }
    if( p->hasAttribute("outmask") ) // Permanent Outputs
    {
        uint outMask = p->attribute("outmask").toUInt( 0, 2 );
        for( int i=0; i<port->m_numPins; ++i )
            port->m_pins[i]->m_outMask = outMask & 1<<i;
    }
    if( p->hasAttribute("inpmask") ) // Permanent Inputs
    {
        uint inpMask = p->attribute("inpmask").toUInt( 0, 2 );
        for( int i=0; i<port->m_numPins; ++i )
            port->m_pins[i]->m_inpMask = inpMask & 1<<i;
    }
    if( p->hasAttribute("pullups") )
    {
        QString pullups = p->attribute( "pullups" );
        bool ok = false;
        uint pullup = pullups.toUInt( &ok, 2 );
        if( ok ){
            for( int i=0; i<port->m_numPins; ++i )          // Permanent Pullups
                port->m_pins[i]->m_puMask = pullup & 1<<i;
        }else{
            if( mcu->regExist( pullups ) )
                watchRegNames( pullups, R_WRITE, port, &McuPort::setPullups, mcu );
            else{
                if( pullups.startsWith("!") ){
                    pullups = pullups.remove( 0,1 );
                    watchBitNames( pullups, R_WRITE, port, &McuPort::clearAllPullups, mcu );
                }
                else watchBitNames( pullups, R_WRITE, port, &McuPort::setAllPullups, mcu );
            }
        }
    }
    if( p->hasAttribute("opencol") ) // OPen Drain
    {
        uint opencol = p->attribute("opencol").toUInt( 0, 2 );
        for( int i=0; i<port->m_numPins; ++i )
            if( opencol & 1<<i ) port->m_pins[i]->m_openColl = true;
    }
    QDomNode node = p->firstChild();
    while( !node.isNull() )
    {
        QDomElement el = node.toElement();

        if( el.tagName() == "interrupt" )
        {
            setInterrupt( el.attribute("name"), port );  // Pin change interrupt

            if( el.hasAttribute("mask") )
            {
                QString mask = el.attribute( "mask" );
                bool ok = false;
                int bits = mask.toInt( &ok,2 );
                port->rstIntMask( !ok );
                if( ok ) port->setIntMask( bits ); // Hardcode Interrupt to Pin
                else                               // Interrupt controlled by register
                {
                    watchRegNames( mask, R_WRITE, port, &McuPort::intChanged, mcu );
                }
            }
            else if(  el.hasAttribute("bitmask") )
            {
                QString bitNames = el.attribute( "bitmask" );
                port->m_intBits = getRegBits( bitNames, mcu );
                watchBitNames( bitNames, R_WRITE, port, &McuPort::intChanged, mcu );
            }
        }
        else if( el.tagName() == "extint" )
        {
            QString intName = el.attribute("name");
            Interrupt* inte = mcu->m_interrupts.m_intList.value( intName );

            McuPin* pin = port->getPin( el.attribute("pin") );
            if( pin ) pin->m_extInt = inte;

            if( el.hasAttribute("configbits") )
            {
                QString configBits = el.attribute("configbits");
                watchBitNames( configBits, R_WRITE, pin, &McuPin::ConfExtInt, mcu );
                if( pin ) pin->m_extIntBits = getRegBits( configBits, mcu );
        }   }
        node = node.nextSibling();
}   }

void McuCreator::createTimer( QDomElement* t )
{
    McuTimer* timer = nullptr;
    QString timerName = t->attribute("name");
    int type = t->attribute("type").toInt();

    if     ( m_core == "8051" )   timer = new I51Timer( mcu, timerName );
    else if( m_core == "AVR" )    timer = AvrTimer::createTimer( mcu, timerName, type );
    else if( m_core == "Pic12" )  timer = PicTimer::createTimer( mcu, timerName, type );
    else if( m_core == "Pic14" )  timer = PicTimer::createTimer( mcu, timerName, type );
    else if( m_core == "Pic14e" ) timer = PicTimer::createTimer( mcu, timerName, type );

    if( !timer ) { qDebug() << "Error creating Timer"<< timerName; return; }

    mcu->m_timerList.insert( timerName, timer );
    mcu->m_modules.emplace_back( timer );

    setConfigRegs( t, timer );

    if( t->hasAttribute("counter") )
    {
        QStringList regs = t->attribute("counter").split(",");
        QString lowByte  = regs.value(0);
        QString highByte = regs.value(1);

        if( !lowByte.isEmpty() ){
            timer->m_countL = mcu->getReg( lowByte );
            watchRegNames( lowByte, R_WRITE, timer, &McuTimer::countWriteL, mcu );
            watchRegNames( lowByte, R_READ,  timer, &McuTimer::updtCount, mcu );
        }
        if( !highByte.isEmpty() ){
            timer->m_countH = mcu->getReg( highByte );
            watchRegNames( highByte, R_WRITE, timer, &McuTimer::countWriteH, mcu );  // Low byte triggers read/write operations
            watchRegNames( highByte, R_READ,  timer, &McuTimer::updtCount, mcu );
    }   }
    if( t->hasAttribute("enable") )
    {
        QString enable = t->attribute("enable");
        watchBitNames( enable, R_WRITE, timer, &McuTimer::enable, mcu );
    }

    if( t->hasAttribute("clockpin") )
    {
        QStringList pinList = t->attribute("clockpin").split(",");
        timer->setClockPins( pinList );
    }

    QString topReg0 = "";
    if( t->hasAttribute("topreg0") ) /// Still done in AvrTimer16bit
    {
        topReg0 = t->attribute("topreg0");
        QStringList list = topReg0.split(",");

        timer->m_topReg0L = mcu->getReg( list.value(0) );
        watchRegNames( topReg0, R_WRITE, timer, &McuTimer::topReg0Changed, mcu );

        if( list.size() > 1 ) timer->m_topReg0H = mcu->getReg( list.value(1) );
    }
    setInterrupt( t->attribute("interrupt"), timer );
    setPrescalers( t->attribute("prescalers"), timer );
    if( t->hasAttribute("prselect") ) timer->m_prSelBits = getRegBits( t->attribute("prselect"), mcu );

    QDomNode node = t->firstChild();
    while( !node.isNull() )
    {
        QDomElement el = node.toElement();

        if( el.tagName() == "ocunit" )
        {
            QString ocName = el.attribute("name");
            McuOcUnit* ocUnit = nullptr;
            if     ( m_core == "AVR" ) ocUnit = new AvrOcUnit( mcu, ocName );
            //else if( m_core == "Pic14" ) ocUnit = new PicOcUnit( mcu, el.attribute("name") );
            if( !ocUnit ) { node = node.nextSibling(); continue; }

            timer->addOcUnit( ocUnit );
            ocUnit->m_timer = timer;
            ocUnit->m_ocPin = mcu->getMcuPin( el.attribute("pin") );

            QStringList regs = el.attribute("ocreg").split(",");
            QString lowByte = regs.value(0);
            QString highByte =regs.value(1);

            if( !lowByte.isEmpty() ){
                uint8_t* ocRegL = mcu->getReg( lowByte );
                ocUnit->m_ocRegL = ocRegL;
                if( topReg0 == "" && ocName.endsWith("A") ) // OCRA managed in Timer
                {
                    timer->m_topReg0L = ocRegL;
                    watchRegNames( lowByte, R_WRITE, timer, &McuTimer::topReg0Changed, mcu );
                }
                else watchRegNames( lowByte, R_WRITE, ocUnit, &McuOcUnit::ocrWriteL, mcu );
            }
            if( !highByte.isEmpty() ){
                uint8_t* ocRegH = mcu->getReg( highByte );
                ocUnit->m_ocRegH = ocRegH;
                if( ocName.endsWith("A") ) timer->m_topReg0H = ocRegH;
                /// Low byte triggers red/write operations
                watchRegNames( highByte, R_WRITE, ocUnit, &McuOcUnit::ocrWriteH, mcu );
            }
            if( el.hasAttribute("bits") ) // This doesn't watch register, configure must be called from Timer
            {
                ocUnit->m_configBitsA = getRegBits( el.attribute("bits"), mcu );
            }
            else setConfigRegs( &el, ocUnit );

            setInterrupt( el.attribute("interrupt"), ocUnit );
        }
        else if( el.tagName() == "icunit" )
        {
            QString icName = el.attribute("name");
            McuIcUnit* icUnit = nullptr;
            if( m_core == "AVR" ) icUnit = new AvrIcUnit( mcu, icName );
            if( !icUnit ) { node = node.nextSibling(); continue; }

            timer->m_ICunit = icUnit;
            icUnit->m_timer = timer;
            icUnit->m_icPin = mcu->getMcuPin( el.attribute("pin") );

            QStringList regs = el.attribute("icreg").split(",");
            if( !regs.isEmpty() ) icUnit-> m_icRegL = mcu->getReg( regs.value(0) );
            if( !regs.isEmpty() ) icUnit-> m_icRegH = mcu->getReg( regs.value(1) );

            if( el.hasAttribute("bits") )
            {
                QString configBits = el.attribute("bits");
                watchBitNames( configBits, R_WRITE, icUnit, &McuIcUnit::configure, mcu );
            }
            setInterrupt( el.attribute("interrupt"), icUnit );
        }
        node = node.nextSibling();
}   }

void McuCreator::createOcm( QDomElement* e )
{
    McuOcm* ocm = nullptr;
    QString name = e->attribute("name");

    if( m_core == "AVR" ) ocm = new AvrOcm( mcu, name );
    if( !ocm ) return;

    mcu->m_modules.emplace_back( ocm );
    setConfigRegs( e, ocm );

    QStringList ocunits = e->attribute("ocunits").split(",");
    for( int i=0; i<2; i++ )
    {
        QString ocunit = ocunits.takeFirst();
        McuTimer* timer = mcu->getTimer("TIMER"+ocunit.mid( 2, 1) );

        McuOcUnit* ocUnit = timer->getOcUnit( ocunit );
        ocUnit->m_ocm = ocm;
        if( i == 0 ) ocm->m_OC1 = ocUnit;
        else         ocm->m_OC2 = ocUnit;
    }
}

void McuCreator::createCcpUnit( QDomElement* c )
{
    PicCcpUnit* ccpUnit = nullptr;
    if     ( m_core == "Pic14" ) ccpUnit = new PicCcpUnit( mcu, c->attribute("name"), c->attribute("type").toInt() );
    else if( m_core == "Pic14e") ccpUnit = new PicCcpUnit( mcu, c->attribute("name"), c->attribute("type").toInt() );
    if( !ccpUnit ) return;

    mcu->m_modules.emplace_back( ccpUnit );

    ccpUnit->setPin( mcu->getMcuPin( c->attribute("pin") ) );

    QStringList regs = c->attribute("ccpreg").split(",");
    QString lowByte  = regs.value(0);
    QString highByte = regs.value(1);

    if( !lowByte.isEmpty() ){
        uint8_t* regL = mcu->getReg( lowByte );
        ccpUnit->m_ccpRegL = regL;
        ccpUnit->m_capUnit->m_icRegL = regL;
        watchRegNames( lowByte, R_WRITE, ccpUnit, &PicCcpUnit::ccprWriteL, mcu );
    }
    if( !highByte.isEmpty() ){
        uint8_t* regH = mcu->getReg( highByte );
        ccpUnit->m_ccpRegH = regH;
        ccpUnit->m_capUnit->m_icRegH = regH;
        watchRegNames( highByte, R_WRITE, ccpUnit, &PicCcpUnit::ccprWriteH, mcu );
    }
    setConfigRegs( c, ccpUnit );
    setInterrupt( c->attribute("interrupt"), ccpUnit );
}

void McuCreator::createMsspUnit( QDomElement* c )
{
    PicMssp* msspUnit = new PicMssp(  mcu, c->attribute("name"), c->attribute("type").toInt() );

    mcu->m_modules.emplace_back( msspUnit );
    setConfigRegs( c, msspUnit );

    QDomNode node = c->firstChild();
    while( !node.isNull() )
    {
        QDomElement el = node.toElement();
        if     ( el.tagName() == "twi") createTwi( &el );
        else if( el.tagName() == "spi") createSpi( &el );
        node = node.nextSibling();
    }
    msspUnit->m_twiUnit = static_cast<PicTwi*>( m_twi );
    msspUnit->m_spiUnit = static_cast<PicSpi*>( m_spi );
}

void McuCreator::createUsart( QDomElement* u )
{
    QString name = u->attribute( "name" );
    int   number = u->attribute( "number" ).toInt();

    QString core = m_core;
    if( u->hasAttribute("core") ) core = u->attribute("core");

    McuUsart* usartM;
    if     ( core == "8051" )    usartM = new I51Usart( mcu, name, number );
    else if( core == "AVR"  )    usartM = new AvrUsart( mcu, name, number );
    else if( core == "Pic14")    usartM = new PicUsart( mcu, name, number );
    else if( core == "Pic14e")   usartM = new PicUsart( mcu, name, number );
    else if( core == "scripted"){
        ScriptUsart* su = new ScriptUsart( mcu, name, number );
        usartM = su;
        m_scriptPerif.push_back( su );
    }
    else return;

    mcu->m_usarts.emplace_back( usartM );
    mcu->m_modules.emplace_back( usartM );

    setConfigRegs( u, usartM );
    setInterrupt( u->attribute("interrupt"), usartM );

    QDomNode node = u->firstChild();
    while( !node.isNull() )
    {
        QDomElement el = node.toElement();

        if( el.tagName() == "trunit" )
        {
            UartTR* trUnit = nullptr;
            QString type = el.attribute( "type" );
            if( type == "tx" )
            {
                trUnit = usartM->m_sender;
                m_txRegName = el.attribute( "register" );
                watchRegNames( m_txRegName, R_WRITE, usartM, &McuUsart::sendByte, mcu );
            }
            else if( type == "rx" )
            {
                trUnit = usartM->m_receiver;
                QString regName = el.attribute( "register" );
                if( regName.isEmpty() ) regName = m_txRegName; // Tx and rx using the same register
                watchRegNames( regName, R_READ, usartM, &McuUsart::readByte, mcu );
            }
            if( trUnit )
            {
                setConfigRegs( &el, trUnit );

                QStringList pinNames = el.attribute( "pin" ).split(",");
                QList<IoPin*> pinList;
                for( QString pinName : pinNames ) pinList.append( mcu->getIoPin( pinName ) );
                trUnit->setPins( pinList );

                if( el.hasAttribute("enable") )
                {
                    QString enable = el.attribute( "enable" );
                    watchBitNames( enable, R_WRITE, trUnit, &UartTR::enable, mcu );
                }
                setInterrupt( el.attribute("interrupt"), trUnit );
            }
        }
        node = node.nextSibling();
}   }

void McuCreator::createAdc( QDomElement* e )
{
    McuAdc* adc = nullptr;
    QString name = e->attribute( "name" );
    int type = e->attribute("type").toInt();

    if     ( m_core == "AVR" )   adc = AvrAdc::createAdc( mcu, name, type );
    else if( m_core == "Pic14")  adc = PicAdc::createAdc( mcu, name, type );
    else if( m_core == "Pic14e") adc = PicAdc::createAdc( mcu, name, type );
    if( !adc ) return;

    mcu->m_modules.emplace_back( adc );

    setConfigRegs( e, adc );

    if( e->hasAttribute("bits") )
    {
        bool ok = false;
        int bits = e->attribute("bits").toInt( &ok );
        if( ok ) adc->m_maxValue = pow( 2, bits )-1;
    }
    if( e->hasAttribute("dataregs") )
    {
        QStringList regs = e->attribute("dataregs").split(",");
        QString lowByte  = regs.value(0);
        QString highByte = regs.value(1);

        if( !lowByte.isEmpty() )  adc->m_ADCL = mcu->getReg( lowByte );
        if( !highByte.isEmpty() ) adc->m_ADCH = mcu->getReg( highByte );
    }
    if( e->hasAttribute("multiplex") )
    {
        QString configRegs = e->attribute("multiplex");
        watchRegNames( configRegs, R_WRITE, adc, &McuAdc::setChannel, mcu );
    }
    setInterrupt( e->attribute("interrupt"), adc );
    setPrescalers( e->attribute("prescalers"), adc );

    QStringList pins = e->attribute("adcpins").remove(" ").split(",");
    for( QString pinName : pins )
    {
        McuPin* pin = mcu->getMcuPin( pinName );
        if( pin ) adc->m_adcPin.emplace_back( pin );
    }
    if( e->hasAttribute("vrefpins") )
    {
        pins = e->attribute("vrefpins").remove(" ").split(",");
        for( QString pinName : pins )
        {
            McuPin* pin = mcu->getMcuPin( pinName );
            if( pin ) adc->m_refPin.emplace_back( pin );
    }   }
}

void McuCreator::createDac( QDomElement* e )
{
    McuDac* dac = nullptr;
    QString name = e->attribute( "name" );

    if( m_core == "Pic14e") dac = new PicDac( mcu, name );
    else return;

    mcu->m_modules.emplace_back( dac );

    setConfigRegs( e, dac );

    if( e->hasAttribute("dacreg") )
    {
        QString dacReg = e->attribute("dacreg");
        dac->m_dacReg = mcu->getReg( dacReg );
        watchRegNames( dacReg, R_WRITE, dac, &McuDac::outRegChanged, mcu );
    }
    if( e->hasAttribute("pins") )
    {
        QStringList pins = e->attribute("pins").remove(" ").split(",");
        for( QString pinName : pins )
        {
            McuPin* pin = mcu->getMcuPin( pinName );
            if( pin ) dac->m_pins.emplace_back( pin );
    }   }
}

void McuCreator::createAcomp( QDomElement* e )
{
    McuComp* comp = nullptr;
    QString name = e->attribute( "name" );
    int type = e->attribute("type").toInt();

    if     ( m_core == "AVR" )    comp = new AvrComp( mcu, name );
    else if( m_core == "Pic12" )  comp = PicComp::createComparator( mcu, name, type );
    else if( m_core == "Pic14" )  comp = PicComp::createComparator( mcu, name, type );
    else if( m_core == "Pic14e" ) comp = PicComp::createComparator( mcu, name, type );
    if( !comp ) return;

    mcu->m_modules.emplace_back( comp );
    mcu->m_comparator = comp;
    setConfigRegs( e, comp );

    QStringList pins = e->attribute( "pins" ).split(",");
    for( QString pinName : pins )
    {
        McuPin* pin = mcu->getMcuPin( pinName );
        if( pin ) comp->m_pins.emplace_back( pin );
    }
    setInterrupt( e->attribute("interrupt"), comp );
}

void McuCreator::createVref( QDomElement* e )
{
    QString name = e->attribute( "name" );
    McuVref* vref = nullptr;
    if     ( m_core == "Pic14" )  vref = new PicVref( mcu, name );
    else if( m_core == "Pic14e" ) vref = new PicVrefE( mcu, name );
    if( !vref ) return;

    mcu->m_modules.emplace_back( vref );
    mcu->m_vrefModule = vref;

    setConfigRegs( e, vref );

    if( e->hasAttribute("pinout") )
        vref->m_pinOut = mcu->getMcuPin( e->attribute("pinout") );
}

void McuCreator::createTwi( QDomElement* e )
{
    QString name = e->attribute( "name" );

    if     ( m_core == "AVR" )  m_twi = new AvrTwi( mcu, name );
    else if( m_core == "Pic14") m_twi = new PicTwi( mcu, name );
    else if( m_core == "scripted"){
        ScriptTwi* twi = new ScriptTwi( mcu, name );
        m_twi = twi;
        m_scriptPerif.push_back( twi );
    }
    else return;

    mcu->m_modules.emplace_back( m_twi );

    setConfigRegs( e, m_twi );

    if( e->hasAttribute("dataregs") )
    {
        QString dataReg = e->attribute("dataregs");
        m_twi->m_dataReg = mcu->getReg( dataReg );
        watchRegNames( dataReg, R_WRITE, m_twi, &McuTwi::writeTwiReg, mcu );
        if( m_core == "Pic14")
            watchRegNames( dataReg, R_READ, m_twi, &McuTwi::readTwiReg, mcu ); //
    }
    if( e->hasAttribute("addressreg") )
    {
        QString addrReg = e->attribute("addressreg");
        m_twi->m_addrReg = mcu->getReg( addrReg );
        watchRegNames( addrReg, R_WRITE, m_twi, &McuTwi::writeAddrReg, mcu );
    }
    if( e->hasAttribute("statusreg") )
    {
        QString statReg = e->attribute("statusreg");
        m_twi->m_statReg = mcu->getReg( statReg );
        watchRegNames( statReg, R_WRITE, m_twi, &McuTwi::writeStatus, mcu );
    }
    setInterrupt( e->attribute("interrupt"), m_twi );
    setPrescalers( e->attribute("prescalers"), m_twi );

    QStringList pins = e->attribute("pins").remove(" ").split(",");
    m_twi->setSdaPin( mcu->getIoPin( pins.value(0) ) );
    m_twi->setSclPin( mcu->getIoPin( pins.value(1) ) );
}

void McuCreator::createSpi( QDomElement* e )
{
    QString name = e->attribute( "name" );

    if     ( m_core == "AVR" )  m_spi = new AvrSpi( mcu, name );
    else if( m_core == "Pic14") m_spi = new PicSpi( mcu, name );
    else if( m_core == "scripted"){
        ScriptSpi* su = new ScriptSpi( mcu, name );
        m_spi = su;
        m_scriptPerif.push_back( su );
    }
    else return;

    mcu->m_modules.emplace_back( m_spi );

    setConfigRegs( e, m_spi );

    if( e->hasAttribute("dataregs") )
    {
        QString dataReg = e->attribute("dataregs");
        m_spi->m_dataReg = mcu->getReg( dataReg );
        watchRegNames( dataReg, R_WRITE, m_spi, &McuSpi::writeSpiReg, mcu );
    }
    if( e->hasAttribute("statusreg") )
    {
        QString statReg = e->attribute("statusreg");
        m_spi->m_statReg = mcu->getReg( statReg );
        watchRegNames( statReg, R_WRITE, m_spi, &McuSpi::writeStatus, mcu );
    }
    setInterrupt(  e->attribute("interrupt") , m_spi );
    setPrescalers( e->attribute("prescalers"), m_spi );

    QStringList pins = e->attribute("pins").remove(" ").split(",");
    m_spi->setMosiPin( mcu->getIoPin( pins.value(0) ) );
    m_spi->setMisoPin( mcu->getIoPin( pins.value(1) ) );
    m_spi->setSckPin(  mcu->getIoPin( pins.value(2) ) );
    m_spi->setSsPin(   mcu->getIoPin( pins.value(3) ) );
}

void McuCreator::createTcp( QDomElement* e )
{
    QString name = e->attribute( "name" );

    if( m_core == "scripted"){
        ScriptTcp* su = new ScriptTcp( mcu, name );
        m_scriptPerif.push_back( su );
        /// mcu->m_modules.emplace_back( su );
    }
    else return;
}

void McuCreator::createUsi( QDomElement* e )
{
    QString name = e->attribute( "name" );
    AvrUsi* usi = nullptr;
    if( m_core == "AVR" ) usi = new AvrUsi( mcu, name );
    else return;

    mcu->m_modules.emplace_back( usi );

    setConfigRegs( e, usi );

    /*    if(  e->hasAttribute("dataregs") )
    {
        QString dataReg = e->attribute("dataregs");
        usi->m_dataReg = mcu->getReg( dataReg );
        watchRegNames( dataReg, R_WRITE, usi, &AvrUsi::writeSpiReg, mcu );
    }
    if(  e->hasAttribute("statusreg") )
    {
        QString statReg = e->attribute("statusreg");
        usi->m_statusReg = mcu->getReg( statReg );
        watchRegNames( statReg, R_WRITE, usi, &AvrUsi::writeStatus, mcu );
    }*/

    setInterrupt( e->attribute("interrupt") , usi );
    usi->m_startInte = mcu->m_interrupts.m_intList.value( "USISTA" );

    usi->setPins( e->attribute("pins").remove(" ") );
}

void McuCreator::createWdt( QDomElement* e )
{
    QString name = e->attribute( "name" );
    int type = e->attribute("type").toInt();
    McuWdt* wdt = nullptr;
    if     ( m_core == "AVR" )   wdt = AvrWdt::createWdt( mcu, name, type );
    else if( m_core == "Pic12" ) wdt = PicWdt::createWdt( mcu, name, type );
    else if( m_core == "Pic14" ) wdt = PicWdt::createWdt( mcu, name, type );
    else if( m_core == "Pic14e") wdt = PicWdt::createWdt( mcu, name, type );

    if( !wdt ) return;

    mcu->m_modules.emplace_back( wdt );
    mcu->m_wdt = wdt;

    setConfigRegs( e, wdt );
    setInterrupt( e->attribute("interrupt"), wdt );
    setPrescalers( e->attribute("prescalers"), wdt );
}

void McuCreator::createSleep( QDomElement* e )
{
    QString name = e->attribute( "name" );
    McuSleep* sleep;
    if     ( m_core == "AVR"  ) sleep = new AvrSleep( mcu, name );
    else if( m_core == "Pic14") sleep = new PicSleep( mcu, name );
    else return;

    mcu->m_modules.emplace_back( sleep );
    mcu->m_sleepModule = sleep;

    setConfigRegs( e, sleep );
}

void McuCreator::createDisplay( QDomElement* e )
{
    QString name = e->attribute("name");

    int width  = e->attribute("width").toInt();
    int height = e->attribute("height").toInt();

    MyDisplay* display = nullptr;
    if( m_core == "scripted" )
    {
        ScriptDisplay* d = new ScriptDisplay( width, height, name, nullptr );
        m_scriptPerif.push_back( d );
        display = d;
    }
    else display = new MyDisplay( width, height, name, nullptr );

    if( e->hasAttribute("scale") ){
        double scale = e->attribute("scale").toDouble();
        display->setMonitorScale( scale );
    }
    if( e->hasAttribute("monitorscale") ){
        double scale = e->attribute("monitorscale").toDouble();
        display->setMonitorScale( scale );
    }
    if( e->hasAttribute("margins") ) m_mcuComp->setMargins( e->attribute("margins") );

    bool embed = e->attribute("embeed") == "true";
    display->setEmbed( embed );
    if( embed ) m_mcuComp->setBackData( display->getBackData() ); // Display in Package
    else        m_displays.append( display );                     // Display in Mcu Monitor
}

void McuCreator::createStack( QDomElement* s )
{
    QStringList spRegs = s->attribute("spreg").split(",");
    mcu->m_cpu->m_spl = mcu->getReg( spRegs.value(0) );
    if( spRegs.size() > 1 )
        mcu->m_cpu->m_sph = mcu->getReg( spRegs.value(1) );

    QString inc = s->attribute("increment");

    mcu->m_cpu->m_spPre = inc.contains("pre");
    mcu->m_cpu->m_spInc = inc.contains("inc") ? 1:-1;
}

void McuCreator::createInterrupts( QDomElement* i )
{
    QString enable = i->attribute("enable");
    if( !enable.isEmpty() )
    {
        mcu->m_interrupts.m_enGlobalFlag = getRegBits( enable, mcu );
        watchBitNames( enable, R_WRITE, mcu, &eMcu::enableInterrupts, mcu );
    }
    QDomNode node = i->firstChild();
    while( !node.isNull() )
    {
        QDomElement el = node.toElement();
        if( el.tagName() == "interrupt" ) createInterrupt( &el );
        node = node.nextSibling();
}   }

void McuCreator::createInterrupt( QDomElement* el )
{
    QString  intName = el->attribute("name");
    if( intName.isEmpty() ) return;

    uint16_t intVector  = el->attribute("vector").toUInt(0,0);

    Interrupt* iv = nullptr;
    //if     ( m_core == "8051" )  iv = I51Interrupt::getInterrupt( intName, intVector, mcu );
    //else if( m_core == "AVR" )   iv = AVRInterrupt::getInterrupt( intName, intVector, mcu );
    if     ( m_core == "Pic14" ) iv = new PicInterrupt( intName, intVector, mcu );
    else if( m_core == "Pic14e") iv = new Pic14eInterrupt( intName, intVector, mcu );
    else                         iv = new Interrupt( intName, intVector, mcu );
    if( !iv ) return;

    mcu->m_interrupts.m_intList.insert( intName, iv );
    iv->m_interrupts = &(mcu->m_interrupts);

    QString enable = el->attribute("enable");
    if( !enable.isEmpty() ) watchBitNames( enable, R_WRITE, iv, &Interrupt::enableFlag, mcu );

    QString intFlag = el->attribute("flag");
    iv->m_flagMask = mcu->m_bitMasks.value( intFlag );
    iv->m_flagReg  = mcu->m_bitRegs.value( intFlag );

    QString intPrio = el->attribute("priority");
    bool ok = false;
    uint8_t prio = intPrio.toUInt(&ok,0);

    if( ok ) iv->setPriority( prio );
    else     watchBitNames( intPrio, R_WRITE, iv, &Interrupt::setPriority, mcu );

    if( m_core != "AVR" ) /// TODO find a generic way
    {
        watchBitNames( intFlag, R_WRITE, iv, &Interrupt::flagCleared, mcu );
    }
    if( el->hasAttribute("clear") ) // If clear="1" Clear flag by writting 1 to it
    {
        uint8_t val = el->attribute("clear").toUInt();
        if( val ) watchRegister( iv->m_flagReg, R_WRITE, iv, &Interrupt::writeFlag, mcu );
    }
    if( el->hasAttribute("autoclear") ) // If autoclear="0" deactivate autoclear
    {
        uint8_t val = el->attribute("autoclear").toUInt();
        if( val ) iv->m_autoClear = true;
        else      iv->m_autoClear = false;
    }
    if( el->hasAttribute("pin") )
    {
        iv->m_intPin = mcu->getIoPin( el->attribute("pin") );
    }
    if( el->hasAttribute("wakeup") )
    {
        iv->m_wakeup = el->attribute("wakeup").toUInt( 0, 2 );
}   }

void McuCreator::setInterrupt( QString intName, McuModule* module )
{
    if( intName.isEmpty() ) return;
    Interrupt* inte = mcu->m_interrupts.m_intList.value( intName );
    if( !inte ) qDebug() << "    Error: Interrupt not found: " << intName << Qt::endl;
    module->setInterrupt( inte );
}

void McuCreator::setConfigRegs( QDomElement* u, McuModule* module )
{
    if( u->hasAttribute("configregsA") )
    {
        QString regs = u->attribute("configregsA");
        watchRegNames( regs, R_WRITE, module, &McuModule::configureA, mcu );
    }
    if( u->hasAttribute("configregsB") )
    {
        QString regs = u->attribute("configregsB");
        watchRegNames( regs, R_WRITE, module, &McuModule::configureB, mcu );
    }
    if( u->hasAttribute("configregsC") )
    {
        QString regs = u->attribute("configregsC");
        watchRegNames( regs, R_WRITE, module, &McuModule::configureC, mcu );
    }
    if( u->hasAttribute("configbitsA") )
    {
        QString configBits = u->attribute("configbitsA");
        watchBitNames( configBits, R_WRITE, module, &McuModule::configureA, mcu );
        module->m_configBitsA = getRegBits( configBits, mcu );
    }
    if( u->hasAttribute("configbitsB") )
    {
        QString configBits = u->attribute("configbitsB");
        watchBitNames( configBits, R_WRITE, module, &McuModule::configureB, mcu );
        module->m_configBitsB = getRegBits( configBits, mcu );
    }
    if( u->hasAttribute("configbitsC") )
    {
        QString configBits = u->attribute("configbitsC");
        watchBitNames( configBits, R_WRITE, module, &McuModule::configureC, mcu );
        module->m_configBitsC = getRegBits( configBits, mcu );
}   }

void McuCreator::setPrescalers( QString pr, McuPrescaled* module )
{
    if( pr.isEmpty() ) return;
    QStringList prescalers = pr.remove(" ").split(",");
    module->m_prescList.resize( prescalers.size() );

    for( int i=0; i<prescalers.size(); ++i )
    {
        QString prStr = prescalers.value(i);
        uint prVal;
        if     ( prStr == "EXT_F" ) prVal = 0x8000; // Ext clock, falling edge
        else if( prStr == "EXT_R" ) prVal = 0x8001; // Ext clock, rising edge
        else                        prVal = prStr.toUInt();
        module->m_prescList[i] = prVal;
    }
}
