/***************************************************************************
 *   Copyright (C) 2019 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>
#include <QGraphicsProxyWidget>

#include "sr04.h"
#include "iopin.h"
#include "itemlibrary.h"
#include "circuitwidget.h"
#include "simulator.h"
#include "circuit.h"
#include "mainwindow.h"
#include "customslider.h"

#include "boolprop.h"

#define tr(str) simulideTr("SR04",str)

Component* SR04::construct( QString type, QString id )
{ return new SR04( type, id ); }

LibraryItem* SR04::libraryItem()
{
    return new LibraryItem(
        "HC-SR04",
        "Sensors",
        "sr04_ico.png",
        "SR04",
        SR04::construct);
}

SR04::SR04( QString type, QString id )
    : Component( type, id )
    , eElement( id )
{
    m_graphical = true;

    m_area = QRect(-10*8,-4*8, 21*8, 9*8 );
    setBackground("sr04.svg");
    setLabelPos(-16,-48, 0);

    m_pin.resize(5);

    m_pin[0] = m_inpin = new Pin( 180, QPoint(-11*8,-3*8), id+"-inpin", 0, this );
    m_inpin->setLabelText( " In v=m" );

    Pin* vccPin = new Pin( 270, QPoint(-8,48), id+"-vccpin", 0, this );
    vccPin->setLabelText( " Vcc" );
    vccPin->setUnused( true );
    m_pin[1] = vccPin;

    Pin* gndPin = new Pin( 270, QPoint(16,48), id+"-gndpin", 0, this );
    gndPin->setLabelText( " Gnd" );
    gndPin->setUnused( true );
    m_pin[2] = gndPin;

    m_pin[3] =m_trigpin = new Pin( 270, QPoint(0,48), id+"-trigpin", 0, this );
    m_trigpin->setLabelText( " Trig" );
    
    m_pin[4] = m_echo = new IoPin( 270, QPoint(8,48), id+"-outpin", 0, this, output );
    m_echo->setLabelText( " Echo" );
    m_echo->setOutHighV( 5 );

    m_slider = new CustomSlider();
    m_slider->setFixedSize( 180, 12 );
    m_slider->setSingleStep( 2 );
    m_slider->setVisible( false );
    m_slider->setValue( 0 );
    m_useDial = false;
    m_distance = 0;

    QGraphicsProxyWidget* proxy = Circuit::self()->addWidget( m_slider );
    proxy->setParentItem( this );
    proxy->setPos(-86,-44 );

    QObject::connect( m_slider, &QAbstractSlider::valueChanged, [=](int v){ dialChanged(v); } );

    Simulator::self()->addToUpdateList( this );

    SR04::initialize();

    addPropGroup( { tr("Main"), {
        new BoolProp<SR04>("Slider", tr("Use slider"),""
                              , this, &SR04::slider, &SR04::setSlider ),
    },0} );
}
SR04::~SR04(){}

void SR04::stamp()
{
    m_trigpin->changeCallBack( this ); // Register for Trigger Pin changes
}

void SR04::initialize()
{
    m_lastStep = Simulator::self()->circTime();
    m_lastTrig = false;
    m_echouS = 0;
}

void SR04::updateStep()
{
    if( !m_useDial ) m_distance = m_inpin->getVoltage();
    update();
}

void SR04::voltChanged()              // Called when Trigger Pin changes
{
    bool trigState = m_trigpin->getVoltage()>2.5;
    
    if( !m_lastTrig && trigState )                 // Rising trigger Pin
    {
        m_lastStep = Simulator::self()->circTime();
    }
    else if( m_lastTrig && !trigState )            // Triggered
    {
        uint64_t time = Simulator::self()->circTime()-m_lastStep; // in picosecondss

        if( time >= 10*1e6 )     // >=10 uS Trigger pulse
        {
            if( !m_useDial ) m_distance = m_inpin->getVoltage();

            m_echouS = (m_distance*2000/0.344+0.5);
            if     ( m_echouS < 116 )   m_echouS = 116;   // Min range 2 cm = 116 us pulse
            else if( m_echouS > 38000 ) m_echouS = 38000; // Timeout 38 ms
            
            Simulator::self()->addEvent( 200*1e6, this ); // Send echo after 200 us
        }
    }
    m_lastTrig = trigState;
}

void SR04::runEvent()
{
    if( m_echouS )
    {
        m_echo->scheduleState( true, 0 );
        Simulator::self()->addEvent( m_echouS*1e6, this ); // Event to finish echo
        m_echouS = 0;
    }
    else m_echo->scheduleState( false, 0 );
}

void SR04::dialChanged( int value )
{
    m_distance = double(value)/250;
}

void SR04::setSlider( bool s )
{
    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();
    m_useDial = s;

    m_slider->setVisible( s );
    m_inpin->setVisible( !s );
    if( m_useDial ) m_inpin->removeConnector();
}

void SR04::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );
    p->setRenderHint( QPainter::SmoothPixmapTransform );
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);

    p->drawRoundedRect( m_area, 2, 2 );

    p->drawPixmap( m_area, *m_backPixmap, m_backPixmap->rect() );

    p->setPen( Qt::white );
    QFont font = p->font();
    font.setPixelSize( 9 );
    p->setFont( font );
    p->drawText( QRectF(-14,-32, 40, 12 ), Qt::AlignCenter, QString::number( m_distance, 'f', 3 )+" m" );

    Component::paintSelected( p );
}
