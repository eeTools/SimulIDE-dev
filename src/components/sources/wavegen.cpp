/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QtMath>
#include <QDebug>

#include "wavegen.h"
#include "iopin.h"
#include "simulator.h"
#include "circuit.h"
#include "itemlibrary.h"
#include "propdialog.h"
#include "mainwindow.h"
#include "circuitwidget.h"
#include "e-reactive.h"

#include "doubleprop.h"
#include "stringprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("WaveGen",str)

Component* WaveGen::construct( QString type, QString id )
{ return new WaveGen( type, id ); }

LibraryItem* WaveGen::libraryItem()
{
    return new LibraryItem(
        tr("Wave Generator"),
        "Sources",
        "wavegen.png",
        "WaveGen",
        WaveGen::construct );
}

WaveGen::WaveGen( QString type, QString id )
       : ClockBase( type, id )
{
    m_bipolar  = false;
    m_floating = false;
    m_voltBase = -2.5;
    m_phaseShift = 0;
    m_voltMid  = 0;
    m_lastVout = 0;
    m_minSteps = 100;
    m_waveType = Sine;
    m_wavePixmap = nullptr;

    m_pin.resize(2);
    m_pin[1] = m_gndpin = new IoPin( 0, QPoint(16,4), id+"-gndnod", 0, this, source );
    m_gndpin->setVisible( false );
    
    //setSteps( 100 );
    setDuty( 50 );

    Simulator::self()->addToUpdateList( this );

    remPropGroup( tr("Main") );

    QString waves = "Sine,Saw,Triangle,Square,Random,Wav;"
            +tr("Sine")+","+tr("Saw")+","+tr("Triangle")+","+tr("Square")+","+tr("Random")+",Wav";

    addPropGroup( { tr("Main"), {
        new StrProp <WaveGen>("Wave_Type", tr("Wave Type"), waves
                             , this, &WaveGen::waveType, &WaveGen::setWaveType,0,"enum" ),

        new DoubProp<WaveGen>("Freq", tr("Frequency"), "kHz"
                             , this, &WaveGen::freq, &WaveGen::setFreq ),

        new DoubProp<WaveGen>("Phase", tr("Phase shift"), "_º"
                             , this, &WaveGen::phaseShift, &WaveGen::setPhaseShift ),

        new IntProp <WaveGen>("Steps", tr("Minimum Steps"), ""
                             , this, &WaveGen::minSteps, &WaveGen::setMinSteps ),

        new DoubProp<WaveGen>("Duty", tr("Duty"), "_\%"
                             , this, &WaveGen::duty, &WaveGen::setDuty ),

        new StrProp <WaveGen>("File", tr("File"), ""
                             , this, &WaveGen::fileName, &WaveGen::setFile ),

        new BoolProp<WaveGen>("Always_On", tr("Always On"), ""
                             , this, &WaveGen::alwaysOn, &WaveGen::setAlwaysOn )
    },0} );

    addPropGroup( { tr("Electric"), {
        new BoolProp<WaveGen>("Bipolar", tr("Bipolar"), ""
                             , this, &WaveGen::bipolar, &WaveGen::setBipolar, propNoCopy ),

        new BoolProp<WaveGen>("Floating", tr("Floating"), ""
                             , this, &WaveGen::floating, &WaveGen::setFloating, propNoCopy ),

        new DoubProp<WaveGen>("Semi_Ampli", tr("Semi Amplitude"), "V"
                             , this, &WaveGen::semiAmpli, &WaveGen::setSemiAmpli ),

        new DoubProp<WaveGen>("Mid_Volt", tr("Middle Voltage"), "V"
                             , this, &WaveGen::midVolt, &WaveGen::setMidVolt )
    },0} );

    setWaveType("Sine");
}
WaveGen::~WaveGen()
{
    delete m_wavePixmap;
}

bool WaveGen::propNotFound( QString prop, QString val )
{
    if( prop =="Volt_Base" ) m_voltBase = val.toDouble(); //  Old: TODELETE
    else return ClockBase::propNotFound( prop, val );
    return true;
}

void WaveGen::initialize()
{
    Simulator::self()->cancelEvents( this );
    AnalogClock::self()->remClkElement( this );

    if     ( m_waveType == Wav    ) m_eventTime = m_psPerCycleInt;
    else if( m_waveType == Square ) m_eventTime = m_psPerCycleDbl-m_halfW;
    else if( m_waveType == Random ) m_eventTime = m_psPerCycleInt/3;
    else{
        m_eventTime = 0;
        if( m_isRunning ) AnalogClock::self()->addClkElement( this );
    }
    if( m_isRunning && m_eventTime ) Simulator::self()->addEvent( m_eventTime, this );
}

void WaveGen::stamp()
{
    //ClockBase::stamp();
    m_phaseTime = m_psPerCycleInt*m_phaseShift/360;
    m_lastVout = m_vOut = 0;
    m_index = 0;

    if( m_bipolar && m_floating )
    {
        m_outpin->skipStamp( true );
        m_gndpin->skipStamp( true );

        m_outpin->setEnodeComp( m_gndpin->getEnode() );
        m_gndpin->setEnodeComp( m_outpin->getEnode() );

        m_outpin->createCurrent();
        m_gndpin->createCurrent();

        m_outpin->setImpedance( low_imp );
        m_gndpin->setImpedance( low_imp );
    }
    else{
        m_outpin->skipStamp( false );
        m_gndpin->skipStamp( false );

        m_outpin->setImpedance( low_imp );
        m_gndpin->setImpedance( low_imp );
    }
}

void WaveGen::runEvent()
{
    m_time = fmod( Simulator::self()->circTime() - m_phaseTime, m_psPerCycleDbl );

    switch( m_waveType ) {
        case Sine:     genSine();     break;
        case Saw:      genSaw();      break;
        case Triangle: genTriangle(); break;
        case Square:   genSquare();   break;
        case Random:   genRandom();   break;
        case Wav:      genWav();      break;
    }

    if( m_vOut != m_lastVout )
    {
        m_lastVout = m_vOut;

        if( m_bipolar )
        {
            double volt = m_voltage*(m_vOut-0.5);
            if( m_floating ) {
                m_outpin->stampCurrent( volt*high_imp );
                m_gndpin->stampCurrent(-volt*high_imp );
            }else{
                volt /= 2;
                m_outpin->setVoltage( m_voltMid+volt );
                m_gndpin->setVoltage( m_voltMid-volt );
            }
        }
        else m_outpin->setVoltage( m_voltBase+m_voltage*m_vOut );
    }

    if( m_eventTime ) Simulator::self()->addEvent( m_eventTime, this );
}

void WaveGen::genSine()
{
    m_time = qDegreesToRadians( (double)m_time*360/m_psPerCycleDbl );
    m_vOut = sin( m_time )/2+0.5;
}

void WaveGen::genSaw()
{
    m_vOut = m_time/m_psPerCycleDbl;
}

void WaveGen::genTriangle()
{
    if( m_time >= m_halfW ) m_vOut = 1-(m_time-m_halfW)/(m_psPerCycleDbl-m_halfW);
    else                    m_vOut = m_time/m_halfW;
}

void WaveGen::genSquare()
{
    if( m_vOut == 1 )
    {
        m_vOut = 0;
        m_eventTime = m_psPerCycleDbl-m_halfW;
    }else{
        m_vOut = 1;
        m_eventTime = m_halfW;
    }
    m_eventTime += getRemainer();
}

void WaveGen::genRandom()
{
    m_vOut = (double)rand()/(double)RAND_MAX;
}

void WaveGen::genWav()
{
    m_vOut = m_data.at( m_index );

    m_index++;
    if( m_index >= m_data.size() ) m_index = 0;
}

void WaveGen::setMinSteps( int steps )
{
    if( steps < 10 ) steps = 10;
    m_minSteps = steps;
    WaveGen::setFreq( m_freq );
}

void WaveGen::setDuty( double duty )
{
    if( duty > 100 ) duty = 100;
    m_duty = duty;
    m_halfW = m_psPerCycleDbl*m_duty/100;
}

void WaveGen::setFreq( double freq )
{
    //ClockBase::setFreq( freq );

    double psPerCycleDbl = 1e6*1e6/freq;

    uint64_t minimum = AnalogClock::self()->getStep()*m_minSteps; // Minimum 100 steps per wave cycle

    if( psPerCycleDbl < minimum ) // Scale Step
    {
        double divider = minimum/psPerCycleDbl;
        AnalogClock::self()->setDivider( std::ceil( divider ) );
    }

    m_psPerCycleDbl = psPerCycleDbl;
    m_psPerCycleInt = psPerCycleDbl;

    m_freq = freq;
    m_remainder = 0;

    setRunning( m_isRunning || m_alwaysOn );

    setDuty( m_duty );

    if     ( m_waveType == Square ) m_eventTime = m_psPerCycleDbl-m_halfW;
    else if( m_waveType == Random ) m_eventTime = m_psPerCycleInt/3;
}

void WaveGen::setSemiAmpli( double v )
{
    m_voltBase = m_voltMid-v;
    m_voltage = v*2;
}

void WaveGen::setMidVolt( double v )
{
    m_voltBase = v-m_voltage/2;
    m_voltMid = v;
}

void WaveGen::setBipolar(bool b )
{
    if( m_bipolar == b ) return;
    m_bipolar = b;

    m_gndpin->setVisible( b );
    if( !b ) m_gndpin->removeConnector();

    if( b ) m_outpin->setY( -4 );
    else    m_outpin->setY( 0 );

    updtProperties();
}

void WaveGen::setFloating( bool f )
{
    if( m_floating == f ) return;
    m_floating = f;

    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    updtProperties();
}

void WaveGen::setLinkedValue( double v, int i )
{
    if( i ) setDuty( v );
    else    setFreq( v );
}

void WaveGen::setWaveType( QString type )
{
     Simulator::self()->pauseSim();
    m_waveTypeStr = type;
    if( m_showVal && (m_showProperty == "Wave_Type") )
        setValLabelText( type );

    QString pixmapPath;

    if     ( type == "Sine"    ) { pixmapPath = ":/sin.png"; m_waveType = Sine;}
    else if( type == "Saw"     ) { pixmapPath = ":/saw.png"; m_waveType = Saw;}
    else if( type == "Triangle") { pixmapPath = ":/tri.png"; m_waveType = Triangle;}
    else if( type == "Square"  ) { pixmapPath = ":/sqa.png"; m_waveType = Square;}
    else if( type == "Random"  ) { pixmapPath = ":/rnd.png"; m_waveType = Random;}
    else if( type == "Wav"     ) { pixmapPath = ":/wav.png"; m_waveType = Wav;}

    if( m_wavePixmap ) delete m_wavePixmap;
    m_wavePixmap = new QPixmap( pixmapPath );
    updtProperties();
    if( Simulator::self()->isRunning() ) initialize();
    Simulator::self()->resumeSim();
}

void WaveGen::updtProperties()
{
    if( !m_propDialog ) return;

    bool showFile = false;
    bool showDuty = false;

    if     ( m_waveType == Triangle) showDuty = true;
    else if( m_waveType == Square  ) showDuty = true;
    else if( m_waveType == Wav     ) showFile = true;

    m_propDialog->showProp("File", showFile );
    m_propDialog->showProp("Duty", showDuty );

    //m_propDialog->showProp("Mid_Volt", !m_bipolar || !m_floating );
    m_propDialog->showProp("Floating", m_bipolar );
    m_propDialog->adjustWidgets();
}

void WaveGen::slotProperties()
{
    Component::slotProperties();
    updtProperties();
}

void WaveGen::contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu )
{
    if( m_waveType == Wav )
    {
        QAction* loadAction = menu->addAction( QIcon(":/load.svg"),tr("Load Wav File") );
        QObject::connect( loadAction, &QAction::triggered, [=](){ slotLoad(); } );

        menu->addSeparator();
    }
    Component::contextMenu( event, menu );
}

void WaveGen::slotLoad()
{
    QString fil = m_background;
    if( fil.isEmpty() ) fil = Circuit::self()->getFilePath();

    const QString dir = fil;

    QString fileName = QFileDialog::getOpenFileName( 0l, tr("Load Image"), dir,
                       tr("Wav files (*.wav);;All files (*.*)"));

    if( fileName.isEmpty() ) return; // User cancels loading

    setFile( fileName );
}

void WaveGen::setFile( QString fileName )
{
    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    m_fileName = fileName;
    if( fileName.isEmpty() ) return;

    QDir circuitDir;
    circuitDir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    QString fileNameAbs  = circuitDir.absoluteFilePath( fileName );

    if( !QFileInfo::exists( fileNameAbs ) )
    {
        qDebug() << "WaveGen::setFile Error: file doesn't exist:\n"<<fileNameAbs<<"\n";
        return;
    }
    QFile WAVFile( fileNameAbs );
    if( !WAVFile.open( QIODevice::ReadWrite ) )
    {
       qDebug() << "WaveGen::setFile Could not open:\n" << fileNameAbs<<"\n";
       return;
    }
    Simulator::self()->pauseSim();

    QDataStream dataStream( &WAVFile );
    dataStream.setByteOrder( QDataStream::LittleEndian );
    char strm[4];
    int32_t dataSize = 0;
    while( true )                   // Read Header
    {
        dataStream.readRawData( strm, 4 );    // 4 File Format = "RIFF"
        if( QString( QByteArray(strm,4) ) != "RIFF" ) break;
        dataStream.readRawData( strm, 4 );    // 4 File Size
        dataStream.readRawData( strm, 4 );    // 4 File Type = "WAVE"
        if( QString( QByteArray(strm,4) ) != "WAVE" ) break;
        dataStream.readRawData( strm, 4 );    // 4 Format section header= "fmt "
        if( QString( QByteArray(strm,4) ) != "fmt " ) break;
        dataStream.readRawData( strm, 4 );    // 4 Size of Format section
        dataStream >> m_audioFormat;          // 2 Format type
        dataStream >> m_numChannels;          // 2 Number of channels
        dataStream >> m_sampleRate;           // 4 Sample rate
        dataStream.readRawData( strm, 4 );    // 4 Byte rate: (Sample Rate * BitsPerSample * Channels) / 8.
        dataStream >> m_blockSize;            // 2 Block size (bytes): (BitsPerSample * Channels) / 8.
        dataStream >> m_bitsPerSample;        // 2 Bits per sample
        dataStream.readRawData( strm, 4 );    // 4 Data section header = "data"
        QString section = QString( QByteArray(strm,4) );
        while( section  != "data" )
        {
            qDebug() << "WaveGen::setFile Warning: Section not supported: " << section;
            uint32_t size = 0;   // 4 Size of Format section
            dataStream >> size;
            uint8_t data = 0;
            for( uint i=0; i<size; i++ ) dataStream >> data;
            dataStream.readRawData( strm, 4 );
            section = QString( QByteArray(strm,4) );
        }
        dataStream >> dataSize; // Size of Data section
        break;
    }
    if( dataSize )                  // Read samples
    {
        int bytes = m_blockSize/m_numChannels;

        if( m_audioFormat == 1 ) // PCM
        {
            qDebug() << "WaveGen::setFile Audio format: PCM" << m_bitsPerSample << "bits"<<m_numChannels<<"Channels";
            if( bytes == 1 ){
                m_minValue = 0;
                m_maxValue = 255;
            }else if( bytes == 2 ){
                m_minValue = -32768;
                m_maxValue = 32767;
            }
            else { qDebug() << "WaveGen::setFile Error: PCM format"<<bytes<<"bytes"; WAVFile.close(); return;}
        }
        else if( m_audioFormat == 3 ) // IEEE_FLOAT
        {
            if( bytes == 4 || bytes == 8 ){
                qDebug() << "WaveGen::setFile Audio format: IEEE_FLOAT" << m_bitsPerSample << "bits"<<m_numChannels<<"Channels";
                m_minValue = -1;
                m_maxValue = 1;
            }
            else { qDebug() << "WaveGen::setFile Error: IEEE_FLOAT format"<<bytes<<"bytes"; WAVFile.close(); return;}
        }
        else { qDebug() << "WaveGen::setFile Error: Audio format not supported:"; WAVFile.close(); return;}

        char sample[bytes];
        m_data.clear();
        //m_data.resize( m_numChannels );

        for( int block=0; block < dataSize/m_blockSize; ++block )
        {
            for( int ch=0; ch < m_numChannels; ++ch )
            {
                dataStream.readRawData( sample, bytes );
                if( ch != 0 ) continue;

                if( m_audioFormat == 1 ) // PCM
                {
                    int16_t data = 0;
                    memcpy( &data, sample, bytes );
                    m_data.emplace_back( normalize( data ) );
                }
                else if( m_audioFormat == 3 ) // IEEE_FLOAT
                {
                    if( bytes == 4 ){         // 32 bits
                        float data = 0;
                        memcpy( &data, sample, bytes );
                        m_data.emplace_back( normalize( data ) );
                    }
                    else if( bytes == 8 ){    // 64 bits
                        double data = 0;
                        memcpy( &data, sample, bytes );
                        m_data.emplace_back( normalize( data ) );
                    }
                }
            }
        }
        setFreq( m_sampleRate );
        //setSteps( 1 );
        qDebug() << "WaveGen::setFile Success Loaded wav file:\n" << fileNameAbs;
    }
    else qDebug() << "WaveGen::setFile Error reading wav file:\n" << fileNameAbs;
    qDebug() << "\n";

    WAVFile.close();
}

inline double WaveGen::normalize( double data )
{
    if     ( data > m_maxValue ) return 1;
    else if( data < m_minValue ) return 0;

    data = data-m_minValue;
    data /= (m_maxValue-m_minValue);
    return data;
}

void WaveGen::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    if( m_hidden ) return;
    Component::paint( p, o, w );

    if (  m_isRunning ) p->setBrush( QColor( 250, 200, 50 ) );
    else                p->setBrush( QColor( 230, 230, 255 ) );

    p->drawRoundedRect( m_area,2 ,2 );
    p->drawPixmap( m_area.x()+3, m_area.y()+4,16, 8, *m_wavePixmap );

    Component::paintSelected( p );
}
