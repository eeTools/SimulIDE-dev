/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QGraphicsProxyWidget>
#include <QFile>

#include "logicanalizer.h"
#include "itemlibrary.h"
#include "connector.h"
#include "circuitwidget.h"
#include "circuit.h"
#include "simulator.h"
#include "lachannel.h"
#include "lawidget.h"
#include "datalawidget.h"
#include "tunnel.h"
#include "e-node.h"
#include "iopin.h"

#include "stringprop.h"
#include "doubleprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("LAnalizer",str)

Component* LAnalizer::construct( QString type, QString id )
{ return new LAnalizer( type, id ); }

LibraryItem* LAnalizer::libraryItem()
{
    return new LibraryItem(
        tr( "Logic Analyzer" ),
        "Meters",
        "lanalizer.png",
        "LAnalizer",
        LAnalizer::construct );
}

LAnalizer::LAnalizer( QString type, QString id )
         : PlotBase( type, id )
{
    m_numChannels = 8;
    m_bufferSize = 100000;

    m_laWidget   = new LaWidget( CircuitWidget::self(), this );
    m_dataWidget = new DataLaWidget( nullptr, this );
    m_proxy = Circuit::self()->addWidget( m_dataWidget );
    m_proxy->setParentItem( this );
    m_dataWidget->show();
    m_display = m_laWidget->display();
    m_display->setPlotBase( this );
    m_display->setFixedSize( m_baSizeX+6*8, m_baSizeY+2*8 );
    m_display->setChannels( 8 );
    m_display->setTracks( 8 );

    m_pin.resize(8);
    m_inPin.resize(8);
    for( int i=0; i<8; ++i )
    {
        m_pin[i] = m_inPin[i] = new IoPin( 180, QPoint(-80-8,-64+16*i ), id+"-Pin"+QString::number(i), 0, this, undef_mode );
        double admit = m_connectGnd ? m_inputAdmit : 0;
        m_inPin[i]->setInputAdmit( admit );
        LaChannel* ch = new LaChannel( this, id+"Chan"+QString::number(i) );

        ch->m_channel = i;
        ch->setPin( m_pin[i] );
        ch->m_buffer.resize( m_bufferSize );
        ch->m_time.resize( m_bufferSize );
        m_channel[i] = ch;

        m_display->setChannel( i, m_channel[i] );
        m_display->setColor( i, m_color[i%4] );
        m_display->setLimits( i, 1, 0 );
        m_display->setVPos( i, -0.5 );

        m_dataWidget->setColor( i, m_color[i%4] );
    }
    m_updtCount = 0;
    m_thresholdR = 0;
    m_thresholdF = 0;

    setThresholdR( 2.5 );
    setThresholdF( 2.5 );

    setTimePos( 0 );
    setTimeDiv( 1e9 );   // 1 ms
    setVoltDiv( 0.6/4 ); // For Logic values 0 : 1

    setLabelPos(-90,-100, 0);
    expand( false );
    setTrigger( 9 ); // Trigger = None

    addPropGroup( { tr("Export"), {
        new IntProp <LAnalizer>("TimeStep", tr("Base Time Step"), "_ps"
                               , this, &LAnalizer::timeStep,   &LAnalizer::setTimeStep,0,"uint" ),

        new BoolProp<LAnalizer>("AutoExport", tr("Export at pause"),""
                               , this, &LAnalizer::autoExport, &LAnalizer::setAutoExport ),
    },0} );

    addPropGroup( { "Hidden1", {
        new DoubProp<LAnalizer>("TresholdR", "", "V"
                               , this, &LAnalizer::thresholdR, &LAnalizer::setThresholdR ),

        new StrProp <LAnalizer>("Bus", "", ""
                               , this, &LAnalizer::busStr, &LAnalizer::setBusStr ),

        new DoubProp<LAnalizer>("TresholdF", "", "V"
                               , this, &LAnalizer::thresholdF, &LAnalizer::setThresholdF )
    }, groupHidden } );
}
LAnalizer::~LAnalizer()
{
    m_proxy->setWidget( nullptr );
    delete m_dataWidget;

    m_laWidget->setParent( nullptr );
    m_laWidget->close();
    delete m_laWidget;
}

void LAnalizer::updateStep()
{
    if( !Simulator::self()->isPaused() )
    {
        uint64_t simTime = Simulator::self()->circTime(); // free running
        if( m_trigger < 9 )
        {
            uint64_t risEdge;
            if( m_trigger == 8 ) risEdge = m_risEdge;
            else                 risEdge = m_channel[m_trigger]->m_risEdge;

            if( risEdge > 0 ) // We have a Trigger
            {
                if( m_trigger < 8 ) m_channel[m_trigger]->m_risEdge = 0;
                simTime = risEdge;
                m_updtCount = 0;
            }
            else if( ++m_updtCount < 20 ) return;
        }

        m_display->setTimeEnd( simTime );

        for( int i=0; i<8; i++ )
        {
            bool connected = m_pin[i]->connector();

            if( !connected && !m_pin[i]->isBus() )
            {
                QString chTunnel = m_channel[i]->m_chTunnel;

                eNode* enode = Tunnel::getEnode( chTunnel );
                m_pin[i]->setEnode( enode );
                if( enode )
                {
                    enode->voltChangedCallback( m_channel[i] );
                    connected = true;
                }
                display()->connectChannel( i, connected );
            }
            m_channel[i]->m_connected = connected;
            if( !connected ) m_channel[i]->initialize();
            m_channel[i]->m_trigIndex = m_channel[i]->m_bufferCounter;
        }
        m_risEdge = 0;
    }
    m_display->update();

    if( m_changed ){
        m_changed = false;
        double admit = m_connectGnd ? m_inputAdmit : 0;
        for( IoPin* pin : m_inPin ){
            pin->setInputAdmit( admit );
            pin->stampAdmitance( admit );
        }
    }
}

void LAnalizer::expand( bool e )
{
    m_expand = e;
    m_screenSizeY = m_baSizeY+2*4;
    if( e ){
        m_display->setMaximumSize( 9999, 9999 );
        m_laWidget->getDispLayout()->insertWidget( 0, m_display );
        m_laWidget->setWindowTitle( idLabel() );
        m_laWidget->show();
        m_screenSizeX = 0;
    }else{
        m_screenSizeX = m_baSizeX+2*4;
        m_laWidget->hide();
        m_dataWidget->getLayout()->addWidget( m_display );
        m_display->setMaximumSize( m_screenSizeX, m_screenSizeY );
    }
    m_display->setMinimumSize( m_screenSizeX, m_screenSizeY );

    int widgetSizeX = m_screenSizeX+68+4;
    int widgetSizeY = m_screenSizeY+4;
    int centerY = widgetSizeY/2;
    m_dataWidget->setFixedSize( widgetSizeX, widgetSizeY );
    m_proxy->setPos( -80+2, -centerY-2+4 );
    m_area = QRectF( -80, -centerY, widgetSizeX+4, widgetSizeY+4+2 );

    m_display->setExpand( e );
    m_display->updateValues();
    Circuit::self()->update();
}

QString LAnalizer::timPos()
{
    return QString::number( m_timePos );
}

void LAnalizer::setTimPos( QString tp )
{
    setTimePos( tp.toLongLong() );
}

QString LAnalizer::volDiv()
{
    return QString::number( m_voltDiv );
}

void LAnalizer::setVolDiv( QString vd )
{
    setVoltDiv( vd.toDouble() );
}

void LAnalizer::setTimeDiv( uint64_t td )
{
    if( td < 1 ) td = 1;
    PlotBase::setTimeDiv( td );
    m_laWidget->updateTimeDivBox( td );
}

void LAnalizer::setThresholdR( double thr )
{
    if( thr < m_thresholdF ) thr = m_thresholdF;
    m_thresholdR = thr;
    m_laWidget->updateThresholdR( thr );
}

void LAnalizer::setThresholdF( double thr )
{
    if( thr > m_thresholdR ) thr = m_thresholdR;
    m_thresholdF = thr;
    m_laWidget->updateThresholdF( thr );
}

void LAnalizer::setTimePos( int64_t tp )
{
    m_timePos = tp;
    for( int i=0; i<8; ++i ) m_display->setHPos( i, m_timePos );
    m_laWidget->updateTimePosBox( tp );
}

void LAnalizer::moveTimePos( int64_t delta )
{
    m_timePos = m_timePos+delta;
    for( int i=0; i<8; ++i ) m_display->setHPos( i, m_timePos );
    m_laWidget->updateTimePosBox( m_timePos );
}

void LAnalizer::setVoltDiv( double )
{
    m_voltDiv = 0.6/4; // For Logic values 0 : 1
    for( int i=0; i<8; i++ ) m_display->setVTick( i, m_voltDiv );
}

void LAnalizer::setTrigger( int ch )
{
    m_trigger = ch;
    m_laWidget->setTrigger( ch );

    bool pauseOnCond = ( m_trigger == 8 ); // Pause on condition
    for( int i=0; i<8; i++ ) m_channel[i]->m_pauseOnCond = pauseOnCond;
}

QString LAnalizer::busStr()
{
    QString list;
    QString bus;
    for( int i=0; i<8; ++i ){
        bus = m_channel[i]->isBus()? "true":"false";
        list.append( bus ).append(",");
    }
    return list;
}

void LAnalizer::setBusStr( QString hc )
{
    QStringList list = hc.split(",");
    for( int i=0; i<8; ++i ){
        if( i == list.size() ) break;
        bool bus = (list.at(i) == "true")? true:false;
        m_laWidget->setIsBus( i, bus );
}   }

void LAnalizer::setConds( QString conds )
{
    m_laWidget->setConds( conds );
    updateConds( conds );
}

void LAnalizer::setTunnels( QString tunnels )
{
    QStringList list = tunnels.split(",");
    for( int i=0; i<list.size(); i++ )
    {
        if( i >= m_numChannels ) break;
        m_channel[i]->m_chTunnel = list.at(i);
        m_dataWidget->setTunnel( i, list.at(i) );
}   }

void LAnalizer::dumpData( const QString &fn )
{
    QChar identifiers[8] = {'*', '"', '#', '$', '%', '&', '(', ')'};

    QFile file(fn);
    if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) return;
    m_exportFile = fn;

    QTextStream out( &file );
    out.setLocale( QLocale::C );

    QMultiMap<uint64_t, sample_t> samples; // collect timing data, use QMap to implicitely sort it

    uint64_t startTime = m_display->startTime();
    if( (int64_t)startTime+m_timePos >= 0 ) startTime += m_timePos;
    else                                    startTime  = 1;

    uint64_t endTime = m_display->endTime()+m_timePos;
    uint64_t lastTime = (endTime-startTime)/m_timeStep;
    uint64_t pTime = 0;
    double pVal=-1;

    QString varDef;
    QString dumpVars = "\n$dumpvars\n";
    uint64_t gcd = 1;  // Problems in Pulseview using gcd

    for( uint ch=0; ch<8; ++ch )
    {
        if( !m_channel[ch]->m_connected ) continue;

        QString name = m_channel[ch]->getChName();             // Get channel name
        if( name.isEmpty() ) name = "D"+QString::number( ch ); // If name is empty set name = Dn

        varDef += "$var wire 1 " + QString( identifiers[ch] )+" "+name+" $end\n";

        bool init = false;
        double initVal = 0;
        int index = m_channel[ch]->m_bufferCounter; // Start with the first sample (circular buffer)

        for( int i=0; i<m_bufferSize; ++i )
        {
            index++;
            if( index >= m_bufferSize ) index -= m_bufferSize; // It's a circular buffer

            double   val  = m_channel[ch]->m_buffer[index];
            uint64_t time = m_channel[ch]->m_time[index];

            if( pTime == time ) continue;                      // Avoid repeated times
            pTime = time;

            if( time == 0 ) continue;                            // Empty samples: Simulation times start at 1 ps
            if( time <= startTime ) { initVal = val; continue; } // Previous value from first valid sample
            if( time > endTime ) { time = endTime; val = pVal; } // Add final value (screen edge)
            if( !init ) {
                dumpVars += QString::number(initVal)+identifiers[ch]+"\n"; // Add initial value
                init = true;
            }
            pVal = val;
            time = (time-startTime)/m_timeStep;

            //if( gcd > 0 ) gcd = getGcd( gcd, time ); // Get Greatest Common Denominator
            //else          gcd = time;

            samples.insert( time, { val, ch } );
            if( time == lastTime ) break;            // All samples before endTime already registered
        }
    }
    dumpVars += "$end\n";
    //if( gcd < 1 ) gcd = 1; // This should not happen

    out <<"$timescale "<< gcd*m_timeStep <<"ps $end"<< Qt::endl<< Qt::endl;
    out << varDef;
    out << Qt::endl <<"$enddefinitions $end"<< Qt::endl;
    out << dumpVars;

    uint64_t timeStamp;
    for( uint64_t time : samples.uniqueKeys() )
    {
        timeStamp = time/gcd;
        out << Qt::endl <<"#"<< timeStamp;
        for( sample_t sample : samples.values( time ) )
            out <<" "<< sample.value <<identifiers[sample.channel];
    }
    out << Qt::endl <<"#"<< timeStamp+1; // last time stamp
    file.close();
}

uint64_t LAnalizer::getGcd( uint64_t a, uint64_t b )  // Greatest Common Denominator
{
    uint64_t h;
    do {
        h = a % b;
        a = b;
        b = h;
    } while (b != 0);

    return a;
}
