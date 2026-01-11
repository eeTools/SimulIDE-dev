/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>
#include <math.h>
#include <QMenu>
#include <QGraphicsProxyWidget>

#include "dcmotor.h"
#include "itemlibrary.h"
#include "simulator.h"
#include "circuit.h"
#include "pin.h"
#include "label.h"
#include "watcher.h"

#include "doubleprop.h"
#include "intprop.h"

#define tr(str) simulideTr("DcMotor",str)

Component* DcMotor::construct( QString type, QString id )
{ return new DcMotor( type, id ); }

LibraryItem* DcMotor::libraryItem()
{
    return new LibraryItem(
        tr("Dc Motor"),
        "Motors",
        "dcmotor.png",
        "DcMotor",
        DcMotor::construct );
}

DcMotor::DcMotor( QString type, QString id )
       : LinkerComponent( type, id )
       , eResistor( id )
       , Watched()
{
    m_graphical = true;
    
    m_area = QRectF( -35,-33, 70, 66 );
    m_color = QColor( 50, 50, 70 );
    m_ang  = 0;
    m_speed = 0;
    m_voltNom = 5;
    setRpm( 60 );

    m_pin.resize( 2 );
    m_ePin[0] = m_pin[0] = new Pin( 180, QPoint(-40,0), m_id+"-lPin", 0, this);
    m_pin[0]->setLength( 4 );
    m_pin[0]->setFontSize( 9 );
    m_pin[0]->setSpace( 1.7 );
    m_pin[0]->setLabelText("+");

    m_ePin[1] =m_pin[1] = new Pin( 0, QPoint(40,0), m_id+"-rPin", 1, this);
    m_pin[1]->setLength( 4 );
    m_pin[1]->setFontSize( 9 );
    m_pin[1]->setSpace( 1.7 );
    m_pin[1]->setLabelText("–");  // U+2013

    setShowId( true );
    setLabelPos(-22,-48, 0);
    setValLabelPos(-14, 36, 0);

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"), {
        new IntProp <DcMotor>("RPM_Nominal", tr("Nominal Speed"), "_RPM"
                             , this, &DcMotor::rpm, &DcMotor::setRpm ),

        new DoubProp<DcMotor>("Volt_Nominal", tr("Nominal Voltage"), "V"
                             , this, &DcMotor::volt, &DcMotor::setVolt ),

        new DoubProp<DcMotor>("Resistance", tr("Resistance"), "Ω"
                             , this, &DcMotor::resistance, &DcMotor::setResSafe )
    },0} );
}
DcMotor::~DcMotor(){}

QString DcMotor::getPropStr( QString prop )
{
    if( prop.toLower() == "speed" ) // To be called from scripts
    {
        return QString::number( -m_speed );  // Speed: 1 = 100%
    }
    else return LinkerComponent::getPropStr( prop );
}

void DcMotor::initialize()
{
    m_ang = 0;
    m_speed = 0;
    m_delta = 0;
    m_lastTime = 0;
    m_updtTime = 0;
    m_LastVolt = 0;
}

void DcMotor::stamp()
{
     if( m_ePin[0]->isConnected() && m_ePin[1]->isConnected() )
     {
         m_ePin[0]->changeCallBack( this );
         m_ePin[1]->changeCallBack( this );
     }
     eResistor::stamp();
}

void DcMotor::updateStep()
{
    updatePos();

    if( m_updtTime ) m_speed = m_delta/(m_updtTime/1e12);

    m_ang += m_motStPs*m_delta;
    m_ang = remainder( m_ang, (16.0*360.0) );

    if(  m_linkedComp.size() )
    {
        double val = m_ang*1000/(16.0*360.0);
        if( val > 0 ) val = 1000-val;
        else          val = -val;
        for( Component* comp : m_linkedComp ) comp->setLinkedValue( val ); // angle 0-1000
    }
    if( m_watcher ) m_watcher->updateValues();

    m_delta = 0;
    m_updtTime = 0;
    update();
}

void DcMotor::voltChanged() { updatePos(); }

void DcMotor::updatePos()
{
    uint64_t timePS = Simulator::self()->circTime();
    uint64_t duration = timePS-m_lastTime;
    m_updtTime += duration;
    m_lastTime = timePS;

    m_delta += (m_LastVolt/m_voltNom)*(duration/1e12);
    m_LastVolt = m_ePin[1]->getVoltage() - m_ePin[0]->getVoltage();
}

void DcMotor::setRpm( int rpm )
{
    if( rpm < 1 ) rpm = 1;
    m_rpm = rpm;
    m_motStPs = 16*360*rpm/60;

    update();
}

void DcMotor::openWatcher()
{
    if( !m_watcher )
    {
        createWatcher( true );

        QGraphicsProxyWidget* proxy = Circuit::self()->addWidget( m_watcher );
        proxy->setParentItem( this );
        proxy->setPos(-65,40 );
        //proxy->setTransformOriginPoint( proxy->boundingRect().center() );
        m_watcher->setProxy( proxy );

        m_watcher->addRegister( "Voltage" , "double", "V" );
        m_watcher->addRegister( "Current" , "double", "A" );
        m_watcher->addRegister( "Speed"   , "double", "RPM" );
        m_watcher->setWindowTitle( this->idLabel() );
    }
    m_watcher->show();
}

double DcMotor::getDblReg( QString reg )
{
    if( reg == "Voltage" ) return -m_LastVolt;
    if( reg == "Current" ) return eResistor::current();
    if( reg == "Speed"   ) return -m_speed*m_rpm;

    return 0;
}

void DcMotor::contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu )
{
    QAction* watcherAction = menu->addAction( QIcon(":/terminal.svg"),tr("Open Monitor") );
    QObject::connect( watcherAction, &QAction::triggered, [=](){ openWatcher(); } );

    menu->addSeparator();

    LinkerComponent::contextMenu( event, menu );
}

void DcMotor::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );

    p->setBrush( QColor(50, 70, 100) );
    p->drawEllipse(-32,-32, 64, 64 );

    p->setBrush( QColor(255, 0, 0) );
    p->drawRoundedRect(-36,-4, 8, 8, 4, 4 );
    p->setBrush( QColor(0, 0, 0) );
    p->drawRoundedRect( 28,-4, 8, 8, 4, 4 );

    p->setPen( QColor(0, 0, 0) );
    p->setBrush( QColor(255, 255, 255) );
    p->drawEllipse(-26,-26, 52, 52 );

    // rotating pointer
    p->setPen ( QColor(0, 0, 0) );
    p->setBrush( QColor(50, 70, 100) );
    p->drawPie(-24,-24, 48, 48, m_ang-120, 240 );

    p->setBrush( QColor(50, 70, 100) );
    p->drawEllipse(-20,-20, 40, 40 );

    if( m_speed != 0 ) // Speed and Direction Indicator
    {
        double speed = m_speed;
        double exedd = 0;
        if( m_speed > 1 ){
            exedd = m_speed -1;
            speed = 1;
        }
        else if( m_speed < -1 ){
            exedd = m_speed +1;
            speed = -1;
        }
        p->setPen ( QColor(50, 70, 100) );
        p->setBrush( QColor(100, 200, 70) );
        p->drawPie(-20,-20, 40, 40, 16*90, speed*16*180 );

        p->setBrush( QColor(200, 100, 70) );
        p->drawPie(-20,-20, 40, 40, -16*90, exedd*16*180 );
    }
    Component::paintSelected( p );
}
