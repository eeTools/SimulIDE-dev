/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QCoreApplication>
#include <QPainter>
#include <QtMath>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSink>

#include "audio_out.h"
#include "connector.h"
#include "itemlibrary.h"
#include "propdialog.h"
#include "pin.h"
#include "label.h"
#include "simulator.h"

#include "doubleprop.h"
#include "boolprop.h"

#define tr(str) simulideTr("AudioOut",str)

Component* AudioOut::construct( QString type, QString id )
{ return new AudioOut( type, id ); }

LibraryItem* AudioOut::libraryItem()
{
    return new LibraryItem(
        tr("Audio Out"),
        "Other Outputs",
        "audio_out.png",
        "AudioOut",
        AudioOut::construct);
}

AudioOut::AudioOut( QString type, QString id )
        : Component( type, id )
        , eResistor( id )
{
    m_area = QRect( -12, -24, 24, 40 );
    
    m_pin.resize( 2 );
    m_ePin[0] = m_pin[0] = new Pin( 180, QPoint(-16,-8), id+"-lPin", 0, this);
    m_pin[0]->setLabelColor( Qt::red );
    m_pin[0]->setFontSize( 9 );
    m_pin[0]->setLength( 6 );
    m_pin[0]->setSpace( 1.7 );
    m_pin[0]->setLabelText("+");

    m_ePin[1] = m_pin[1] = new Pin( 180, QPoint(-16,0), id+"-rPin", 1, this);
    m_pin[1]->setLabelColor( QColor( 0, 0, 0 ) );
    m_pin[1]->setFontSize( 9 );
    m_pin[1]->setLength( 6 );
    m_pin[1]->setSpace( 1.7 );
    m_pin[1]->setLabelText("–");  // U+2013

    setLabelPos(-20,-36, 0);
    
    m_admit = 1.0/8;
    m_impedance = 8;
    m_buzzer = false;
    m_audioOutput = nullptr;
    m_ioDevice = nullptr;

    // m_deviceinfo is a QAudioDevice
    m_deviceinfo = QMediaDevices::defaultAudioOutput();

    if (!m_deviceinfo.isNull())
    {
        const auto deviceInfos = QMediaDevices::audioOutputs();

        if (deviceInfos.isEmpty())
        {
            qDebug() << "   Error: No Audio Output Devices Found at all";
            qDebug() << "Check that Qt6 multimedia & multimedia-plugins packages are installed";
        }
        else
        {
            qDebug() << "   Error: No default Audio Output Device Found";
            qDebug() << "Audio Output Devices available:";

            for (const QAudioDevice &deviceInfo : deviceInfos)
                qDebug() << "Device name:" << deviceInfo.description();
        }
    }
    m_format = m_deviceinfo.preferredFormat();
    m_format.setChannelCount(1);
    m_format.setSampleRate(m_format.sampleRate());   // keep preferred rate, or set your own
    m_format.setSampleFormat(QAudioFormat::UInt8);   // replaces sample size/type/byte order


    if (!m_deviceinfo.isFormatSupported(m_format))
    {
        qDebug() << "Warning: Default format not supported - trying to use preferred";
        m_format = m_deviceinfo.preferredFormat();
    }

    // Print sample format info
    int bits = m_format.bytesPerSample() * 8;

    qDebug() << m_format.sampleRate()
             << m_format.channelCount()
             << bits;

    m_audioOutput = new QAudioSink(m_deviceinfo);  // OK
    //m_audioOutput->setFormat(m_format);            // OK
    m_ioDevice = m_audioOutput->start();

    addPropGroup( { tr("Main"), {
        new BoolProp<AudioOut>("Buzzer", tr("Buzzer"), ""
                              , this, &AudioOut::buzzer, &AudioOut::setBuzzer ),

        new DoubProp<AudioOut>("Impedance", tr("Impedance"), "Ω"
                              , this, &AudioOut::impedance, &AudioOut::setImpedance ),

        new DoubProp<AudioOut>("Frequency", tr("Frequency"), "Hz"
                              , this, &AudioOut::frequency, &AudioOut::setFrequency ),
    },0} );

    setPropStr( "Frequency", "1 kHz" );

    Simulator::self()->addToUpdateList( this );
    }
AudioOut::~AudioOut()
{
    if( m_audioOutput ) delete m_audioOutput;
}

void AudioOut::initialize()
{
    if( m_deviceinfo.isNull() ) return;

    m_dataBuffer.clear();
    m_audioOutput->stop();
    m_audioOutput->reset();

    Simulator::self()->cancelEvents( this );
}

void AudioOut::stamp()
{
    m_admit = m_buzzer ? 1e-4 : 1/m_impedance;
    eResistor::stamp();

    if( m_deviceinfo.isNull() ) return;

    m_audioBuffer = m_audioOutput->start();
    m_dataSize    = m_audioOutput->bytesFree();
    m_dataBuffer.reserve( m_dataSize );

    if( m_ePin[0]->isConnected() && m_ePin[1]->isConnected() )
            Simulator::self()->addEvent( 1, this );
}

void AudioOut::updateStep()
{
    if( !m_changed ) return;
    m_changed = false;

    double admit = m_buzzer ? 1e-4 : 1/m_impedance;
    setAdmit( admit );
}

void AudioOut::runEvent()
{
    double voltPN = m_ePin[0]->getVoltage()-m_ePin[1]->getVoltage();
    int outVal = 128;

    if( m_buzzer){
        if( voltPN > 2.5 )
        {
            double stepsPC = 1e12/m_frequency;
            double time = Simulator::self()->circTime();
            time = remainder( time, stepsPC );
            time = qDegreesToRadians( time*360/stepsPC );

            outVal += sin( time )*128;
    }   }
    else outVal += voltPN*51;

    if     ( outVal > 255 ) outVal = 255;
    else if( outVal < 0 )   outVal = 0;

    m_dataBuffer.append( (char)outVal );

    if( m_dataBuffer.size() == m_dataSize )
    {
        m_audioBuffer->write( m_dataBuffer.data(), m_dataSize );
        m_dataBuffer.clear();
    }
    double realSpeed = Simulator::self()->realSpeed();
    if( realSpeed < 1e-6 )
    {
        realSpeed = Simulator::self()->psPerSec();
        realSpeed /= 1e8;
    }
    realSpeed *= (1e12/10000);
    uint64_t nextEvent = realSpeed/m_format.sampleRate();
    Simulator::self()->addEvent( nextEvent, this ); // 25 us
}

void AudioOut::setImpedance( double i )
{
    m_impedance = i;
    m_changed = true;
    if( !Simulator::self()->isRunning() ) updateStep();
}

void AudioOut::setBuzzer( bool b )
{
    m_buzzer = b;
    m_changed = true;
    if( !Simulator::self()->isRunning() ) updateStep();
    updtProperties();
    update();
}

void AudioOut::slotProperties()
{
    Component::slotProperties();
    updtProperties();
}

void AudioOut::updtProperties()
{
    if( !m_propDialog ) return;
    m_propDialog->showProp("Frequency", m_buzzer );
    m_propDialog->showProp("Impedance", !m_buzzer );
    m_propDialog->adjustWidgets();
}

QPainterPath AudioOut::shape() const
{
    QPainterPath path;
    
    QVector<QPointF> points;
    points << QPointF(-10,-12 )
           << QPointF(-10, 4 )
           << QPointF( 0, 4 )
           << QPointF( 10, 16 )
           << QPointF( 10, -24 )
           << QPointF( 0, -12 )
           << QPointF(-10, -12 );
        
    path.addPolygon( QPolygonF(points) );
    path.closeSubpath();
    return path;
}

void AudioOut::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );

    if( m_buzzer)
    {
        p->drawChord( QRectF(-10,-24, 40, 40 ),-90*16,-180*16 );
    }
    else{
        static const QPointF points[7] = {
           QPointF(-10,-12 ),
           QPointF(-10,  4 ),
           QPointF(  0,  4 ),
           QPointF( 10, 16 ),
           QPointF( 10,-24 ),
           QPointF(  0,-12 ),
           QPointF(-10,-12 )
       };
       p->drawPolygon(points, 7);
    }

    if( m_deviceinfo.isNull() ){
        p->drawLine(0,-8, 7, 0 );
        p->drawLine( 7,-8,0, 0 );
    }
    Component::paintSelected( p );
}
