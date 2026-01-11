/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QDomDocument>
#include <QFileInfo>
#include <QTranslator>
#include <QSignalMapper>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QObject>
#include <QtGlobal>       // sometimes helps
// → THIS ONE is the missing one in Qt 6:
#include <QMetaObject>
//#include <QObjectConnection>

#include "mcu.h"
#include "cpu8bits.h"
#include "mcuport.h"
#include "mcupin.h"
#include "mcuwdt.h"
#include "mcucreator.h"
#include "scriptcpu.h"
#include "circuit.h"
#include "simulator.h"
#include "itemlibrary.h"
#include "circuitwidget.h"
#include "infowidget.h"
#include "mainwindow.h"
#include "componentlist.h"
#include "mcumonitor.h"
#include "memdata.h"
#include "mcuuart.h"
#include "mcuintosc.h"
#include "utils.h"
#include "watcher.h"

#include "stringprop.h"
#include "doubleprop.h"
#include "intprop.h"
#include "boolprop.h"

#include "propdialog.h"

#define tr(str) simulideTr("Mcu",str)

Mcu* Mcu::m_pSelf = nullptr;

LibraryItem* Mcu::libraryItem()
{
    return new LibraryItem(
        "NEW_MCU",
        "",
        "ic2.png",
        "MCU",
        Mcu::construct );
}

Component* Mcu::construct( QString type, QString id )
{
    m_error = 0;
    QString device = Chip::getDevice( id );
    Mcu* mcu = new Mcu( type, id, device );
    if( !m_error) m_error = McuCreator::createMcu( mcu, id );

    if( m_error > 0 )
    {
        Circuit::self()->removeComp( mcu );
        mcu = nullptr;
        m_pSelf = nullptr;
        m_error = 0;
    }
    return mcu;
}

Mcu::Mcu( QString type, QString id, QString device )
   : Chip( type, id, device )
   , m_eMcu( this, id )
{
    qDebug() << "        Initializing"<<id;

    m_resetPin   = nullptr;
    m_portRstPin = nullptr;
    m_mcuMonitor = nullptr;
    m_scriptLink = nullptr;

    m_savePGM  = false;
    m_autoLoad = false;
    m_scripted = false;
    m_resetPol = false;
    m_isLinker = true;
    m_forceFreq = true;

    m_uiFreq = 0;
    m_serialMon = -1;

    addPropGroup( { tr("Main"), {},0} );

    m_device = m_name;
    if( m_device.contains("@") ) m_device = m_device.split("@").last(); // MCU in Subcircuit

    if( m_device.startsWith("p") ) // PICs TODELETE
    {
        if( m_device.endsWith("a") ) m_device.remove( m_device.size()-1, 1 );
        m_device.replace("f", "F");
    }

    setName( m_device );

     // Search in Circuit folder
    QDir mcuDir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    QString baseFile = mcuDir.absoluteFilePath( m_device+"/"+m_device );
    QString mcuFile = baseFile+".mcu";
    if( !QFile::exists( mcuFile ) )
    {
        baseFile = mcuDir.absoluteFilePath( "data/"+m_device+"/"+m_device );
        mcuFile = baseFile+".mcu";
    }

    if( !QFile::exists( mcuFile ) ) // Not found in Circuit folder, check if Defined in xml file
    {
        baseFile = "";
        QString xmlFile = ComponentList::self()->getDataFile( m_device );

        if( QFile::exists( xmlFile ) )
        {
            QDomDocument domDoc = fileToDomDoc( xmlFile, "Mcu::Mcu" );
            if( domDoc.isNull() ) { m_error = 1; return; }

            QDomElement root  = domDoc.documentElement();
            QDomNode    rNode = root.firstChild();

            bool found = false;
            while( !rNode.isNull() )
            {
                QDomElement itemSet = rNode.toElement();
                QDomNode    node    = itemSet.firstChild();

                QString folder = "";
                if( itemSet.hasAttribute("folder") ) folder = itemSet.attribute("folder")+"/";

                while( !node.isNull() )
                {
                    QDomElement element = node.toElement();
                    if( element.attribute("name") == m_device )
                    {
                        QFileInfo fi( xmlFile );

                        QString pkg = m_device+"/"+m_device;
                        QString dat = pkg;

                        if( element.hasAttribute("package") ) pkg = element.attribute("package");
                        if( element.hasAttribute("data")    ) dat = element.attribute("data");

                        baseFile = fi.absolutePath()+"/"+ folder + pkg;
                        mcuFile  = fi.absolutePath()+"/"+ folder + dat+".mcu";

                        found = true;
                    }
                    if( found ) break;
                    node = node.nextSibling();
                }
                if( found ) break;
                rNode = rNode.nextSibling();
            }
        }
    }
    if( !QFile::exists( mcuFile ) ){
        QDir mcuDir;
        QString folder = ComponentList::self()->getFileDir( m_device );

        if( !folder.isEmpty() ) // Found in folder (no xml file)
        {
            mcuDir = QDir( folder );
        }
        else              // Try to find a "data" folder in Circuit folder
        {
            mcuDir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
            folder = "data/"+m_device;
        }
        mcuFile = mcuDir.absoluteFilePath( folder+"/"+m_device+".mcu" );
        baseFile = mcuDir.absoluteFilePath( folder+"/"+m_device );
    }

    if( !baseFile.isEmpty() )
    {
        m_dataFile = mcuFile; //+".mcu";
        if( !QFileInfo::exists( m_dataFile ) )
        {
            qDebug() <<"Mcu::Mcu Files not found for:"<< m_device;
            qDebug() << m_dataFile;
            m_error = 1;
            return;
        }

        QString pkgeFile = baseFile+".package";
        if( QFileInfo::exists( pkgeFile ) ){
            QString pkgStr = fileToString( pkgeFile, "Mcu::Mcu" );
            m_packageList["1- "+m_device+"_DIP"] = convertPackage( pkgStr );
        }
        QString pkgFileLS = baseFile+"_LS.package";
        if( QFileInfo::exists( pkgFileLS ) ){
            QString pkgStr = fileToString( pkgFileLS, "Mcu::Mcu" );
            m_packageList["2- "+m_device+"_LS"] = convertPackage( pkgStr );
        }
    }

    if( m_packageList.isEmpty() ){
        qDebug() << "Mcu::Mcu: No Packages found for"<<m_device<<Qt::endl;
        m_error = 1;
        return;
    }

    QSettings* settings = MainWindow::self()->settings();
    m_lastFirmDir = settings->value("lastFirmDir").toString();

    QDir dir( m_lastFirmDir );
    if( !dir.exists() ) m_lastFirmDir = Circuit::self()->getFilePath();

    Simulator::self()->addToUpdateList( this );

    qDebug() << "       "<<id<< "Initialized"<<Qt::endl;
}
Mcu::~Mcu()
{
    if( m_mcuMonitor ) delete m_mcuMonitor;
    if( m_pSelf == this ) m_pSelf = nullptr;
    InfoWidget::self()->updtMcu();
}

void Mcu::setupMcu()
{
    m_eMcu.setup();

    if( m_pSelf == nullptr ) slotmain();

    // Main Property Group --------------------------------------

    if( m_packageList.size() > 1 )
    addProperty(tr("Main"), new StrProp<Mcu>("Package", tr("Package"), m_packageList.keys().join(",")
                                            , this, &Mcu::package, &Mcu::setPackage, propNoCopy,"enum" ) );

    if( m_eMcu.m_intOsc )
    {
    addProperty(tr("Main"),new DoubProp<Mcu>("Frequency", tr("Frequency"),"MHz"
                                            , this, &Mcu::uiFreq, &Mcu::setUiFreq ));
    addProperty(tr("Main"),new BoolProp<Mcu>("ForceFreq", tr("Force this frequency"),""
                                            , this, &Mcu::forceFreq, &Mcu::setForceFreq ));
    }

    if( m_eMcu.flashSize() )
    {
    addProperty(tr("Main"),new BoolProp<Mcu>("savePGM", tr("PGM persistent"),""
                                            , this, &Mcu::savePGM, &Mcu::setSavePGM ));

    addProperty(tr("Main"),new StrProp <Mcu>("Program", tr("Firmware"),""
                                            , this, &Mcu::program, &Mcu::setProgram ));

    addProperty(tr("Main"),new BoolProp<Mcu>("Auto_Load", tr("Reload hex at Simulation Start"),""
                                            , this, &Mcu::autoLoad, &Mcu::setAutoLoad ));
    }
    if( m_eMcu.romSize() )
    addProperty(tr("Main"),new BoolProp<Mcu>("saveEepr", tr("EEPROM persistent"),""
                                            , this, &Mcu::saveEepr, &Mcu::setSaveEepr ));

    // Config Property Group ------------------------------------

    propGroup cg = { tr("Config"), {
        new ComProperty( "", tr("Changes applied after Simulation Restart"),"","",0),}, groupNoCopy};

    if( m_portRstPin )
        cg.propList.append(new BoolProp<Mcu>("Rst_enabled", tr("Enable Reset Pin"),""
                                            , this, &Mcu::rstPinEnabled, &Mcu::enableRstPin ) );

    if( m_eMcu.m_intOsc && m_eMcu.m_intOsc->hasClockPins() )
        cg.propList.append(new BoolProp<Mcu>("Ext_Osc", tr("External Oscillator"),""
                                            , this, &Mcu::extOscEnabled, &Mcu::enableExtOsc ) );

    if( m_eMcu.m_wdt )
        cg.propList.append(new BoolProp<Mcu>("Wdt_enabled", tr("Enable WatchDog"),""
                                            , this, &Mcu::wdtEnabled, &Mcu::enableWdt ) );

    if( m_eMcu.m_intOsc && m_eMcu.m_intOsc->clkOutPin() )
        cg.propList.append(new BoolProp<Mcu>("Clk_Out", tr("Clock Out"),""
                                            , this, &Mcu::clockOut, &Mcu::setClockOut ) );

    if( cg.propList.size() > 1 ) addPropGroup( cg );


    // Hidden Property Group -----------------------------------

    propGroup hi = {"Hidden", {}, groupHidden }; // Set before Main

    hi.propList.append(new StrProp<Mcu>("varList" ,"","", this, &Mcu::varList,   &Mcu::setVarList) );
    hi.propList.append(new StrProp<Mcu>("cpuRegs" ,"","", this, &Mcu::cpuRegs,   &Mcu::setCpuRegs) );
    hi.propList.append(new StrProp<Mcu>("Links"   ,"","", this, &Mcu::getLinks , &Mcu::setLinks ) );
    hi.propList.append(new BoolProp<Mcu>("MainMcu","","", this, &Mcu::mainMcu , &Mcu::setMainMcu ) );

    if( m_eMcu.flashSize() )
    hi.propList.append(new StrProp<Mcu>("pgm"   ,"","", this, &Mcu::getPGM, &Mcu::setPGM ) );

    if( m_eMcu.romSize() )
    hi.propList.append(new StrProp<Mcu>("eeprom"   ,"","", this, &Mcu::getEeprom, &Mcu::setEeprom ) );

    if( m_eMcu.m_usarts.size() )
    hi.propList.append(new IntProp<Mcu>("SerialMon","","", this, &Mcu::serialMon, &Mcu::setSerialMon ) );

    if( hi.propList.size() > 0 ) addPropGroup( hi );

    //int index = m_isTQFP ? 2 : 0;
    setPackage( m_packageList.keys().at( 0 ) );

    m_eMcu.getRamTable()->setRegisters( m_eMcu.m_regInfo.keys() );
    setUiFreq( m_uiFreq );
}

bool Mcu::propNotFound( QString prop, QString val )
{
    if( prop =="program" ) setProgram( val ); //  Old: TODELETE
    else if( prop =="Mhz" ) setUiFreq( val.toDouble()*1e6 );
    else return Chip::propNotFound( prop, val );
    return true;
}

void Mcu::initialize()
{
    m_crashed = false;
    if( m_mcuMonitor ) m_mcuMonitor->updateRamTable();
}

void Mcu::stamp()
{
    m_eMcu.reset();

    if( m_resetPin ){
        m_resetPin->changeCallBack( this );

        if( m_resetPin == m_portRstPin ) // Not dedicated Reset Pin
        {
            m_portRstPin->controlPin( true, true );
            m_portRstPin->setPinMode( input );
        }
        bool resetWarning = !m_resetPin->isConnected();
        m_resetPin->warning( resetWarning );
        if( resetWarning ) qDebug() << "    Warning!!"<<idLabel()<<"Reset pin not connected:\n";
    }
    else m_eMcu.start();

    if( m_autoLoad )
    { if( !m_eMcu.m_firmware.isEmpty() ) load( m_eMcu.m_firmware ); }
}

void Mcu::updateStep()
{
    if( m_backData ) update();  // used by ScriptDisplay

    if( m_crashed )
    {
        Simulator::self()->setWarning( /*m_warning*/0 );
        update();
    }
    if( m_mcuMonitor
     && m_mcuMonitor->isVisible() ) m_mcuMonitor->updateStep();

    m_eMcu.m_cpu->updateStep();
}

void Mcu::voltChanged() // Reset Pin callBack
{
    if( !m_resetPin ) return;
    m_eMcu.hardReset( m_resetPin->getInpState() == m_resetPol );
}

void Mcu::setProgram( QString pro )
{
    if( m_savePGM ) return;
    if( pro == "" ) return;
    load( m_subcFolder+pro );
}

QString Mcu::varList()
{
    RamTable* ramTable = m_eMcu.getRamTable();
    if( ramTable ) return ramTable->getVarSet().join(",");
    return "";
}

void Mcu::setVarList( QString vl )
{
    RamTable* ramTable = m_eMcu.getRamTable();
    if( !ramTable ) return;

    if( vl.isEmpty() ) return;
    QStringList vars = vl.split(",");
    QStringList varSet;
    for( QString str : vars ) // Format: {"var0/aadr0/type0", "var1/addr1/type1",...}
    {
        QStringList words = str.split("/");
        if( words.size() < 1 ) continue;
        QString name = words.at(0);
        int addr = -1;
        QString type = "uint8";
        bool ok = false;
        if( words.size() > 1 ) addr = words.at(1).toInt( &ok, 0 );
        if( words.size() > 2 ) type = words.at(2);
        if( ok ) m_eMcu.getRamTable()->addVariable( name, addr, type );
        varSet.append( name );
    }
    ramTable->loadVarSet( varSet );
}
//{ m_eMcu.getRamTable()->loadVarSet( vl.split(",") ); }

QString Mcu::cpuRegs()
{
    Watcher* watcher = m_eMcu.cpu()->getWatcher();
    if( !watcher ) return "";
    return watcher->getVarSet().join(",");
}

void Mcu::setCpuRegs( QString vl )
{
    Watcher* watcher = m_eMcu.cpu()->getWatcher();
    if( !watcher ) return;
    m_eMcu.cpu()->getWatcher()->loadVarSet( vl.split(",") );
}

QString Mcu::getPGM()
{
    if( m_savePGM )
    {
        QString pgmStr;
        QVector<int> pgm;
        for( uint16_t val : m_eMcu.m_progMem ) pgmStr += QString::number( val )+",";

        return pgmStr;
    }
    return "";
}

void Mcu::setPGM( QString pgm )
{
    if( pgm.isEmpty() ) return;
    QStringList valList = pgm.split(",");
    int size = m_eMcu.flashSize();
    int i = 0;
    for( QString valStr : valList )
    {
        if(! valStr.isEmpty() ) m_eMcu.setFlashValue( i, valStr.toInt() );
        if( ++i >= size ) break;
    }
}

void Mcu::setEeprom( QString eep )
{
    if( eep.isEmpty() ) return;
    QVector<int> eeprom;
    QStringList list = eep.split(",");
    for( QString val : list ) eeprom.append( val.toUInt() );

    if( eeprom.size() > 0 ) m_eMcu.setEeprom( &eeprom );
}

QString Mcu::getEeprom()  // Used by property, stripped to last written value.
{
    QString eeprom;
    if( m_eMcu.m_saveEepr )
    {
        int size = m_eMcu.romSize();
        if( size > 0 )
        {
            bool empty = true;
            for( int i=size-1; i>=0; --i )
            {
                uint8_t val = m_eMcu.getRomValue( i );
                if( val < 0xFF ) empty = false;
                if( empty ) continue;
                eeprom.prepend( QString::number( val )+"," );
    }   }   }
    return eeprom;
}

void Mcu::loadEEPROM()
{
   QVector<int>* eeprom = m_eMcu.eeprom();
   MemData::loadData( eeprom, false );
   m_eMcu.setEeprom( eeprom );
   if( m_mcuMonitor ) m_mcuMonitor->tabChanged( 1 );
}

void Mcu::saveEEPROM() { MemData::saveData( m_eMcu.eeprom() ); }

void Mcu::slotLoad()
{
    QDir dir( m_lastFirmDir );
    if( !dir.exists() ) m_lastFirmDir = Circuit::self()->getFilePath();

    QString fileName = QFileDialog::getOpenFileName( nullptr, tr("Load Firmware"), m_lastFirmDir,
                       tr("All files (*.*);;Hex Files (*.hex)"));

    if( fileName.isEmpty() ) return; // User cancels loading

    load( fileName );
}

void Mcu::slotReload()
{
    if( !m_eMcu.m_firmware.isEmpty() ) load( m_eMcu.m_firmware );
    else QMessageBox::warning( 0, "Mcu::slotReload", tr("No File to reload ") );
}

bool Mcu::load( QString fileName )
{
    if( fileName.isEmpty() ) return false;

    QDir circuitDir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    QString fileNameAbs  = circuitDir.absoluteFilePath( fileName );
    QString cleanPathAbs = circuitDir.cleanPath( fileNameAbs );

    if( !QFileInfo::exists( cleanPathAbs ) )
    {
        qDebug() << "Error: file doesn't exist:\n"<<cleanPathAbs<<"\n";
        return false;
    }
    if( Simulator::self()->simState() > SIM_STARTING )  CircuitWidget::self()->powerCircOff();

    int size = m_eMcu.flashSize();
    QVector<int> pgm( size );
    for( int i=0; i<size; ++i ) pgm[i] = m_eMcu.getFlashValue( i );

    if( !MemData::loadFile( &pgm, cleanPathAbs, false, m_eMcu.m_wordSize*8, &m_eMcu ) )
        return false;

    for( int i=0; i<size; ++i ) m_eMcu.setFlashValue( i, pgm.at(i) );
    qDebug() << "Firmware successfully loaded\n";

    QString firmware = circuitDir.relativeFilePath( cleanPathAbs );
    if( m_eMcu.m_firmware != firmware ) Circuit::self()->setChanged();
    m_eMcu.m_firmware = firmware;

    m_lastFirmDir = QFileInfo( cleanPathAbs ).absolutePath();
    if( m_propDialog ) m_propDialog->updtValues();

    QSettings* settings = MainWindow::self()->settings();
    settings->setValue( "lastFirmDir", circuitDir.relativeFilePath( fileName ) );

    return true;
}

void Mcu::contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu )
{
    QAction* mainAction = menu->addAction( QIcon(":/subc.png"),tr("Main Mcu") );
    QObject::connect( mainAction, &QAction::triggered, [=](){ slotmain(); } );

    if( m_scriptLink && !parentItem() )
    {
        QAction* linkCompAction = menu->addAction( QIcon(":/subcl.png"),tr("Link to Component") );
        QObject::connect( linkCompAction, &QAction::triggered, [=](){ slotLinkComp(); } );
    }

    if( m_eMcu.flashSize() )
    {
        QAction* loadAction = menu->addAction( QIcon(":/load.svg"),tr("Load firmware") );
        QObject::connect( loadAction, &QAction::triggered, [=](){ slotLoad(); } );

        QAction* reloadAction = menu->addAction( QIcon(":/reload.svg"),tr("Reload firmware") );
        QObject::connect( reloadAction, &QAction::triggered, [=](){ slotReload(); } );

        menu->addSeparator();
    }

    if( m_eMcu.romSize() )
    {
        QAction* loadDaAction = menu->addAction( QIcon(":/open.svg"),tr("Load EEPROM data from file") );
        QObject::connect( loadDaAction, &QAction::triggered, [=](){ loadEEPROM(); } );

        QAction* saveDaAction = menu->addAction(QIcon(":/save.svg"), tr("Save EEPROM data to file") );
        QObject::connect( saveDaAction, &QAction::triggered, [=](){ saveEEPROM(); } );
    }
    menu->addSeparator();

    QAction* openRamTab = menu->addAction( QIcon(":/terminal.svg"),tr("Open Mcu Monitor.") );
    QObject::connect( openRamTab, &QAction::triggered, [=](){ slotOpenMcuMonitor(); } );

    if (!m_eMcu.m_usarts.empty())
    {
        QMenu* serMonMenu = menu->addMenu(QIcon(":/serialterm.png"),
                                          tr("Open Serial Monitor"));

        for (uint i = 0; i < m_eMcu.m_usarts.size(); ++i)
        {
            const int portNumber = i + 1;  // or i if you prefer 0-based
            QString text = "USART" + QString::number(portNumber);

            QAction* act = serMonMenu->addAction(text);

            // Direct connection with captured value – no QSignalMapper needed
            QObject::connect(act, &QAction::triggered,
                             this, [this, portNumber](bool /*checked*/) {
                                 slotOpenTerm(portNumber);
                             });
        }
    }
    menu->addSeparator();
    Component::contextMenu( event, menu );
}

void Mcu::slotmain()
{
    m_pSelf = this;
    m_eMcu.setMain();
    InfoWidget::self()->updtMcu();
    Circuit::self()->update();
}

void Mcu::slotLinkComp()
{
    Linker::startLinking();
}

void Mcu::setIdLabel( QString id )
{
    Component::setIdLabel( id );
    for( uint i=0; i<m_eMcu.m_usarts.size(); ++i )
    {
        QString id = findIdLabel()+" - Uart"+QString::number(i+1);
        m_eMcu.m_usarts.at(i)->setMonitorTittle( id );
    }
}

void Mcu::slotOpenMcuMonitor()
{
    if( !m_mcuMonitor ) m_mcuMonitor = new MCUMonitor( CircuitWidget::self(), &m_eMcu );
    m_mcuMonitor->setWindowTitle( findIdLabel() );
    m_mcuMonitor->show();
}

void Mcu::slotOpenTerm( int num )
{
    m_eMcu.m_usarts.at(num-1)->openMonitor( findIdLabel(), num );
    m_serialMon = num;
}

int Mcu::serialMon()
{
    if( m_serialMon < 0 ) return -1;
    if( m_eMcu.m_usarts.at( m_serialMon-1 )->serialMon() ) return m_serialMon;
    return -1;
}

void Mcu::setSerialMon( int s ) { if( s>=0 ) slotOpenTerm( s ); }

void Mcu::setLinkedValue( double v, int i )
{
    if( m_scriptLink ) m_scriptLink->setLinkedVal( v, i );
}

void Mcu::setLinkedString( QString str, int i )
{
    if( m_scriptLink ) m_scriptLink->setLinkedStr( str, i );
}

Pin* Mcu::addPin( QString id, QString type, QString label,
                  int pos, int xpos, int ypos, int angle, int length, int space )
{
    IoPin* pin = nullptr;
    if( type.contains("rst") )
    {
        if( !m_resetPin )
            m_resetPin = new IoPin( angle, QPoint(xpos, ypos), m_id+"-"+id, pos-1, this, input );
        pin = m_resetPin;
        m_resetPol = type.startsWith("!") ;
    }
    else pin = m_eMcu.getIoPin( id ); // I/O Port

    if( !pin ) return nullptr;

    QColor color = Qt::black;
    if( !m_isLS ) color = QColor( 250, 250, 200 );

    if( type.startsWith("nul") )
    {
        pin->setVisible( false );
        pin->setLabelText( "" );
    }else{
        pin->setVisible( true );
        if( type.startsWith("inv") ) pin->setInverted( true );
    }

    pin->setPos( QPoint( xpos, ypos ) );
    pin->setPinAngle( angle );
    pin->setLength( length );
    pin->setSpace( space );
    pin->setLabelText( label );
    pin->setLabelColor( color );
    pin->setFlag( QGraphicsItem::ItemStacksBehindParent, false );
    return pin;
}

void Mcu::setUiFreq( double freq )
{
    m_uiFreq = freq;
    if( m_forceFreq ) m_eMcu.forceFreq( freq );
    else if( !m_eMcu.intOsc()->freqChanged() ) m_eMcu.forceFreq( freq ); // McuIntOsc can reconfigure frequency, if not then set directly
}

void Mcu::setForceFreq( bool f )
{
    m_forceFreq = f;
    setUiFreq( m_uiFreq );
}

bool Mcu::rstPinEnabled()
{
    if( !m_portRstPin ) return true;
    return (m_resetPin == m_portRstPin);
}

void Mcu::enableRstPin( bool en ) // Called from Property or cfg word
{
    if( !m_portRstPin ) return;

    m_portRstPin->controlPin( en, en );

    if( en ){
        m_resetPin = m_portRstPin;
        m_portRstPin->setPinMode( input );
    }
    else m_resetPin = nullptr;
}

bool Mcu::extOscEnabled()
{
    if( m_eMcu.m_intOsc ) return m_eMcu.m_intOsc->extClock();
    return false;
}

void Mcu::enableExtOsc( bool en )
{
    if( m_eMcu.m_intOsc ) m_eMcu.m_intOsc->enableExtOsc( en );
}

bool Mcu::wdtEnabled()
{
    if( m_eMcu.m_wdt ) return m_eMcu.m_wdt->enabled();
    return false;
}

void Mcu::enableWdt( bool en ) { if( m_eMcu.m_wdt ) m_eMcu.m_wdt->enable( en ); }

bool Mcu::clockOut()
{
    if( m_eMcu.m_intOsc ) return m_eMcu.m_intOsc->clockOut();
    return false;
}

void Mcu::setClockOut( bool clkOut ) { if( m_eMcu.m_intOsc ) m_eMcu.m_intOsc->setClockOut( clkOut ); }

void Mcu::paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    Chip::paint( p, option, widget );

    if( m_pSelf == this )
    {
        QPen pen( Qt::black, 0.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p->setPen( pen );
        p->setBrush( Qt::yellow );
        if( m_width == m_height ) p->drawRoundedRect( 4, 4, 4, 4 , 2, 2);
        else                      p->drawRoundedRect( m_area.width()/2-2, -1, 4, 4 , 2, 2);
}   }

