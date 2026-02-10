/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>

#include <QtMath>

#include "op_amp.h"
#include "itemlibrary.h"
#include "propdialog.h"
#include "connector.h"
#include "simulator.h"
#include "e-node.h"
#include "iopin.h"

#include "doubleprop.h"
#include "boolprop.h"

#define tr(str) simulideTr("OpAmp",str)

Component* OpAmp::construct( QString type, QString id )
{ return new OpAmp( type, id ); }

LibraryItem* OpAmp::libraryItem()
{
    return new LibraryItem(
        tr("OpAmp"),
        "Other Active",
        "opamp.png",
        "opAmp",
        OpAmp::construct );
}

OpAmp::OpAmp( QString type, QString id )
     : Component( type, id )
     , eElement( id )
{
    m_area = QRect(-18, -8*2, 36, 8*2*2 );
    setLabelPos(-16,-32, 0);

    m_pin.resize( 5 );
    m_pin[0] = m_inputP = new IoPin( 180, QPoint(-16-8,-8), id+"-inputNinv", 0, this, input );
    m_pin[0]->setFontSize( 9 );
    m_pin[0]->setSpace( 1.7 );
    m_pin[0]->setLabelText("+");
    m_pin[0]->setLabelColor( Qt::red );

    m_pin[1] = m_inputN = new IoPin( 180, QPoint(-16-8, 8), id+"-inputInv", 1, this, input );
    m_pin[1]->setFontSize( 9 );
    m_pin[1]->setSpace( 1.7 );
    m_pin[1]->setLabelText("–");  // U+2013
    m_pin[1]->setLabelColor( QColor( 0, 0, 0 ) );

    m_output = new IoPin( 0, QPoint( 16+8, 0), id+"-output",   2, this, source );
    m_pin[2] = m_output;

    m_pin[3] = new Pin( 90, QPoint(0,-16), id+"-powerPos", 3, this );
    m_pin[3]->setFontSize( 9 );
    m_pin[3]->setSpace( 1.7 );
    m_pin[3]->setLabelText("+");
    m_pin[3]->setLabelColor( Qt::red );

    m_pin[4] = new Pin( 270, QPoint(0, 16), id+"-powerNeg", 4, this );
    m_pin[4]->setFontSize( 9 );
    m_pin[4]->setSpace( 1.7 );
    m_pin[4]->setLabelText("–");  // U+2013
    m_pin[4]->setLabelColor( QColor( 0, 0, 0 ) );
    
    m_switchPins = false;
    setPowerPins( false );

    m_gain = 1000;
    m_outImp = low_imp;
    m_voltPosDef = 5;
    m_voltNegDef = 0;

    addPropGroup( { tr("Main"), {
        new DoubProp<OpAmp>("Gain", tr("Gain"),""
                           , this, &OpAmp::gain, &OpAmp::setGain ),

        new DoubProp<OpAmp>("Out_Imped", tr("Output Impedance"),"Ω"
                           , this, &OpAmp::outImp, &OpAmp::setOutImp )
    },0 } );

    addPropGroup( { tr("Supply"), {
        new BoolProp<OpAmp>("Power_Pins", tr("Use Supply Pins"),""
                           , this, &OpAmp::powerPins, &OpAmp::setPowerPins, propNoCopy ),

        new BoolProp<OpAmp>("Switch_Pins", tr("Switch Supply Pins"),""
                           , this, &OpAmp::switchPins, &OpAmp::setSwitchPins, propNoCopy ),

        new DoubProp<OpAmp>("Volt_Pos", tr("V+"),"V"
                           , this, &OpAmp::voltPos,&OpAmp::setVoltPos ),

        new DoubProp<OpAmp>("Volt_Neg", tr("V-"),"V"
                           , this, &OpAmp::voltNeg, &OpAmp::setVoltNeg ),
    },0 } );
}
OpAmp::~OpAmp(){}

void OpAmp::initialize()
{
    m_accuracy = 5e-6;

    added = false;
    m_step = 0;
    m_lastOut = 0;
    m_lastIn  = 0;
    m_k = 1e-6/m_gain;
}

void OpAmp::stamp()
{
    if( m_inputP->isConnected() ) m_inputP->getEnode()->addToNoLinList(this);
    if( m_inputN->isConnected() ) m_inputN->getEnode()->addToNoLinList(this);
    if( m_output->isConnected() )
    {
        m_output->getEnode()->addToNoLinList(this);
        m_output->createCurrent();
    }
}

void OpAmp::updateStep()
{
    if( !m_changed ) return;
    m_changed = false;

    m_output->setImpedance( m_outImp );
    voltChanged();
}

void OpAmp::voltChanged() // Called when any pin node change volt
{
    if( m_powerPins )
    {
        m_voltPos = m_pin[3]->getVoltage();
        m_voltNeg = m_pin[4]->getVoltage();
    } else {
        m_voltPos = m_voltPosDef;
        m_voltNeg = m_voltNegDef;
    }
    double vd = m_inputP->getVoltage()-m_inputN->getVoltage();

    double out = vd * m_gain;
    if     ( out > m_voltPos ) out = m_voltPos;
    else if( out < m_voltNeg ) out = m_voltNeg;

    if( (m_step==0)
     && (qFabs(m_lastIn-vd) < m_accuracy )
     && (qFabs(out-m_lastOut) < m_accuracy) )
    { m_step = 0; return; }         // Converged

    Simulator::self()->notCorverged();

    if( m_step==0 )                  // First step after a convergence
    {
        double dOut = -1e-6;         // Do a tiny step to se what happens
        if( vd>0 ) dOut = 1e-6;

        out = m_lastOut + dOut;
        m_step = 1;
    } else {
        if( m_lastIn != vd ) // We problably are in a close loop configuration
        {
            double dIn  = qFabs(m_lastIn-vd); // Input diff with last step
            out = (m_lastOut*dIn + vd*1e-6)/(dIn + m_k); // Guess next converging output:
        }
        m_step = 0;
    }
    if     ( out >= m_voltPos ) out = m_voltPos;
    else if( out <= m_voltNeg ) out = m_voltNeg;

    m_lastIn  = vd;
    m_lastOut = out;

    m_output->stampCurrent( out/m_outImp );
}

void OpAmp::setOutImp( double imp )
{
    if( imp < low_imp ) imp = low_imp;
    m_outImp = imp;
    m_changed = true;
    if( !Simulator::self()->isRunning() ) updateStep();
}

void OpAmp::setPowerPins( bool set )
{
    m_pin[3]->setEnabled( set );
    m_pin[3]->setVisible( set );
    m_pin[4]->setEnabled( set );
    m_pin[4]->setVisible( set );
    
    if( !set )
    {
        m_pin[3]->removeConnector();
        m_pin[4]->removeConnector();
    }
    m_powerPins = set;
    updtProperties();
}

void OpAmp::setSwitchPins( bool s )
{
    m_switchPins = s;
    int angleP = s ? 270 :  90;
    int angleN = s ?  90 : 270;
    qreal yP   = s ?  16 : -16;
    qreal yN   = s ? -16 :  16;

    m_pin[3]->setPinAngle( angleP );
    m_pin[3]->setY( yP );
    m_pin[4]->setPinAngle( angleN );
    m_pin[4]->setY( yN );
}

void OpAmp::updtProperties()
{
    if( !m_propDialog ) return;
    m_propDialog->showProp("Volt_Pos", !m_powerPins );
    m_propDialog->showProp("Volt_Neg", !m_powerPins );
    m_propDialog->showProp("Switch_Pins", m_powerPins );
    m_propDialog->adjustWidgets();
}

void OpAmp::slotProperties()
{
    Component::slotProperties();
    updtProperties();
}

QPainterPath OpAmp::shape() const
{
    QPainterPath path;
    
    QVector<QPointF> points;
    
    points << QPointF(-16,-16 )
           << QPointF(-16, 16 )
           << QPointF( 16,  1 )
           << QPointF( 16, -1 );
        
    path.addPolygon( QPolygonF(points) );
    path.closeSubpath();
    return path;
}

void OpAmp::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );

    QPen pen = p->pen();
    pen.setWidth(2);
    p->setPen(pen);

 static const QPointF points[4] = {
        QPointF(-16,-16 ),
        QPointF(-16, 16 ),
        QPointF( 16,  1 ),
        QPointF( 16, -1 )            };

    p->drawPolygon(points, 4);

    Component::paintSelected( p );
}
