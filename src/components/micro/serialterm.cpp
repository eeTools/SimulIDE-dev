/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QMenu>

#include "serialterm.h"
#include "serialmon.h"
#include "terminal.h"
#include "itemlibrary.h"
#include "simulator.h"
#include "circuit.h"
#include "usarttx.h"
#include "usartrx.h"
#include "iopin.h"
#include "utils.h"
#include "mainwindow.h"
#include "custombutton.h"

#include "stringprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("SerialTerm",str)

Component* SerialTerm::construct( QString type, QString id )
{ return new SerialTerm( type, id ); }

LibraryItem* SerialTerm::libraryItem()
{
    return new LibraryItem(
        tr("Serial Terminal"),
        "Peripherals",
        "serialterm.png",
        "SerialTerm",
        SerialTerm::construct );
}

SerialTerm::SerialTerm( QString type, QString id )
          : Component( type, id )
          , UsartModule( nullptr, id+"-Uart" )
          , eElement( (id+"-eElement") )
{
    m_graphical = true;

    m_area = QRect(-16,-16, 72, 32 );
    setLabelPos(-20,-32 );

    m_pin.resize(2);

    IoPin* pinTx = new IoPin( 180, QPoint(-24,-8), id+"-pin0", 0, this, output );
    pinTx->setLabelText( "Tx" );
    pinTx->setOutHighV( 5 );
    m_pin[0] = pinTx;
    m_sender->setPins( {pinTx} );
    m_sending = false;

    IoPin* pinRx = new IoPin( 180, QPoint(-24, 8), id+"-pin1", 0, this, input );
    pinRx->setLabelText( "Rx" );
    m_pin[1] = pinRx;
    m_receiver->setPins( {pinRx} );
    m_receiving = false;

    m_button = new CustomButton( );
    m_button->setMaximumSize( 44, 20 );
    m_button->setGeometry(-44,-20, 44, 20 );
    m_button->setCheckable( true );
    m_button->setText( tr("Open") );

    QFont font = m_button->font();
    font.setFamily( MainWindow::self()->defaultFontName() );
    font.setPixelSize(12);
    m_button->setFont( font );
    QObject::connect( m_button, &CustomButton::clicked, [=](){ onbuttonclicked(); });

    m_proxy = Circuit::self()->addWidget( m_button );
    m_proxy->setParentItem( this );
    m_proxy->setPos( QPoint( 8,-10) );

    m_terminal = new Terminal( MainWindow::self() );
    m_terminal->setWindowTitle( id );
    QObject::connect( m_terminal, &Terminal::sendBytes, [=](QByteArray d){ sendByteArray( d ); } );
    QObject::connect( m_terminal, &Terminal::closed   , [=](){ TerminalClosed(); } );

    setBaudRate( 9600 );

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"), {
//new BoolProp  <Chip>( "Logic_Symbol","","", this, &Chip::logicSymbol, &Chip::setLogicSymbol ),
    }, 0 } );

    addPropGroup( { tr("Config"), {
        new IntProp<SerialTerm>("Baudrate", tr("Baudrate"),"_Bd"
                               , this, &SerialTerm::baudRate, &SerialTerm::setBaudRate,0,"uint" ),

        new IntProp<SerialTerm>("DataBits", tr("Data Bits"),"_bits"
                               , this, &SerialTerm::dataBits, &SerialTerm::setDataBits,0,"uint" ),

        new IntProp<SerialTerm>("StopBits", tr("Stop Bits"),"_bits"
                               , this, &SerialTerm::stopBits, &SerialTerm::setStopBits,0,"uint" ),
    }, 0 } );

    addPropGroup( {"Hidden", {
        new BoolProp<SerialTerm>("SerialMon","",""
                                , this, &SerialTerm::serialMon, &SerialTerm::setSerialMon ),
    }, groupHidden} );
}
SerialTerm::~SerialTerm()
{
    m_terminal->close();
}

void SerialTerm::stamp()
{
    m_uartData.clear();
    m_sender->enable( true );
    m_receiver->enable( true );
    m_sending   = false;
    m_receiving = false;
}

void SerialTerm::updateStep()
{
    if( m_uartData.size() ){
        if( !m_sending ){
            m_sending = true;
            Simulator::self()->addEvent( 1, this );
        }
    } else m_sending = false;

    update();
}

void SerialTerm::runEvent()
{
    if( m_uartData.isEmpty() ) return;
    UsartModule::sendByte( m_uartData.at( 0 ) ); // Start transaction
    m_uartData = m_uartData.right( m_uartData.size()-1 );
}

void SerialTerm::sendByteArray( QByteArray data )
{
    m_uartData += data;
}

void SerialTerm::sendByte( uint8_t data )
{
    m_uartData += data;
}

void SerialTerm::byteReceived( uint8_t byte )
{
    m_receiver->getData();
    if( m_monitor ) m_monitor->printIn( byte );
    m_terminal->received( byte );
    m_receiving = true;
}

void SerialTerm::frameSent( uint8_t data )
{
    if( m_monitor ) m_monitor->printOut( data );
    if( m_uartData.size() )
    {
        uint8_t byte = m_uartData.at( 0 );
        m_uartData = m_uartData.right( m_uartData.size()-1 );
        UsartModule::sendByte( byte );
    }
    else m_sending = false;
}

void SerialTerm::onbuttonclicked()
{
    if( m_button->isChecked() ){
        m_terminal->show(); //slotOpenTerm();
        m_button->setText( tr("Close") );
    }else{
        m_terminal->hide(); //m_monitor->close();
        m_button->setText( tr("Open") );
    }
}

void SerialTerm::setIdLabel( QString id )
{
    Component::setIdLabel( id );
    if( m_monitor ) m_monitor->setWindowTitle( id );
    m_terminal->setWindowTitle( id );
}

void SerialTerm::slotOpenTerm()
{
    UsartModule::openMonitor( idLabel(), 0, /*send=*/false );
    //m_monitor->activateSend();
}

void SerialTerm::setSerialMon( bool s )
{
    if( s ) m_terminal->show(); //slotOpenTerm();
}

void SerialTerm::monitorClosed()
{
    UsartModule::monitorClosed();
    //m_button->setChecked( false );
}

void SerialTerm::TerminalClosed()
{
    m_button->setChecked( false );
    m_button->setText( tr("Open") );
}

void SerialTerm::setflip()
{
    Component::setflip();
    m_proxy->setPos( QPoint( 8 + ( m_Hflip>0 ? 0 : m_button->width() ),-10 + ( m_Vflip>0 ? 0 : m_button->height() ) ) );
    m_proxy->setTransform( QTransform::fromScale( m_Hflip, m_Vflip ) );
}

void SerialTerm::contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu )
{
    QAction* openSerMon = menu->addAction( QIcon(":/terminal.svg"),tr("Open Serial Monitor.") );
    QObject::connect( openSerMon, &QAction::triggered, [=](){ slotOpenTerm(); } );

    menu->addSeparator();
    Component::contextMenu( event, menu );
}

void SerialTerm::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );
    
    p->setBrush( Qt::darkBlue );
    p->drawRoundedRect( m_area, 4, 4 );

    if( m_pin[0]->connector() )
    {
        if( m_sending ) p->setBrush( Qt::yellow );
        else            p->setBrush( Qt::red );
    }
    else p->setBrush( Qt::black );
    p->drawRoundedRect( -5,-11, 8, 6, 2, 2 ); // Tx led

    if( m_pin[1]->connector() )
    {
        if( m_receiving ){
            m_receiving = false;
            p->setBrush( Qt::yellow );
        }else p->setBrush( Qt::red );
    }
    else p->setBrush( Qt::black );
    p->drawRoundedRect(-5, 5, 8, 6, 2, 2 ); // Rx led

    Component::paintSelected( p );
}
