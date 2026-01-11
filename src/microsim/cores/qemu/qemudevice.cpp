/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <qtconcurrentrun.h>
#include <QLibrary>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSignalMapper>
#include <QDir>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/mman.h>
#include <sys/shm.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include "qemudevice.h"
#include "itemlibrary.h"
#include "qemuusart.h"
#include "circuitwidget.h"
#include "iopin.h"

#include "stm32.h"
#include "esp32.h"

#include "circuit.h"
#include "simulator.h"
#include "componentlist.h"
#include "utils.h"

#include "stringprop.h"

#define tr(str) simulideTr("QemuDevice",str)

QemuDevice* QemuDevice::m_pSelf = nullptr;

Component* QemuDevice::construct( QString type, QString id )
{
    //if( QemuDevice::self() )
    //{
    //    qDebug() << "\nQemuDevice::construct ERROR: only one QemuDevice allowed\n";
    //    return nullptr;
    //}
    QString device = Chip::getDevice( id );

    QemuDevice* qdev = nullptr;

    if     ( device.startsWith("STM32") ) qdev = new Stm32( type, id, device );
    else if( device.startsWith("Esp32") ) qdev = new Esp32( type, id, device );
    return qdev;
}

LibraryItem* QemuDevice::libraryItem()
{
    return new LibraryItem(
        "QemuDevice",
        "",
        "ic2.png",
        "QemuDevice",
        QemuDevice::construct );
}

QemuDevice::QemuDevice( QString type, QString id )
          : Chip( type, id )
{
    m_pSelf = this;
    m_rstPin = nullptr;

    //m_fullSynch = false; //true;

    uint64_t pid = QCoreApplication::applicationPid();
    m_shMemKey = QString::number( pid )+id;
    void* arena = nullptr;

    const int shMemSize = sizeof( qemuArena_t );

    // create the shared memory object
#ifdef __linux__
    const char* charMemKey = m_shMemKey.toLocal8Bit().data();
    m_shMemId = shm_open( charMemKey, O_CREAT | O_RDWR, 0666);
    if( m_shMemId != -1 )
    {
        int t = ftruncate( m_shMemId, shMemSize );
        arena = mmap( 0, shMemSize, PROT_WRITE, MAP_SHARED, m_shMemId, 0 );
    }
#elif defined(_WIN32)
    const wchar_t* charMemKey = m_shMemKey.toStdWString().c_str();
    // Create a file mapping object
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE, // Use paging file
        NULL,                 // Default security
        PAGE_READWRITE,       // Read/write access
        0,                    // Maximum object size (high-order DWORD)
        shMemSize,            // Maximum object size (low-order DWORD)
        charMemKey );         // Name of the mapping object

    if( hMapFile != NULL )
        arena = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, shMemSize );

    m_wHandle = hMapFile;
#endif
    if( arena )
    {
        m_arena = (qemuArena_t*)arena;
        qDebug() << "QemuDevice::QemuDevice Shared Mem created" << shMemSize << "bytes";
    }else{
        m_arena = nullptr;
        m_shMemId = -1;
        qDebug() << "QemuDevice::QemuDevice Error creating arena";
    }

    m_qemuProcess.setProcessChannelMode( /*QProcess::MergedChannels*/ QProcess::ForwardedChannels ); // Merge stdout and stderr

    //Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"),{
        new StrProp<QemuDevice>("Program", tr("Firmware"),""
                               , this, &QemuDevice::firmware, &QemuDevice::setFirmware ),

        new StrProp<QemuDevice>("Args", tr("Extra arguments"),""
                               , this, &QemuDevice::extraArgs, &QemuDevice::setExtraArgs )
    }, 0 } );
}
QemuDevice::~QemuDevice()
{
    initialize();
#ifdef __linux__
    if( m_shMemId != -1 ) shm_unlink( m_shMemKey.toLocal8Bit().data() );
#elif defined(_WIN32)
    if( m_wHandle != NULL )
    {
        UnmapViewOfFile( (LPVOID)m_arena );
        CloseHandle( (HANDLE)m_wHandle );
    }
#endif
    m_pSelf = nullptr;
}

void QemuDevice::initialize()
{
    if( m_shMemId == -1 ) return;

    m_arena->running = 0;

    m_qemuProcess.waitForFinished( 500 );
    if( m_qemuProcess.state() != QProcess::NotRunning )
    {
        m_qemuProcess.kill();
        qDebug() << "QemuDevice: Qemu proccess killed";
    }
    else qDebug() << "QemuDevice: Qemu proccess finished";
    //updateStep();

    for( QemuModule* module : m_modules ) module->reset();
}

void QemuDevice::stamp()
{
    if( m_shMemId == -1 ) return;

    m_eventModule = nullptr;

    //m_lastTime = 0;
    m_arena->simuTime = 0;
    m_arena->qemuTime = 0;
    m_arena->regData = 0;
    m_arena->regAddr = 0;
    m_arena->irqNumber = 0;
    m_arena->irqLevel  = 0;
    m_arena->simuAction = 0;
    m_arena->qemuAction = 0;
    m_arena->loop_timeout_ns = 0;
    m_arena->running = 0;
    m_arena->ps_per_inst = 0;

    for( IoPin* pin : m_ioPin ) // Qemu calls us to read input
    {
        if( !pin ) continue;
        pin->setOutState( false );
        pin->setPinMode( input );
    }
    if( m_rstPin ) m_rstPin->changeCallBack( this );

    if( createArgs() )
    {
        QString executable = m_executable;
#ifdef _WIN32
        executable += ".exe";
#endif
        if( !QFileInfo::exists( executable ) )
        {
            qDebug() << "Error: QemuDevice::stamp executable does not exist:" << Qt::endl << executable;
        }
        m_qemuProcess.start( executable, m_arguments );

        uint64_t timeout = 0;
        while( !m_arena->running )   // Wait for Qemu running
        {
            //sleep( 10 );
            if( timeout++ > 2e9 ) // Don't wait forever
            {
                qDebug() << "Error: QemuDevice::stamp timeout";

                m_qemuProcess.waitForFinished( 500 );

                if( m_qemuProcess.exitStatus() != QProcess::NormalExit )
                {
                    QString output = m_qemuProcess.readAllStandardError();
                    if( !output.isEmpty() )
                    {
                        QStringList lines = output.split("\n");
                        for( QString line : lines ) qDebug() << line.remove("\"");
                    }

                    qDebug() << m_qemuProcess.exitStatus();
                    qDebug() << m_qemuProcess.error();
                    qDebug() << m_qemuProcess.exitCode();
                    qDebug() << m_qemuProcess.state();
                }
//                    m_qemuProcess.kill();
                return;
            }
        }
        qDebug() << "QemuDevice::stamp started";

        Simulator::self()->addEvent( 1, this );
    }
}

//void QemuDevice::updateStep()
//{
//    return;
//
//    QString output = m_qemuProcess.readAllStandardOutput();
//    if( !output.isEmpty() )
//    {
//        QStringList lines = output.split("\n");
//        for( QString line : lines ) qDebug() << line.remove("\"");
//    }
//}

void QemuDevice::voltChanged()
{
    if( m_shMemId == -1 ) return;

    bool reset = !m_rstPin->getInpState();

    if( reset )
    {
        m_arena->running = 0;

        if( m_qemuProcess.state() > QProcess::NotRunning )
        {
            Simulator::self()->cancelEvents( this );
            m_qemuProcess.kill();
        }
    }
    else if( m_qemuProcess.state() == QProcess::NotRunning ) stamp();
}

void QemuDevice::runToTime( uint64_t time )
{
    /// if( this->eventTime ) // Our event still not executed
    /// {
    ///     //qDebug() << "QemuDevice::runToTime ERROR: event not executed"<<Simulator::self()->circTime();
    ///     return;
    /// }

    /// //if( !m_fullSynch )
    /// //{
    /// //    uint64_t now = Simulator::self()->circTime();
    /// //    if( m_firstEvent )
    /// //    {
    /// //        m_arena->qemuEvent = 1;
    /// //        time = m_firstEvent->eventTime;
    /// //        //qDebug() << "\nQemuDevice::runToTime qemuEvent"<<now<< time;
    /// //    }
    /// //    else time = now + 1000000000; // 1 ms
    /// //}
    /// //if( m_lastTime != time )
    /// //{
    /// //    m_lastTime = time;
    /// //    //qDebug() << "\nQemuDevice::runToTime"<< time;

    /// //    //m_arena->simuTime = 0;
    /// //    m_arena->qemuTime = time; // Tell Qemu to run up to time
    /// //}

    /// while( m_arena->simuTime == 0 ) // Wait for Qemu action
    /// {
    ///     if( Simulator::self()->simState() != SIM_RUNNING )
    ///     {
    ///         //qDebug() << "QemuDevice::runToTime BREAK --------------------------";
    ///         return;
    ///     }
    /// }
    /// if( m_arena->simuTime <= Simulator::self()->circTime()){
    ///     qDebug() << "QemuDevice::runToTime ERROR"<< m_arena->simuTime << Simulator::self()->circTime();
    ///     //m_arena->simuTime += 1;
    ///     return;
    /// }

    /// Simulator::self()->addEventAt( m_arena->simuTime, this );
    /// //if( m_firstEvent )
    ///     //qDebug() << "QemuDevice::runToTime event"<< m_arena->simuTime;
}

void QemuDevice::runEvent()
{
    uint64_t now = Simulator::self()->circTime();
    //qDebug() << "   QemuDevice::runEvent"<< now;

    if( m_eventModule )
    {
        m_eventModule->runAction();
        m_eventModule->m_eventAction = 0;
        m_eventModule = nullptr;
    }


    m_arena->simuTime = 0;
    while( !m_arena->simuTime )   // Wait for next event from Qemu
    {
        if( Simulator::self()->simState() < SIM_RUNNING ) //
        {
            Simulator::self()->addEvent( 1, this );
            return;
        }
    }
    uint64_t nextTime = m_arena->simuTime;

    if( nextTime <= now ){
        qDebug() << "    QemuDevice::runEvent ERROR"<< nextTime << now;

        return;
    }

    if( m_arena->simuAction )
    {
        if     ( m_arena->simuAction == SIM_FREQ  ) updtFrequency();
        else if( m_arena->simuAction != SIM_EVENT ) doAction();
        m_arena->simuAction = 0;
    }
    else qDebug() << "QemuDevice::runEvent No action";

    //qDebug() << "QemuDevice::runEvent Next"<< nextQemutime;

    Simulator::self()->addEventAt( nextTime, this );
    //m_lastEvent = m_nextEvent;
}

void QemuDevice::doAction()
{
    uint32_t address = m_arena->regAddr;
    //qDebug() << "   QemuDevice::doAction"<< QString::number( address, 16 ); //Simulator::self()->circTime();

    for( QemuModule* module : m_modules )
    {
        if( address < module->m_memStart
         || address > module->m_memEnd ) continue;
//qDebug() << "   QemuDevice::doAction"<< module->m_name;
        m_eventModule = module;
        module->m_eventAddress = address;
        module->m_eventValue   = m_arena->regData;
        module->m_eventAction  = m_arena->simuAction;
        break;
    }
}

//void QemuDevice::addEvent( uint64_t time, QemuModule* el )
//{
//    //if( m_state < SIM_STARTING ) return;
//
//    if( el->eventTime ){
//        //m_warning = 200; //
//        qDebug() << "Warning: QemuDevice::addEvent Repeated event"<<el->eventTime;
//        return;
//    }
//
//    time += Simulator::self()->circTime();
//    QemuModule* last  = nullptr;
//    QemuModule* event = m_firstEvent;
//
//    while( event )
//    {
//        if( time <= event->eventTime ) break; // Insert event here
//        last  = event;
//        event = event->nextEvent;
//    }
//    el->eventTime = time;
//
//    if( last ) last->nextEvent = el;
//    else       m_firstEvent = el; // List was empty or insert First
//
//    el->nextEvent = event;
//}
//
//void QemuDevice::cancelEvents( QemuModule* el )
//{
//    if( el->eventTime == 0 ) return;
//    QemuModule* event = m_firstEvent;
//    QemuModule* last  = nullptr;
//    QemuModule* next  = nullptr;
//    el->eventTime = 0;
//
//    while( event ){
//        next = event->nextEvent;
//        if( el == event )
//        {
//            if( last ) last->nextEvent = next;
//            else       m_firstEvent = next;
//            event->nextEvent = nullptr;
//        }
//        else last = event;
//        event = next;
//    }
//}

void QemuDevice::slotOpenTerm( int num )
{
    m_usarts.at(num-1)->openMonitor( findIdLabel(), num );
    //m_serialMon = num;
}

void QemuDevice::slotLoad()
{
    QDir dir( m_lastFirmDir );
    if( !dir.exists() ) m_lastFirmDir = Circuit::self()->getFilePath();

    QString fileName = QFileDialog::getOpenFileName( nullptr, tr("Load Firmware"), m_lastFirmDir,
                                                    tr("All files (*.*);;Hex Files (*.hex)"));

    if( fileName.isEmpty() ) return; // User cancels loading

    setFirmware( fileName );
}

void QemuDevice::slotReload()
{
    if( !m_firmware.isEmpty() ) setFirmware( m_firmware );
    else QMessageBox::warning( 0, "QemuDevice::slotReload", tr("No File to reload ") );
}

void QemuDevice::setFirmware( QString file )
{
    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    //QDir    circuitDir   = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    //QString fileNameAbs  = circuitDir.absoluteFilePath( file );
    //QString cleanPathAbs = circuitDir.cleanPath( fileNameAbs );
    m_firmware = file;//cleanPathAbs;
}

void QemuDevice::setPackageFile( QString package )
{
    if( !QFile::exists( package ) )
    {
        qDebug() <<"File does not exist:"<< package;
        return;
    }
    QString pkgText = fileToString( package, "QemuDevice::setPackageFile");
    QString pkgStr  = convertPackage( pkgText );
    m_isLS = package.endsWith("_LS.package");
    initPackage( pkgStr );
    setLogicSymbol( m_isLS );

    m_label.setPlainText( m_name );

    Circuit::self()->update();
}

void QemuDevice::contextMenu(QGraphicsSceneContextMenuEvent* event, QMenu* menu)
{
    // Firmware actions
    if (true) // if( m_eMcu.flashSize() )
    {
        QAction* loadAction = menu->addAction(QIcon(":/load.svg"), tr("Load firmware"));
        QObject::connect(loadAction, &QAction::triggered, [=]() { slotLoad(); });

        QAction* reloadAction = menu->addAction(QIcon(":/reload.svg"), tr("Reload firmware"));
        QObject::connect(reloadAction, &QAction::triggered, [=]() { slotReload(); });

        menu->addSeparator();
    }

    // Serial Monitor menu
    if (!m_usarts.empty())
    {
        QMenu* serMonMenu = menu->addMenu(QIcon(":/serialterm.png"), tr("Open Serial Monitor."));

        for (uint i = 0; i < m_usarts.size(); ++i)
        {
            QAction* openSerMonAct = serMonMenu->addAction("USart" + QString::number(i + 1));
            // Direct lambda capture of i+1, replacing QSignalMapper
            QObject::connect(openSerMonAct, &QAction::triggered, [this, i]() {
                slotOpenTerm(i + 1);
            });
        }
    }

    menu->addSeparator();
    Component::contextMenu(event, menu);
}
