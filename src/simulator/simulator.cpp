/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <qtconcurrentrun.h>
#include <QHash>
#include <math.h>

#include "simulator.h"
#include "editorwindow.h"
#include "circuit.h"
#include "updatable.h"
#include "outpaneltext.h"
#include "mainwindow.h"
#include "infowidget.h"
#include "circuitwidget.h"
#include "circmatrix.h"
#include "e-element.h"
#include "socket.h"
#include "node.h"

#include "qemudevice.h"

Simulator* Simulator::m_pSelf = nullptr;

Simulator::Simulator( QObject* parent )
         : QObject( parent )
{
    m_pSelf = this;

    m_matrix = new CircMatrix();
    addToElementList( &m_analogClock );
    addToUpdateList( &m_analogClock );

    m_fps = 20;
    m_timerId   = 0;
    m_timerTick_ms = 50;   // 50 ms default
    m_psPerSec  = 1e12;
    m_stepSize  = 1e6;
    m_stepsPS   = 1e6;
    m_maxNlstp  = 100000;
    m_slopeSteps = 0;

    m_errors[0] = "";
    //m_errors[1] = "Could not solve Matrix";
    m_errors[2] = "Add Event: NULL free event";
    m_errors[3] = "LAST_SIM_EVENT reached";

    m_warnings[1] = "NonLinear Not Converging";
    m_warnings[2] = "Probably Circuit Error";  // Warning if matrix diagonal element = 0.
    m_warnings[100] = "AVR crashed !!!";

    resetSim();
    CircuitWidget::self()->setMsg( " "+tr("Stopped")+" ", 1 );

    m_RefTimer.start();
}
Simulator::~Simulator()
{
    m_CircuitFuture.waitForFinished();
    delete m_matrix;
}

inline void Simulator::solveMatrix()
{
    while( m_changedNode ){
        m_changedNode->stampMatrix();
        m_changedNode = m_changedNode->nextCH;
    }
    //if( !m_matrix->solveMatrix() ) // m_matrix sets the eNode voltages
    //    m_warning = 2;             // Warning if diagonal element = 0.
    m_matrix->solveMatrix(); // m_matrix sets the eNode voltages
}

void Simulator::timerEvent( QTimerEvent* e )  //update at m_timerTick_ms rate (50 ms, 20 Hz max)
{
    if( e ) e->accept();

    if( m_state == SIM_WAITING ) return;

    // Get Simulation times
    m_simPsPF = m_circTime-m_tStep;
    m_tStep   = m_circTime;

#ifdef DEBUG_EVENTS
    double simMilis = (double)m_simPsPF/1e9;
    uint64_t eventPerMili = 0;
    if( m_totEvents && simMilis ) eventPerMili = (double)m_totEvents/simMilis;
    double finds = 0;
    if( m_totEvents && m_findEvents ) finds = (double)m_findEvents/(double)m_totEvents;

    qDebug() << "Simulator::timerEvent" << m_totEvents<< eventPerMili << finds << m_maxfinds << m_maxEvents << m_events;
    m_maxEvents = 0;
    m_totEvents = 0;
    m_findEvents = 0;
    m_maxfinds = 0;
#endif

    uint64_t currentTime = m_RefTimer.nsecsElapsed();
    double fps = 1e9/(currentTime-m_timerTime);
    m_realFPS = (m_realFPS*9+fps)/10;
    m_timerTime = currentTime;

    if( m_error )
    {
        CircuitWidget::self()->powerCircOff();
        CircuitWidget::self()->setError( m_errors.value( m_error ) );
        return;
    }
    else if( m_warning > 0 )
    {
        if( m_warning == 200 )
        {
            qDebug() << "Warning: Simulator::addEvent Repeated event";
            m_warning = 0;
        }else{
            int type = (m_warning < 100)? 1:2;
            CircuitWidget::self()->setMsg( m_warnings.value( m_warning), type );
            m_warning = -10;
        }
    }
    else if( m_warning < 0 )
    { if( ++m_warning == 0 ) CircuitWidget::self()->setMsg( " "+tr("Running")+" ", 0 ); }

    if( !m_CircuitFuture.isFinished() ) // Stop remaining parallel thread
    {
        simState_t state = m_state;
        m_state = SIM_WAITING;
        m_CircuitFuture.waitForFinished();
        //while( m_state == SIM_WAITING ){;}
        m_state = state;
    }
    if( m_debug && m_state == SIM_PAUSED ) CircuitWidget::self()->debugPaused();

    for( Updatable* el : m_updateList ) el->updateStep();
    EditorWindow::self()->outPane()->updateStep(); // OutPanel in Editor can be created before this simulator.

    // Calculate Simulation Load
    double timer_ns = m_timerTick_ms*1e6;
    uint64_t simLoop = 0;
    if( m_loopTime > m_refTime ) simLoop = m_loopTime-m_refTime;
    m_simLoad = (m_simLoad+100*simLoop/timer_ns)/2;



    if( m_state == SIM_RUNNING ) // Run Circuit in a parallel thread
        m_CircuitFuture = QtConcurrent::run([this]() { this->runCircuit(); });

    if( Circuit::self()->animateLogic() ) // Moved here to be in parallel with runCircuit thread
    {
        if( (m_timerTime-m_updtTime) >= 2e8 ){ // Animate at 5 FPS
            //Circuit::self()->updateConnectors();
            for( eNode* node : m_eNodeList) node->updateConnectors();
            m_updtTime = m_timerTime;
        }
    }
    if( Circuit::self()->animateCurr() )  // TODO:  optimize
    {
        QStringList pinNames = Circuit::self()->m_pinMap.keys();
        //pinNames.sort();
        for( QString pinName : pinNames )
        {
            Pin* pin = Circuit::self()->m_pinMap.value( pinName );
            if( !pin ) continue;
            pin->resetCurrent();
        }
        for( eNode* node : m_eNodeList) node->updateCurrents();

        QList<Node*> nodeCompList = Circuit::self()->m_nodeList;
        int counter = 0;
        while( !nodeCompList.isEmpty() )
        {
            for( Node* node : nodeCompList )
                if( node->hasCurrents() )
                    nodeCompList.removeOne( node );
            counter++;
            if( counter > 100 ) break;
        }
        Circuit::self()->update();                              // Diagonal wires don't update
    }

    // Calculate Real Simulation Speed
    m_refTime  = m_RefTimer.nsecsElapsed();
    uint64_t deltaRefTime = m_refTime-m_lastRefT;
    if( deltaRefTime >= 1e9 )               // We want steps per 1 Sec = 1e9 ns
    {
        double guiLoad = 100*(double)m_guiTime/(double)deltaRefTime;
        m_guiTime = 0;

        m_realSpeed = (m_tStep-m_lastStep)*10.0/deltaRefTime;
        InfoWidget::self()->setRate( m_realSpeed, m_simLoad, guiLoad, m_realFPS+0.5 );
        m_lastStep = m_tStep;
        m_lastRefT = m_refTime;
    }
    InfoWidget::self()->setCircTime( m_tStep );

    m_guiTime += m_RefTimer.nsecsElapsed()-m_timerTime; // Time in this function
}

//void Simulator::runCircuit()
//{
//    solveCircuit(); // Solve any pending changes
//    if( m_state < SIM_RUNNING ) return;
//
//    eElement* event;
//    uint64_t endRun = m_circTime + m_psPF; // Run upto next Timer event
//    uint64_t nextTime;
//
//    while( m_firstEvent )                       // Simulator event loop
//    {
//        if( m_firstEvent->eventTime > endRun ) break;  // All events for this Timer Tick are done
//
//        nextTime = m_circTime;
//        while( m_circTime == nextTime )         // Run all event with same timeStamp
//        {
//            m_circTime = m_firstEvent->eventTime;
//            event = m_firstEvent;
//            m_firstEvent = event->nextEvent;
//
//            event->nextEvent = nullptr;         // free Event
//            event->eventTime = 0;
//            event->runEvent();                  // Run event callback
//#ifdef DEBUG_EVENTS
//            m_events--;
//#endif
//            if( !m_firstEvent ) break;
//            if( m_state < SIM_RUNNING ) break;  // Needed for QemuDevice
//
//            nextTime = m_firstEvent->eventTime;
//        }
//        solveCircuit();
//        if( m_state < SIM_RUNNING ) break;
//    }
//    m_loopTime = m_RefTimer.nsecsElapsed();
//
//    if     ( m_state >  SIM_WAITING ) m_circTime = endRun;
//    //else if( m_state == SIM_WAITING ) m_state = SIM_PAUSED;
//}

void Simulator::runCircuit()
{
    solveCircuit();                        // Solve any pending changes
    if( m_state < SIM_RUNNING ) return;

    eElement* event;
    uint64_t endRun = m_circTime + m_psPF; // Run upto next Timer event
    uint64_t nextTime;

    while( true )          // Simulator event loop
    {
        //if( QemuDevice::self() )
        //{
        //    if( m_firstEvent ) nextTime = m_firstEvent->eventTime;
        //    else               nextTime = m_circTime + 1000000000;

        //    QemuDevice::self()->runToTime( nextTime ); // This will add events
        //}
        if( !m_firstEvent ) break;


        nextTime = m_firstEvent->eventTime;
        if( nextTime > endRun ) break;           // All events for this Timer Tick are done

        m_circTime = nextTime;
        while( m_circTime == nextTime )          // Run all event with same timeStamp
        {
            event = m_firstEvent;
            m_firstEvent = event->nextEvent;

            event->nextEvent = nullptr;          // free Event
            event->eventTime = 0;
            event->runEvent();                   // Run event callback
#ifdef DEBUG_EVENTS
            m_events--;
#endif
            if( !m_firstEvent ) break;

            nextTime = m_firstEvent->eventTime;
        }
        solveCircuit();
        if( m_state < SIM_RUNNING ) break;
    }
    m_loopTime = m_RefTimer.nsecsElapsed();

    if     ( m_state >  SIM_WAITING ) m_circTime = endRun;
    //else if( m_state == SIM_WAITING ) m_state = SIM_PAUSED;
    //qDebug() << "\n---------------------------------------------\n";
}

void Simulator::solveCircuit()
{
    while( m_changedNode || m_nonLinear || !m_converged ) // Also Proccess changes gererated in voltChanged()
    {
        if( m_changedNode ) solveMatrix();

        if( m_converged ) m_converged = m_nonLinear==nullptr;
        while( !m_converged )              // Non Linear Components
        {
            m_converged = true;
            while( m_nonLinear ){
                m_nonLinear->added = false;
                m_nonLinear->voltChanged();
                m_nonLinear = m_nonLinear->nextChanged;
            }
            if( m_maxNlstp && (m_NLstep++ >= m_maxNlstp) ) { m_warning = 1; return; } // Max iterations reached
            if( m_state < SIM_RUNNING ){ m_converged = false; break; }    // Loop broken without converging
            if( m_changedNode ) solveMatrix();
        }
        if( !m_converged ) return; // Don't run linear until nonliear converged (Loop broken)

        m_NLstep = 0;
        while( m_voltChanged )
        {
            m_voltChanged->added = false;
            m_voltChanged->voltChanged();
            m_voltChanged = m_voltChanged->nextChanged;
        }
        if( m_state < SIM_RUNNING ) break;    // Loop broken without converging
    }
}

void Simulator::resetSim()
{
    m_state    = SIM_STOPPED;
    m_simLoad  = 0;
    m_guiTime  = 0;
    m_error    = 0;
    m_warning  = 0;
    m_lastStep = 0;
    m_tStep    = 0;
    m_lastRefT = 0;
    m_circTime = 1;
    m_updtTime = 0;
    m_NLstep   = 0;
    ///m_pauseCirc = false;
    m_simPsPF = 1;

    InfoWidget::self()->setCircTime( 0 );
    clearEventList();
    m_changedNode = nullptr;
    m_voltChanged = nullptr;
    m_nonLinear = nullptr;

#ifdef DEBUG_EVENTS
    m_events = 0;
    m_maxEvents = 0;
    m_totEvents = 0;
    m_findEvents = 0;
    m_maxfinds = 0;
#endif
}

void Simulator::createNodes()
{
    //qDebug() <<"\ncreateNodes...\n";
    for( eNode* enode : m_eNodeList )
    {
        enode->clear();
        delete enode;
    }
    m_eNodeList.clear();

    int i = 0;
    QStringList pinList;
    QStringList pinNames = Circuit::self()->m_pinMap.keys();
    pinNames.sort();
    for( QString pinName : pinNames )
    {
        Pin* pin = Circuit::self()->m_pinMap.value( pinName );
        if( !pin ) continue;
        if( pinList.contains( pinName ) ) continue;
        if( !pin->conPin() ) continue;
        if( pin->isBus() ) continue;
        if( pinName.startsWith("Node") ) continue;

        eNode* node = new eNode( "eNodeSim-"+QString::number(i) );
        i++;
        //qDebug() <<"--------------createNode "<<i<<node->itemId();
        pin->registerPinsW( node );
        pin->registerEnode( node );
        for( ePin* nodePin : node->getEpins() )
        {
            QString pinId = nodePin->getId();//qDebug() <<pinId<<"\t\t\t"<<nodePin->getEnode()->itemId();
            if( pinId.startsWith("Node") ) continue;
            if( !pinList.contains(pinId) ) pinList.append( pinId );
        }
    }
    /// qDebug() <<"  Created      "<< i << "\teNodes"<<pinList.size()<<"Pins";
}

void Simulator::startSim( bool paused )
{
    resetSim();
    setPsPerSec( m_psPerSec );
    m_debug = paused;
    m_state = SIM_STARTING;

    qDebug() <<"\nStarting Circuit Simulation...\n";

    for( Socket* sock : m_socketList ) sock->connectPins( true );

    createNodes();

    qDebug() <<"  Initializing "<< m_elementList.size() << "\teElements";
    for( eElement* el : m_elementList )    // Initialize all Elements
    {                                      // This can create new eNodes
        //qDebug() << "initializing  "<< el->getId();
        el->eventTime = 0;
        el->initialize();
        el->added = false;
    }

    qDebug() <<"  Initializing "<< m_eNodeList.size()<< "\teNodes";
    for( int i=0; i<m_eNodeList.size(); i++ )         // Initialize eNodes
    {
        eNode* enode = m_eNodeList.at(i);
        enode->setNodeNumber( i );
        enode->initialize();
        //qDebug() << "initializing  "<< enode->itemId();
    }
    for( eElement* el : m_elementList ) el->stamp();

    m_matrix->createMatrix( m_eNodeList );

    /// qDebug() << "\nCircuit Matrix looks good";

    /*double sps100 = 100*(double)m_psPerSec/1e12; // Speed %

    qDebug()  << "\nSpeed:" <<         sps100      << "%"
              << "\nSpeed:" << (double)m_psPerSec  << "\tps per Sec"
              << "\nFPS:  " <<         m_fps       << "\tFrames per Sec"
              << "\nFrame:" << (double)m_psPF      << "\tps per Frame"
              << "\nNonLi:" << (double)m_maxNlstp  << "\tMax Iterations"
              << "\nReact:" << (double)m_reactStep << "\tps Reactive step";*/

    qDebug() << "    Simulation Running... \n";

    if( paused ) // We are debugging
    {
        m_oldState = SIM_RUNNING;
        m_state    = SIM_PAUSED;
    }
    else m_state = SIM_RUNNING;

    if( m_timerId != 0 ) this->killTimer( m_timerId );               // Stop Timer
    m_refTime  = m_RefTimer.nsecsElapsed();
    m_loopTime = m_refTime;
    m_timerTime = m_loopTime;
    m_realFPS = m_fps;
    m_timerId = this->startTimer( m_timerTick_ms, Qt::PreciseTimer ); // Init Timer

    timerEvent( nullptr );
}

void Simulator::stopSim()
{
    if( m_timerId != 0 ){                   // Stop Timer
        this->killTimer( m_timerId );
        m_timerId = 0;
    }
    m_state = SIM_STOPPED;
    if( !m_CircuitFuture.isFinished() ) m_CircuitFuture.waitForFinished();

    qDebug() << "\n    Simulation Stopped ";
    qDebug() << "\n-------------------------------------------------\n ";

    for( eNode* node  : m_eNodeList  )  node->setVolt( 0 );
    for( eElement* el : m_elementList ) el->initialize();
    for( Updatable* el : m_updateList ) el->updateStep();

    clearEventList();
    m_changedNode = nullptr;
}

void Simulator::pauseSim() // Only pause simulation, don't update UI
{
    if( m_state <= SIM_PAUSED ) return;
    m_oldState = m_state;
    m_state = SIM_PAUSED;
}

void Simulator::resumeSim()
{
    if( m_state != SIM_PAUSED ) return;
    m_state = m_oldState;
}

void Simulator::setFps( uint64_t fps )
{
    m_fps = fps;
    setPsPerSec( m_psPerSec );
}

void Simulator::setStepsPerSec( uint64_t sps )
{
    if( sps < 1 ) sps = 1;

    m_stepsPS = sps;           // Steps per second
    setPsPerSec( m_stepsPS*m_stepSize );
}

void Simulator::setPsPerSec( uint64_t psPs )
{
    if( psPs < 1 ) psPs = 1;

    m_psPerSec = psPs;    // picosecond/second
    m_psPF = psPs/m_fps;  // picosecond/frame

    uint64_t fps = m_fps;
    if( m_psPF == 0 ) // We must lower fps to get at least 1 ps per frame
    {
        m_psPF = 1;
        fps = psPs;
    }
    m_timerTick_ms = 1000/fps;  // in ms

    InfoWidget::self()->setTargetSpeed( 100*m_psPerSec/1e12 );
}

void Simulator::clearEventList()
{
    m_firstEvent = nullptr;
}

void Simulator::addEventAt( uint64_t time, eElement* el )
{
    if( m_state < SIM_STARTING ) return;

    if( el->eventTime ){
        m_warning = 200; //
        //qDebug() << "Warning: Simulator::addEvent Repeated event"<<el->getId()<<el->eventTime<<time + m_circTime;
        return;
    }

    eElement* last  = nullptr;
    eElement* event = m_firstEvent;

#ifdef DEBUG_EVENTS
    uint64_t finds = 0;
#endif

    while( event )
    {
#ifdef DEBUG_EVENTS
        finds++;
#endif
        if( time <= event->eventTime ) break; // Insert event here
        last  = event;
        event = event->nextEvent;
    }
    el->eventTime = time;

#ifdef DEBUG_EVENTS
    if( finds > m_maxfinds ) m_maxfinds = finds;
    m_events++;
    m_totEvents++;
    m_findEvents += finds;
    if( m_events > m_maxEvents ) m_maxEvents = m_events;
#endif

    if( last ) last->nextEvent = el;
    else       m_firstEvent = el; // List was empty or insert First

    el->nextEvent = event;
}

void Simulator::cancelEvents( eElement* el )
{
    if( el->eventTime == 0 ) return;
    eElement* event = m_firstEvent;
    eElement* last  = nullptr;
    eElement* next  = nullptr;
    el->eventTime = 0;

    while( event ){
        next = event->nextEvent;
        if( el == event )
        {
            if( last ) last->nextEvent = next;
            else       m_firstEvent = next;
            event->nextEvent = nullptr;
#ifdef DEBUG_EVENTS
            m_events--;
#endif
        }
        else last = event;
        event = next;
}   }

void Simulator::addToEnodeList( eNode* nod )
{ if( !m_eNodeList.contains(nod) ) m_eNodeList.append( nod ); }

void Simulator::addToElementList( eElement* el )
{ if( !m_elementList.contains(el) ) m_elementList.append(el); }

void Simulator::remFromElementList( eElement* el )
{ m_elementList.removeOne(el); }

void Simulator::addToUpdateList( Updatable* el )
{ if( !m_updateList.contains(el) ) m_updateList.append(el); }

void Simulator::remFromUpdateList( Updatable* el )
{ m_updateList.removeOne(el); }

void Simulator::addToSocketList( Socket* el )
{ if( !m_socketList.contains(el) ) m_socketList.append(el); }

void Simulator::remFromSocketList( Socket* el )
{ m_socketList.removeOne(el); }

#include "moc_simulator.cpp"
