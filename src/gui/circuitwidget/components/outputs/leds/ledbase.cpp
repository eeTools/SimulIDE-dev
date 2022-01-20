/***************************************************************************
 *   Copyright (C) 2012 by santiago González                               *
 *   santigoro@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#include "ledbase.h"
#include "circuitwidget.h"
#include "connector.h"
#include "simulator.h"
#include "e-node.h"
#include "pin.h"

#include "stringprop.h"
#include "doubleprop.h"
#include "boolprop.h"

QStringList LedBase::m_colors = {"Yellow","Red","Green","Blue","Oreange","Purple"};

LedBase::LedBase( QObject* parent, QString type, QString id )
       : Component( parent, type, id )
       , eLed( id )
{
    m_graphical = true;
    m_grounded  = false;
    m_scrEnode  = NULL;
    m_intensity = 0;
    //m_diodeType = "led";

    m_color = QColor( Qt::black );
    setColorStr("Yellow");

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"), {
//new StringProp<LedBase>( "Model"   , tr("Model")   ,"", this, &LedBase::model,    &LedBase::setModel, "enum" ),
new StringProp<LedBase>( "Color"   , tr("Color")   ,"", this, &LedBase::colorStr, &LedBase::setColorStr, "enum" ),
new BoolProp  <LedBase>( "Grounded", tr("Grounded"),"", this, &LedBase::grounded, &LedBase::setGrounded),
    }} );
    addPropGroup( { tr("Electric"), {
new DoubProp<LedBase>( "Threshold" , tr("Forward Voltage"),"V", this, &LedBase::threshold,  &LedBase::setThreshold ),
new DoubProp<LedBase>( "MaxCurrent", tr("Max Current")    ,"A", this, &LedBase::maxCurrent, &LedBase::setMaxCurrent ),
new DoubProp<LedBase>( "Resistance", tr("Resistance")     ,"Ω", this, &LedBase::res,        &LedBase::setRes ),
    }} );
/*    addPropGroup( { tr("Advanced"), {
new DoubProp<LedBase>( "BrkDownV"  , tr("Breakdown Voltage")   ,"V" , this, &LedBase::brkDownV, &LedBase::setBrkDownV ),
new DoubProp<LedBase>( "SatCurrent", tr("Saturation Current")  ,"nA", this, &LedBase::satCur,   &LedBase::setSatCur ),
new DoubProp<LedBase>( "EmCoef"    , tr("Emission Coefficient"),""  , this, &LedBase::emCoef,   &LedBase::setEmCoef ),
    }} );*/
}
LedBase::~LedBase()
{
    if( m_grounded ) m_pin[1]->setEnode( NULL );
}

void LedBase::initialize()
{
    m_crashed = false;
    m_warning = false;
    eLed::initialize();
    update();
}

void LedBase::updateStep()
{
    uint32_t intensity = m_intensity;
    eLed::updateBright();

    if( overCurrent() > 1.5 )
    {
        m_warning = true;
        m_crashed = overCurrent() > 2;
        update();
    }else{
        if( m_warning ) update();
        m_warning = false;
        m_crashed = false;
    }
    if( intensity != m_intensity ) update();
}

void LedBase::setGrounded( bool grounded )
{
    if( grounded == m_grounded ) return;

    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    if( grounded )
    {
        Pin* pin1 = static_cast<Pin*>(m_ePin[1]);
        pin1->removeConnector();
        pin1->setEnabled( false );
        pin1->setVisible( false );

        m_scrEnode = new eNode( m_id+"Gnod" );
        m_scrEnode->setNodeNumber( 0 );
        Simulator::self()->remFromEnodeList( m_scrEnode, /*delete=*/ false );
        m_ePin[1]->setEnode( m_scrEnode );
    }else{
        Pin* pin1 = static_cast<Pin*>(m_ePin[1]);
        pin1->setEnabled( true );
        pin1->setVisible( true );
        m_ePin[1]->setEnode( NULL );
    }
    m_grounded = grounded;
}

void LedBase::setColorStr( QString c )
{
    bool ok = false;
    int index = c.toInt( &ok );
    if( !ok ) index = m_colors.indexOf( c );
    m_ledColor = (LedColor)index;
}

QStringList LedBase::getEnums( QString e )
{
    if( e == "Color" ) return m_colors;
    else return CompBase::getEnums( e );
}

void LedBase::paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    Component::paint( p, option, widget );

    QPen pen(Qt::black, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QColor color;

    if( m_warning/*m_current > m_maxCurrent*1.2*/ ) // Led overcurrent
    {
        p->setBrush( QColor( 255, 150, 0 ) );
        color = QColor( Qt::red );
        pen.setColor( color );
    }
    if( m_crashed )  // Led extreme overcurrent
    {
        p->setBrush( Qt::white );
        color = QColor( Qt::white );
        pen.setColor( color );
    }else{
        int overBight = 100;
        
        if( m_intensity > 25 )
        {
            m_intensity += 15;       // Set a Minimun Bright
            if( m_intensity > 255 )
            {
                overBight += m_intensity-255;
                m_intensity = 255;
        }   }
        if     ( m_ledColor == yellow ) color = QColor( m_intensity, m_intensity,     overBight*2/3 );
        else if( m_ledColor == red    ) color = QColor( m_intensity, m_intensity/4+overBight/2, overBight/2 );
        else if( m_ledColor == green  ) color = QColor( overBight,   m_intensity,     m_intensity*2/3 );
        else if( m_ledColor == blue   ) color = QColor( overBight,   m_intensity*2/3, m_intensity );
        else if( m_ledColor == orange ) color = QColor( m_intensity, m_intensity*2/3, overBight );
        else if( m_ledColor == purple ) color = QColor( m_intensity, m_intensity/4+overBight/2, m_intensity );
    }
    p->setPen( pen );
    drawBackground( p );
    
    pen.setColor( color );
    pen.setWidth(2.5);
    p->setPen(pen);
    p->setBrush( color );

    drawForeground( p );
}
