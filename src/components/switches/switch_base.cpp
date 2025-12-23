/***************************************************************************
 *   Copyright (C) 2016 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>
#include <QGraphicsProxyWidget>

#include "switch_base.h"
#include "simulator.h"
#include "circuit.h"
#include "label.h"
#include "custombutton.h"

SwitchBase::SwitchBase( QString type, QString id )
          : MechContact( type, id )
{
    m_graphical = true;
    m_changed = true;
    m_key = "";
    m_condition = "";

    m_area =  QRectF( 0,0,0,0 );
    m_ePin.resize(2);
    m_idLabel->setPos(-12,-24);

    m_button = new CustomButton( );
    m_button->setMaximumSize( 16, 16 );
    m_button->setCheckable( true );

    m_proxy = Circuit::self()->addWidget( m_button );
    m_proxy->setParentItem( this );
    m_proxy->setPos(-20,-16 );
    m_proxy->setTransformOriginPoint( m_proxy->boundingRect().center() );
    m_keyEventConn = QObject::connect( Circuit::self(), &Circuit::keyEvent, [=](QString k, bool p){ keyEvent(k,p); } );
}
SwitchBase::~SwitchBase(){
    QObject::disconnect( m_keyEventConn );
}

void SwitchBase::updateStep()
{
    if( m_changed )
    {
        setSwitch( m_closed );
        m_changed = false;
        update();
}   }

void SwitchBase::setAngle( double angle )
{
    SwitchBase::rotateAngle( angle );
    Component::setAngle( angle );
}

void SwitchBase::rotateAngle( double a )
{
    Component::rotateAngle( a );
    rotateText(-a );
}

void SwitchBase::rotateText( double angle )
{
    double rot = m_proxy->rotation();
    m_proxy->setRotation( rot + angle );
}

/*void SwitchBase::setflip()
{
    Component::setflip();
    m_proxy->setPos( -8 + ( m_Hflip>0 ? 0 : 16 ), 4 + ( m_Vflip>0 ? 0 : 16 ) );
    m_proxy->setTransform( QTransform::fromScale( m_Hflip, m_Vflip ) );
}*/

void SwitchBase::setHidden( bool hide, bool hidArea, bool hidLabel )
{
    Component::setHidden( hide, hidArea, hidLabel );
    if  ( hidArea ) m_area = QRectF( 0, 0,-4,-4 );
    else if( hide ) m_area = QRectF(-6, 6, 12, 12 );
    else            m_area = QRectF(-13,-16*m_numPoles, 26, 16*m_numPoles );

    m_proxy->setFlag( QGraphicsItem::ItemStacksBehindParent, hide && !hidArea );
}

void SwitchBase::onbuttonclicked()
{
    m_closed = m_button->isChecked();

    if( m_nClose ) m_closed = !m_closed;
    m_changed = true;
    update();
}

void SwitchBase::setKey( QString key )
{
    if( key.startsWith("&#x3") ) key.replace("&#x3D", "=").replace("&#x3C", "<").replace("&#x3E", ">");
    if( key.size()>1 ) key = key.left( 1 );
    m_key = key;
    m_button->setText( key );
}

void SwitchBase::setLinkCondition( QString condition )
{
    m_condition = condition;
}
