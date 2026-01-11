/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <math.h>
#include <QDebug>

#include "component.h"
#include "label.h"
#include "mainwindow.h"
#include "connector.h"
#include "connectorline.h"
#include "circuitwidget.h"
#include "circuit.h"
#include "utils.h"
#include "propdialog.h"
#include "linker.h"

#include "doubleprop.h"
#include "boolprop.h"
#include "intprop.h"
#include "stringprop.h"
#include "pointprop.h"

#define tr(str) simulideTr("Component",str)

int  Component::m_error = 0;
bool Component::m_boardMode = false;

Component::Component( QString type, QString id )
         : CompBase( type, id )
         , QGraphicsItem()
{
    m_help = "";
    m_Hflip  = 1;
    m_Vflip  = 1;
    m_color  = QColor( Qt::white );

    m_backPixmap = nullptr;
    //m_group = NULL;

    m_ansiSymbol = Circuit::self()->ansiSymbols();
    m_showId     = false;
    m_showVal    = false;
    m_moving     = false;
    m_isMainComp = false;
    m_hidden     = false;
    m_crashed    = false;
    m_warning    = false;
    m_graphical  = false;
    m_isLinker   = false;
    m_linkedTo   = nullptr;
    m_background = "";
    m_showProperty = "";
    m_linkNumber = -1;

    m_boardPos = QPointF(-1e6,-1e6 );
    m_boardRot = -1e6;
    m_circPos  = QPointF(0, 0);
    m_circRot  = 0;
    m_boardHflip = 1;
    m_boardVflip = 1;
    m_circHflip = 1;
    m_circVflip = 1;

    QFont font;
    font.setFamily( MainWindow::self()->defaultFontName() );
    font.setPixelSize( 10 );
    font.setLetterSpacing( QFont::PercentageSpacing, 100 );
#ifdef _WIN32
    font.setLetterSpacing( QFont::PercentageSpacing, 90 );
#endif

    m_idLabel = new Label();
    m_idLabel->setComponent( this );
    m_idLabel->setDefaultTextColor( Qt::darkBlue );
    m_idLabel->setFont( font );
    setLabelPos(-16,-24, 0 );
    setShowId( false );
    
    m_valLabel = new Label();
    m_valLabel->setComponent( this );
    m_valLabel->setDefaultTextColor( Qt::darkRed );
    setValLabelPos(-16, 20, 0 );
    font.setPixelSize( 8 );
    m_valLabel->setFont( font );
    m_valLabel->setVisible( false );

    if( !id.contains("-") ) id.prepend( type+"-" );
    Component::setIdLabel( id );

    setCursor( Qt::OpenHandCursor );
    setFlag( QGraphicsItem::ItemIsSelectable, true );

    addPropGroup( { "CompBase", {
new StrProp <Component>("itemtype","","", this, &Component::itemType,  &Component::setItemType ),
new StrProp <Component>("CircId"  ,"","", this, &Component::getUid,    &Component::setUid ),
new BoolProp<Component>("mainComp","","", this, &Component::isMainComp,&Component::setMainComp ),// Related to Subcircuit:
new StrProp <Component>("ShowProp","","", this, &Component::showProp,  &Component::setShowProp )
    }, groupHidden | groupNoCopy } );

    addPropGroup( { "CompGraphic", {
new BoolProp <Component>("Show_id"  ,"","", this, &Component::showId,    &Component::setShowId ),
new BoolProp <Component>("Show_Val" ,"","", this, &Component::showVal,   &Component::setShowVal ),
new PointProp<Component>("Pos"      ,"","", this, &Component::position,  &Component::setPosition ),
new DoubProp <Component>("rotation" ,"","", this, &Component::getAngle,  &Component::setAngle ),
new IntProp  <Component>("hflip"    ,"","", this, &Component::hflip,     &Component::setHflip ),
new IntProp  <Component>("vflip"    ,"","", this, &Component::vflip,     &Component::setVflip ),
new StrProp  <Component>("label"    ,"","", this, &Component::idLabel,   &Component::setIdLabel ),
new PointProp<Component>("idLabPos" ,"","", this, &Component::getIdPos,  &Component::setIdPos ),
new IntProp  <Component>("labelrot" ,"","", this, &Component::getIdRot,  &Component::setIdRot ),
new PointProp<Component>("valLabPos","","", this, &Component::getValPos, &Component::setValPos ),
new IntProp  <Component>("valLabRot","","", this, &Component::getValRot, &Component::setValRot ),
    }, groupHidden | groupNoCopy } );

    addPropGroup( { "Board", {                   // Board properties
new PointProp<Component>("boardPos", "","", this, &Component::boardPos, &Component::setBoardPos ),
new PointProp<Component>("circPos" , "","", this, &Component::circPos,  &Component::setCircPos ),
new DoubProp <Component>("boardRot", "","", this, &Component::boardRot, &Component::setBoardRot ),
new DoubProp <Component>("circRot" , "","", this, &Component::circRot,  &Component::setCircRot ),
new IntProp  <Component>("boardHflip","","",this, &Component::boardHflip, &Component::setBoardHflip ),
new IntProp  <Component>("boardVflip","","",this, &Component::boardVflip, &Component::setBoardVflip ),
    }, groupHidden | groupNoCopy } );
}
Component::~Component(){}

bool Component::setPropStr( QString prop, QString val )
{
    bool ok = CompBase::setPropStr( prop, val );
    if( prop == m_showProperty )
    {
        QString text = "";
        if( !m_showProperty.isEmpty() ) text = getPropStr( m_showProperty );
        setValLabelText( text );
    }
    return ok;
}

void Component::setup()
{
    if( !m_invertedPins.isEmpty() )
    {
        QStringList pinList = m_invertedPins.split(",");

        for( Pin* pin : getPins() ){
            QString id = pin->pinId().split("-").last();
            pin->setInverted( pinList.contains( id ) );  //userInvertPin();
        }
        m_invertedPins.clear();
    }
    QString showP = showProp();
    if( !showP.isEmpty() ) setValLabelText( getPropStr( showP ) );
}

QVariant Component::itemChange( GraphicsItemChange change, const QVariant &value )
{
    if( m_isLinker && change == QGraphicsItem::ItemSelectedChange && value == false ) // Hide linked Components
    {
        Linker* linker = dynamic_cast<Linker*>(this);
        linker->showLinked( false );
    }
    return QGraphicsItem::itemChange( change, value );
}

bool Component::freeMove( bool ctrlMod )
{
    return m_boardMode && ctrlMod;
}

void Component::mousePressEvent( QGraphicsSceneMouseEvent* event )
{
    if( this->parentItem() )
    {
        Component* parentComp = static_cast<Component*>( this->parentItem() );
        parentComp->mousePressEvent( event );
        return;
    }
    if( event->button() == Qt::LeftButton )
    {
        event->accept();
        if( Linker::m_selecComp ){
            Linker::m_selecComp->compSelected( this );
        }
        else if( event->modifiers() & Qt::ControlModifier )
        {
            setSelected( !isSelected() );
        }
        else{
            QList<QGraphicsItem*> itemlist = Circuit::self()->selectedItems();
            if( !isSelected() )     // Unselect everything and select this
            {
                for( QGraphicsItem* item : itemlist ) item->setSelected( false );
                setSelected( true );
                if( m_isLinker && isSelected() ) // Show/Hide linked Components (we are not linking right now)
                {
                    Linker* linker = dynamic_cast<Linker*>(this);
                    linker->showLinked( true );
                }
            }
            else{                   // Deselect childs
                for( QGraphicsItem* item : itemlist )
                {
                    QList<QGraphicsItem*> childs = item->childItems();
                    for( QGraphicsItem* child : childs ) child->setSelected( false );
            }   }
            setCursor( Qt::ClosedHandCursor );
        }
        QApplication::focusWidget()->clearFocus();
        CircuitView::self()->setFocus();
    }
    else if( event->button() == Qt::MiddleButton ) event->setAccepted( false );
}

void Component::mouseMoveEvent( QGraphicsSceneMouseEvent* event )
{
    if( this->parentItem() )
    {
        Component* parentComp = static_cast<Component*>( this->parentItem() );
        parentComp->mouseMoveEvent( event );
        if( !m_hidden ) moveSignal();
        return;
    }
    event->accept();

    QPointF delta = toGrid(event->scenePos()) - toGrid(event->lastScenePos());

    QList<QGraphicsItem*> itemList = Circuit::self()->selectedItems();
    QList<ConnectorLine*> lineList;

    if( !m_moving )         // Get lists of elements to move and save Undo state
    {
        m_moveFree = true;
        m_conMoveList.clear();
        m_compMoveList.clear();

        bool ctrlMod = event->modifiers() == Qt::ControlModifier;

        for( QGraphicsItem* item : itemList )
        {
            if( item->type() == UserType+2 )          // ConnectorLine selected
            {
                m_moveFree = false;
                ConnectorLine* line =  qgraphicsitem_cast<ConnectorLine*>( item );
                lineList.append( line );
                Connector* con = line->connector();
                if( !m_conMoveList.contains( con ) ) m_conMoveList.append( con ); // Connectors selected
            }
            else if( item->type() == UserType+1 )     // Component selected
            {
                Component* comp =  qgraphicsitem_cast<Component*>( item );
                if( !comp->freeMove( ctrlMod ) ) m_moveFree = false;

                m_compMoveList.append( comp );

                std::vector<Pin*> pins = comp->getPins();
                for( Pin* pin : pins )
                {                                     // Connectors attached to selected Component
                    if( !pin ) continue;
                    Connector* con = pin->connector();
                    if( con && !m_conMoveList.contains( con ) ) m_conMoveList.append( con );
        }   }   }
        if( !m_moveFree && delta == QPointF(0, 0) ) return;

        Circuit::self()->beginCircuitBatch();
        for( Component* comp : m_compMoveList )    // Undo step
            Circuit::self()->addCompChange( comp->getUid(), "Pos", comp->getPropStr("Pos") );

        for( Connector* con  : m_conMoveList )    // Undo step
            Circuit::self()->addCompChange( con->getUid(), "pointList", con->pListStr() );

        m_moving = true;
        Circuit::self()->saveChanges();
    }
    if( m_moveFree ) delta = event->scenePos() - event->lastScenePos();

    if( delta == QPointF(0, 0) ) return;

    for( ConnectorLine* line : lineList   ) line->moveSimple( delta ); // Move ConnectorLine
    for( Component* comp : m_compMoveList ) comp->move( delta );    // Move Components selected
    for( Connector* con  : m_conMoveList  ) con->isMoved();         // Update Connectors
}

void Component::mouseReleaseEvent( QGraphicsSceneMouseEvent* event )
{
    if( this->parentItem() )
    {
        Component* parentComp = static_cast<Component*>( this->parentItem() );
        parentComp->mouseReleaseEvent( event );
        return;
    }
    event->accept();
    setCursor( Qt::OpenHandCursor );

    if( m_moving ) m_moving = false;

    Circuit::self()->update();
}

void Component::mouseDoubleClickEvent( QGraphicsSceneMouseEvent* event )
{
    if( this->parentItem() )
    {
        Component* parentComp = static_cast<Component*>( this->parentItem() );
        parentComp->mouseDoubleClickEvent( event );
        return;
    }
    if( event->button() == Qt::LeftButton ) slotProperties();
}

void Component::contextMenuEvent( QGraphicsSceneContextMenuEvent* event )
{
    if( this->parentItem() )
    {
        Component* parentComp = static_cast<Component*>( this->parentItem() );
        parentComp->contextMenuEvent( event );
    }
    else if( !acceptedMouseButtons() ) event->ignore();
    else{
        event->accept();
        QMenu* menu = new QMenu();
        contextMenu( event, menu );
        menu->deleteLater();
}   }

void Component::contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu )
{
    if( !event && m_isMainComp ) // Main Component in Subcircuit
    {
        QAction* propertiesAction = menu->addAction( QIcon( ":/properties.svg"),tr("Properties") );
        QObject::connect( propertiesAction, &QAction::triggered, [=](){ slotProperties(); } );
        menu->addSeparator();
        return;
    }
    m_eventpoint = mapToScene( toGrid(event->pos()) );

    QAction* copyAction = menu->addAction(QIcon(":/copy.svg"),tr("Copy")+"\tCtrl+C");
    QObject::connect( copyAction, &QAction::triggered, [=](){ slotCopy(); } );

    QAction* cutAction = menu->addAction(QIcon(":/cut.svg"),tr("Cut")+"\tCtrl+X");
    QObject::connect( cutAction, &QAction::triggered, [=](){ slotCut(); } );

    QAction* removeAction = menu->addAction( QIcon( ":/remove.svg"),tr("Remove")+"\tDel" );
    QObject::connect( removeAction, &QAction::triggered, [=](){ slotRemove(); } );

    /*QAction* groupAction = menu->addAction( QIcon( ":/group.png"),tr("Group") );
    QObject::connect( groupAction, &QAction::triggered, [=](){ slotGroup())(); } );*/
    
    QAction* propertiesAction = menu->addAction( QIcon( ":/properties.svg"),tr("Properties") );
    QObject::connect( propertiesAction, &QAction::triggered, [=](){ slotProperties(); } );
    menu->addSeparator();

    QAction* rotateCWAction = menu->addAction( QIcon( ":/rotatecw.svg"),tr("Rotate CW")+"\tCtrl+R" );
    QObject::connect( rotateCWAction, &QAction::triggered, [=](){ rotateCW(); } );

    QAction* rotateCCWAction = menu->addAction(QIcon( ":/rotateccw.svg"),tr("Rotate CCW")+"\tCtrl+Shift+R" );
    QObject::connect( rotateCCWAction, &QAction::triggered, [=](){ rotateCCW(); } );

    QAction* rotateHalfAction = menu->addAction(QIcon(":/rotate180.svg"),tr("Rotate 180") );
    QObject::connect( rotateHalfAction, &QAction::triggered, [=](){ rotateHalf(); } );
    
    QAction* H_flipAction = menu->addAction(QIcon(":/hflip.svg"),tr("Horizontal Flip")+"\tCtrl+L" );
    QObject::connect( H_flipAction, &QAction::triggered, [=](){ slotH_flip(); } );
    
    QAction* V_flipAction = menu->addAction(QIcon(":/vflip.svg"),tr("Vertical Flip")+"\tCtrl+Shift+L" );
    QObject::connect( V_flipAction, &QAction::triggered, [=](){ slotV_flip(); } );

    menu->exec(event->screenPos());
}

void Component::slotCopy()
{
    if( !isSelected() ) Circuit::self()->clearSelection();
    setSelected( true );
    Circuit::self()->copy( m_eventpoint );
}

void Component::slotCut()
{
    if( !isSelected() ) Circuit::self()->clearSelection();
    setSelected( true );
    Circuit::self()->copy( m_eventpoint );
    Circuit::self()->removeItems();
}

void Component::slotRemove()
{
    if( !isSelected() ){
        Circuit::self()->clearSelection();
        setSelected( true );
    }
    Circuit::self()->removeItems();
}

void Component::slotGroup()
{
    /*if( !m_group )
    {
        m_group = Circuit::self()->createItemGroup(Circuit::self()->selectedItems());
    }*/
}

void Component::remove()
{
    for( uint i=0; i<m_pin.size(); i++ )
        if( m_pin[i] ) m_pin[i]->removeConnector();

    if( m_linkedTo ) m_linkedTo->removeLinked( this );

    Circuit::self()->compRemoved( true );
}

void Component::deletePin( Pin* pin )
{
    pin->removeConnector();
    m_signalPin.removeAll( pin );
    delete pin;
}

void Component::slotProperties()
{
    if( !m_propDialog )
    {
        if( m_help == "" )
        {
            QString name = m_type;

            if( m_type == "Subcircuit"|| m_type == "MCU" ) findHelp();
            else m_help = MainWindow::self()->getHelp( name, false );
        }
        m_propDialog = new PropDialog( CircuitWidget::self(), m_help );
        m_propDialog->setComponent( this );
    }
    m_propDialog->show();
    m_propDialog->adjustWidgets();
    m_propDialog->raise();
}

QString Component::invertedPins()
{
    QString pinListStr;
    std::vector<Pin*> pins = getPins();

    for( Pin* pin : pins )
        if( pin->inverted() ) //  userInverted() )
            pinListStr += pin->pinId().split("-").last()+",";

    return pinListStr;
}

void Component::setInvertPins( QString pins )
{
    m_invertedPins = pins;
    /*QStringList pinList = pins.split(",");
    for( QString pinStr : pinList )
    {
        Pin* pin = Circuit::self()->getPin( m_id+"-"+pinStr );
        if( pin ) pin->setInverted( true );
    }*/
}

void Component::slotH_flip()
{
    if( !m_hidden ) Circuit::self()->saveCompChange( m_id, "hflip", getPropStr("hflip") );
    m_Hflip = -m_Hflip;
    setflip();
}

void Component::slotV_flip()
{
    if( !m_hidden ) Circuit::self()->saveCompChange( m_id, "vflip", getPropStr("vflip") );
    m_Vflip = -m_Vflip;
    setflip();
}

void Component::setHflip( int hf )
{
    if( hf != 1 && hf != -1 ) hf = 1;
    m_Hflip = hf;
    setflip();
}

void Component::setVflip( int vf )
{
    if( vf != 1 && vf != -1 ) vf = 1;
    m_Vflip = vf;
    setflip();
}

void Component::setflip()
{
    setTransform( QTransform::fromScale( m_Hflip, m_Vflip ) );
    m_idLabel->setTransform( QTransform::fromScale( m_Hflip, m_Vflip ) );
    m_valLabel->setTransform( QTransform::fromScale( m_Hflip, m_Vflip ) );
    m_idLabel->updtLabelPos();
    m_valLabel->updtLabelPos();
    for( Pin* pin : m_signalPin ) pin->flip( m_Hflip, m_Vflip );

    if( !m_hidden ) moveSignal();    // Used by sockets
}

void Component::rotateCW()   { rotateAngle( 90 ); }
void Component::rotateCCW()  { rotateAngle(-90 ); }
void Component::rotateHalf() { rotateAngle(-180); }

void Component::rotateAngle( double a )
{
    if( !m_hidden ) Circuit::self()->saveCompChange( m_id, "rotation", getPropStr("rotation") );
    Component::setRotation( rotation() + a*m_Hflip*m_Vflip );
    if( !m_hidden ) moveSignal();
}

QString Component::findIdLabel() /// FIXME: move to Component??
{
    QString label = idLabel();
    if( this->parentItem() ){
        Component* comp = qgraphicsitem_cast<Component*>( this->parentItem() );
        label = comp->idLabel();
    }
    return label;
}

QString Component::idLabel() { return m_idLabel->toPlainText(); }
void Component::setIdLabel( QString id ) { m_idLabel->setPlainText( id ); m_idLabel->updtLabelPos(); }

QPointF Component::getIdPos() { return m_idLabel->getLabelPos(); }
void Component::setIdPos( QPointF p ) { m_idLabel->setLabelPos( p ); }

int Component::getIdRot() { return m_idLabel->getAngle(); }
void Component::setIdRot( int r ) { m_idLabel->setAngle(r); }

void Component::setLabelPos( float x, float y, int rot )
{
    m_idLabel->m_labelx = x;
    m_idLabel->m_labely = y;
    m_idLabel->m_labelrot = rot;
    m_idLabel->updtLabelPos();
}
void Component::updtLabelPos() { m_idLabel->updtLabelPos(); }

void Component::setShowId( bool show ) { m_idLabel->setVisible( show ); m_showId = show; }
void Component::setShowVal( bool show ) { m_valLabel->setVisible( show ); m_showVal = show; }

QPointF Component::getValPos() { return m_valLabel->getLabelPos(); }
void Component::setValPos( QPointF p ) { m_valLabel->setLabelPos( p ); }

int Component::getValRot() { return m_valLabel->getAngle(); }
void Component::setValRot( int r ) { m_valLabel->setAngle(r); }

void Component::setValLabelPos( float x, float y, int rot )
{
    m_valLabel->m_labelx = x;
    m_valLabel->m_labely = y;
    m_valLabel->m_labelrot = rot;
    m_valLabel->updtLabelPos();
}

void Component::updtValLabelPos() { m_valLabel->updtLabelPos(); }

void Component::setValLabelText( QString t ) { m_valLabel->setPlainText( t ); m_valLabel->updtLabelPos(); }
QString Component::getValLabelText() { return m_valLabel->toPlainText(); }

QString Component::showProp()
{
    if( m_showVal ) return m_showProperty;
    else            return "";
}

void Component::setShowProp( QString prop )
{
    m_showProperty = prop;
    setShowVal( !(prop.isEmpty()) );
}

void Component::moveSignal()
{
    for( Pin* pin : m_signalPin ) pin->isMoved();
}

void Component::addSignalPin( Pin* pin )
{
    if( !m_signalPin.contains( pin ) ) m_signalPin.append( pin );
}

void Component::remSignalPin( Pin* pin )
{
    m_signalPin.removeAll( pin );
}

bool Component::setLinkedTo( Linker* li )
{
    if( li && m_linkedTo ) return false;
    if( !li ) m_linkNumber = -1;
    m_linkedTo = li;
    update();
    return true;
}

void Component::setHidden( bool hid, bool hidArea, bool hidLabel )
{
    m_hidden = hid;

    if( m_graphical ) { for( Pin* pin : m_pin ) pin->setVisible( !hid ); }
    else this->setVisible( !hid );

    if( hidLabel ){
        m_valLabel->setVisible( false );
        m_idLabel->setVisible( false );
    }else{
        m_valLabel->setVisible( m_showVal );
        m_idLabel->setVisible( m_showId );
    }
}

void Component::setBackground( QString bck )
{
    m_background = bck;

    if( bck.startsWith("color") )
    {
        bck.remove("color").remove("(").remove(")").remove(" ");
        QStringList rgb = bck.split(",");
        if( rgb.size() < 3 ) return;

        delete m_backPixmap;
        m_backPixmap = nullptr;

        m_color = QColor( rgb.at(0).toInt(), rgb.at(1).toInt(), rgb.at(2).toInt() );
    }
    else if( bck != "" )
    {
        QString pixmapPath = MainWindow::self()->getCircFilePath( bck );
        if( !QFile::exists( pixmapPath ) ) pixmapPath = MainWindow::self()->getDataFilePath("images")+"/"+bck;
        if( !QFile::exists( pixmapPath ) ) pixmapPath = ":/"+bck; // Image not in simulide data folder, use hardcoded image

        if( QFile::exists( pixmapPath ) )
        {
            if( !m_backPixmap ) m_backPixmap = new QPixmap();
            m_backPixmap->load( pixmapPath );
        }
    }
    update();
}

/*QString Component::print()
{
    if( !m_printable ) return "";
    
    QString str = m_id+" : ";
    str += m_id.split("-").first()+" ";
    /// FIXME if( m_value > 0 ) str += QString::number( m_value )+" "+m_unit+"\n";
    
    return str;
}*/

void Component::paintSelected( QPainter* p )
{
    if( m_warning || m_crashed )
    {
        double speed=0, opaci=1;
        if( m_crashed ){
            speed = 0.1; opaci = 0;
            p->fillRect( boundingRect(), QColor(255, 100, 0, 150) );
        }
        else if( m_warning ){
            speed = 0.05; opaci = 0.4;
            p->fillRect( boundingRect(), QColor(200, 200, 0, 150) );
        }
        m_opCount += speed;
        if( m_opCount > 0.6 ) m_opCount = 0.0;
        p->setOpacity( m_opCount+opaci );
    }
    if( isSelected() )
    {
        p->setOpacity( 0.5 );
        p->fillRect( boundingRect(), Qt::darkGray  );
    }
    else if( /*Linker::m_selecComp &&*/ m_linkNumber >= 0 ){ // This Component is linked
        p->setOpacity( 0.3 );
        p->fillRect( boundingRect(), Qt::blue  );
        p->setOpacity( 1 );
        p->drawText( boundingRect(), Qt::AlignCenter, QString::number(m_linkNumber) );
    }
    else if( !m_hidden )
    {
        if( m_isMainComp ){
            p->setOpacity( 0.5 );
            p->fillRect( boundingRect(), Qt::yellow  );
        }
    }
}

void Component::paint( QPainter* p, const QStyleOptionGraphicsItem*, QWidget* )
{
    QPen pen( Qt::black, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );

    //p->drawPath( shape() );
    p->setBrush( m_color );
    p->setPen( pen );
}
