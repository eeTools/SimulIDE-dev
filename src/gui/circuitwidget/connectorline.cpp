/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <math.h>
#include <QToolTip>

#include "connectorline.h"
#include "connector.h"
#include "circuitview.h"
#include "circuitwidget.h"
#include "simulator.h"
#include "circuit.h"
#include "node.h"
#include "currentwidget.h"
#include "utils.h"

#define tr(str) simulideTr("ConnectorLine",str)
 
ConnectorLine::ConnectorLine( int x1, int y1, int x2, int y2, Connector* connector )
             : QGraphicsItem()
{
    m_pConnector = connector;
    
    m_prevLine = nullptr;
    m_nextLine = nullptr;

    m_p1X = x1;
    m_p1Y = y1;
    m_p2X = x2;
    m_p2Y = y2;

    m_isBus  = false;
    m_moveP1 = false;
    m_moveP2 = false;
    m_moving = false;
    m_animateCurrent = false;

    m_length = 0;

    m_mousePos = QPoint( 1e6, 1e6 );

    this->setFlag( QGraphicsItem::ItemIsSelectable, true );
    setAcceptHoverEvents( true );

    setCursor( Qt::CrossCursor );
    setZValue( 100 );
    updatePos();
}
ConnectorLine::~ConnectorLine(){}

QRectF ConnectorLine::boundingRect() const
{
    int dy = m_p2Y-m_p1Y;
    int dx = m_p2X-m_p1X;
    int p =-1;
    int d = 2;

    if    ( dx != 0
         && dy != 0 ) return QRect( 0   , 0   , dx  , dy );
    else if( dx > 0 ) return QRect(-1   ,-2   , dx+d, 4 );
    else if( dx < 0 ) return QRect( dx+p,-2   ,-dx+d, 4 );
    else if( dy > 0 ) return QRect(-2   ,-1   , 4   , dy+d );
    else if( dy < 0 ) return QRect(-2   , dy+p, 4   ,-dy+d );
    else              return QRect( 0   , 0   , 0   , 0 );
}

void ConnectorLine::sSetP1( QPoint point )
{
    prepareGeometryChange();
    m_p1X = point.x();
    m_p1Y = point.y();
    updatePos();
}

void ConnectorLine::sSetP2( QPoint point )
{
    prepareGeometryChange();
    m_p2X = point.x();
    m_p2Y = point.y();
    updatePos();
}

void ConnectorLine::moveSimple( QPointF delta )
{
    bool deltaH  = fabs( delta.x() )> 0;
    bool deltaV  = fabs( delta.y() )> 0;
    
    prepareGeometryChange();

    m_p1X = m_p1X + delta.x();
    m_p1Y = m_p1Y + delta.y();
    m_p2Y = m_p2Y + delta.y();
    m_p2X = m_p2X + delta.x();

    bool isHoriz = ( dy() == 0 ) && ( dx() != 0 );
    
    if( m_prevLine && !(m_prevLine->isSelected()) )
    {
        m_prevLine->moveLine( delta.toPoint() );
        
        if( (  isHoriz && deltaV )
          ||( !isHoriz && deltaH ))
            m_prevLine->sSetP2( QPoint( m_p1X, m_p1Y) );
            
        m_prevLine->updatePos();
        m_prevLine->updatePrev();
    }
    if( m_nextLine && !(m_nextLine->isSelected()) )
    {
        m_nextLine->moveLine( delta.toPoint() ); 
        
        if( (  isHoriz && deltaV )
          ||( !isHoriz && deltaH ))
            m_nextLine->sSetP1( QPoint( m_p2X, m_p2Y) );
            
        m_nextLine->updatePos();
        m_nextLine->updateNext();
    }
    updatePos();
}

bool ConnectorLine::isDiagonal() { return ( fabs(m_p2X - m_p1X)>0 && fabs(m_p2Y - m_p1Y)>0 ); }

void ConnectorLine::move( QPointF delta )
{
    if( Circuit::self()->pasting() )
    {
        prepareGeometryChange();
        m_p1Y = m_p1Y + delta.y();
        m_p2Y = m_p2Y + delta.y();
        m_p1X = m_p1X + delta.x();
        m_p2X = m_p2X + delta.x();
        updatePos();

        return;
    }
    int myindex = m_pConnector->lineList()->indexOf( this );
    if( ( myindex == 0 ) || ( myindex == m_pConnector->lineList()->size()-1 ) )
        return;    //avoid moving first or last line

    moveLine( delta.toPoint() );
    updatePos();
    updateLines();
    m_pConnector->refreshPointList();
}

void ConnectorLine::moveLine( QPoint delta )
{
    prepareGeometryChange();

    if( dx() != 0 ){
       m_p1Y = m_p1Y + delta.y();
       m_p2Y = m_p2Y + delta.y();
    }
    if( dy() != 0 ){
       m_p1X = m_p1X + delta.x();
       m_p2X = m_p2X + delta.x();
    }
}

void ConnectorLine::updateNext()
{
    if( m_nextLine ){
        m_nextLine->sSetP1( QPoint( m_p2X, m_p2Y) );
        m_nextLine->updatePos();
    }
}

void ConnectorLine::remove() 
{ 
    if( !isSelected() ){
        Circuit::self()->clearSelection();
        setSelected( true );
    }
    Circuit::self()->removeItems();
}

bool ConnectorLine::connectToWire( QPoint point1 )
{
    int index;
    int myindex = m_pConnector->lineList()->indexOf( this );

    if((( dy() == 0 && fabs( point1.x()-m_p2X ) < 8 ) // point near the p2 corner
         || ( dx() == 0 && fabs( point1.y()-m_p2Y ) < 8 ) )
        && ( myindex != m_pConnector->lineList()->size()-1 ) )
    {
        if( myindex == m_pConnector->lineList()->size()-1 ) return false;
        point1 = p2();
        index = myindex+1;
    }
    else if((( dy() == 0 && fabs( point1.x()-m_p1X ) < 8 ) // point near the p1 corner
              || ( dx() == 0 && fabs( point1.y()-m_p1Y ) < 8 ) )
             && ( myindex != 0 ) )
    {
        if( myindex == 0 ) return false;
        point1 = p1();
        index = myindex;
    }
    else{                                           // split this line in two
        if( dy() == 0 ) point1.setY( m_p1Y );
        else            point1.setX( m_p1X );
        index = myindex+1;
        ConnectorLine* line = new ConnectorLine( point1.x(), point1.y(), m_p2X, p2().y(), m_pConnector );
        m_pConnector->addConLine( line, index );
    }
    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    if( !Circuit::self()->is_constarted() ) Circuit::self()->beginUndoStep(); // A new Connector started here

    QString type = "Node";
    QString id = type +"-"+ Circuit::self()->newSceneId();

    Node* node = new Node( type, id );     // Now add the Node
    node->setPos( point1.x(), point1.y());
    Circuit::self()->addNode( node );

    if( Circuit::self()->is_constarted() )   // A Connector wants to connect here (ends in a node)
    {
        m_pConnector->splitCon( index, node->getPin(0), node->getPin(2) );
        Circuit::self()->closeconnector( node->getPin(1), true );
    }
    else{                                    // A new Connector created here (starts in a node)
        Pin* pin = node->getPin(1);
        if( m_isBus ) pin->setIsBus( true );
        Circuit::self()->newconnector( pin, false );
        m_pConnector->splitCon( index, node->getPin(0), node->getPin(2) );
    }
    return true;
}

void ConnectorLine::mousePressEvent( QGraphicsSceneMouseEvent* event )
{
    if( event->button() == Qt::MiddleButton )                      // Move Line
    {
        event->accept();

        if     ( dy() == 0 ) CircuitView::self()->viewport()->setCursor( Qt::SplitVCursor );
        else if( dx() == 0 ) CircuitView::self()->viewport()->setCursor( Qt::SplitHCursor );
        else                 CircuitView::self()->viewport()->setCursor( Qt::SizeAllCursor );
    }
    else if( event->button() == Qt::LeftButton )
    {
        if( event->modifiers() == Qt::ControlModifier ) setSelected( !isSelected() ); // Select - Deselect
        
        else if( event->modifiers() & Qt::ShiftModifier ) // Move Corner
        {
            QPoint evPoint = toGrid( event->scenePos() ).toPoint();
            
            if     ( evPoint==p1() ) m_moveP1 = true;
            else if( evPoint==p2() ) m_moveP2 = true;
            else{                                          // Move Line
                event->accept();

                if     ( dy() == 0 ) CircuitView::self()->viewport()->setCursor( Qt::SplitVCursor );
                else if( dx() == 0 ) CircuitView::self()->viewport()->setCursor( Qt::SplitHCursor );
                else                 CircuitView::self()->viewport()->setCursor( Qt::SizeAllCursor );
            }
        }
        else{                                         // Connecting a wire here
           if( Circuit::self()->is_constarted() )     // Wire started at Pin is connecting here
           {
               Connector* con = Circuit::self()->getNewConnector();
               if( con == this->connector() ) return;
               if( con->isBus() != m_isBus ) { event->ignore(); return; } // Avoid connect Bus with no-Bus
           }
           QPoint point1 = toGrid( event->scenePos() ).toPoint();

           if( connectToWire( point1 ) ) event->accept();
           else                          event->ignore();
}   }   }

void ConnectorLine::mouseMoveEvent( QGraphicsSceneMouseEvent* event )
{
    event->accept();

    QPoint delta = toGrid( event->scenePos() ).toPoint() - toGrid(event->lastScenePos()).toPoint();

    if( !m_moving && !Circuit::self()->is_constarted() )
    {
        Circuit::self()->saveCompChange( m_pConnector->getUid(), "pointList", m_pConnector->pListStr() );
        m_moving = true;
    }

    if( event->modifiers() & Qt::ShiftModifier           // Move Corner
        && (m_moveP1 || m_moveP2))
    {
        if     ( m_moveP1 ) setP1( p1()+delta );
        else if( m_moveP2 ) setP2( p2()+delta );
    }else{
        int myindex = m_pConnector->lineList()->indexOf( this );

        if( myindex == 0 )
            m_pConnector->addConLine( p1().x(), p1().y(), p1().x(), p1().y(), myindex );

        else if( myindex == m_pConnector->lineList()->size()-1 )
            m_pConnector->addConLine( p2().x(), p2().y(), p2().x(), p2().y(), myindex + 1 );

        moveLine( delta );
    }
    updatePos();
    updateLines();
}

void ConnectorLine::mouseReleaseEvent( QGraphicsSceneMouseEvent* event )
{
    event->accept();
    m_moveP1 = false;
    m_moveP2 = false;
    m_pConnector->remNullLines();
    m_moving = false;
}

void ConnectorLine::contextMenuEvent( QGraphicsSceneContextMenuEvent* event )
{
    if( Circuit::self()->is_constarted() ) return;

    if( m_pConnector->endPin() )
    {
       event->accept();
       QMenu menu;

       QAction* removeAction = menu.addAction( tr("Remove") );
       QObject::connect( removeAction, &QAction::triggered, [=](){ remove(); } );

       menu.exec(event->screenPos());
}   }

void ConnectorLine::hoverMoveEvent( QGraphicsSceneHoverEvent* event )
{
    m_mousePos = event->screenPos();
    QGraphicsItem::hoverMoveEvent( event );
    // use p.x() and p.y() to set the tooltip accordingly, for example:
    //if (p.y() < height()/2)
    //    setTooltip("Upper Half");
    //else
    //    setToolTip("Bottom Half");
    //qDebug() << "WireLine::hoverMoveEvent";
}

void ConnectorLine::hoverLeaveEvent( QGraphicsSceneHoverEvent* event )
{
    m_mousePos = QPoint( 1e6, 1e6 );
    QToolTip::hideText();
    QGraphicsItem::hoverLeaveEvent( event );
}

void ConnectorLine::updtLength()
{
    double termX = m_p2X-m_p1X;
    double termY = m_p2Y-m_p1Y;
    m_length = std::fabs( std::sqrt( termX*termX + termY*termY) ) / 8;
    //update(); // Diagonal wires don't update
}

QPainterPath ConnectorLine::shape() const
{
    int dy = m_p2Y-m_p1Y;
    int dx = m_p2X-m_p1X;
    int q = 0;
    int i = 0;

    if( this->cursor().shape() == Qt::ArrowCursor ) // New Connector
    {
        if     ( dx > 0 ) q = -3;
        else if( dx < 0 ) q =  3;
        if     ( dy > 0 ) i = -3;
        else if( dy < 0 ) i =  3;
    }

    QPainterPath path;
    QVector<QPointF> points;
    
    if( fabs(m_p2X - m_p1X) > fabs(m_p2Y - m_p1Y) )
    {
        points << mapFromScene( QPointF( m_p1X  , m_p1Y-2 ) )
               << mapFromScene( QPointF( m_p1X  , m_p1Y+2 ) )
               << mapFromScene( QPointF( m_p2X+q, m_p2Y+2 ) )
               << mapFromScene( QPointF( m_p2X+q, m_p2Y-2 ) );
    }else{
        points << mapFromScene( QPointF( m_p1X-2, m_p1Y   ) )
               << mapFromScene( QPointF( m_p1X+2, m_p1Y   ) )
               << mapFromScene( QPointF( m_p2X+2, m_p2Y+i ) )
               << mapFromScene( QPointF( m_p2X-2, m_p2Y+i ) );
    }
    path.addPolygon( QPolygonF(points) );
    path.closeSubpath();
    return path;
}

void ConnectorLine::paint( QPainter* p, const QStyleOptionGraphicsItem*, QWidget* )
{
    //pen.setColor( Qt::darkGray);
    //p->setPen( pen );

    double dX = dx();
    double dY = dy();

    QColor color;
    if( isSelected() ) color = Qt::darkGray ;
    else if( m_isBus ) color = Qt::darkGreen;
    else if( Circuit::self()->animateLogic() )
    {
        if( m_pConnector->getVoltage() > 2.5 ) color = QColor( 200, 50, 50  );
        else                                   color = QColor( 50,  50, 200 );
    }
    else color = QColor( 40, 40, 60 );

    QPen pen( color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
    //p->setBrush( Qt::green );
    //p->drawRect( boundingRect() );
    //p->setBrush( Qt::blue );
    //p->drawPath( shape() );
    
    if( m_isBus ) pen.setWidth( 3 );
    else if( m_animateCurrent ) pen.setWidthF( 2.8 );

    p->setPen( pen );
    p->drawLine( 0, 0, dX, dY );

    if( m_isBus ) return;
    if( !m_animateCurrent ) return;
    if( !Simulator::self()->isRunning() ) return;

    // TODO: optimize in connector ---------------------------------
    double current = fabs( m_pConnector->m_current );
    if( current == 0 ) return;

    double speed = fabs( m_pConnector->m_currentSpeed );
    double bspeed = CurrentWidget::self()->speed() * current;
    if( bspeed > 8 ) bspeed = 8;

    color = QColor( 79+44*speed, 79+44*speed, 30*bspeed );
    // -------------------------------------------------------------

    p->setBrush( color );
    pen.setWidthF( 0.6 );
    pen.setBrush( color );
    p->setPen( pen );

    double step = m_pConnector->m_step/8;
    dX /= m_length;
    dY /= m_length;

    for( double i=0; i<m_length; i++ )
    {
        double delta = i+step;
        if( delta > m_length ) break;

        p->drawEllipse( QPointF( dX*delta, dY*delta ), 1.6, 1.6 );
    }

    if( m_mousePos.x() < 1e6 )
    {
        //QToolTip::showText( m_mousePos, "" );
        QToolTip::showText( m_mousePos, "current = "+QString::number( current )+" A\n"
                                       +"voltage = "+QString::number( m_pConnector->getVoltage() )+" V");
    }
}

