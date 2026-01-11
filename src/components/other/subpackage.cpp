/***************************************************************************
 *   Copyright (C) 2019 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <math.h>

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QTextStream>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QComboBox>
#include <QDebug>
#include <QList>
#include <QVector>

#include "subpackage.h"
#include "packagepin.h"
#include "mainwindow.h"
#include "itemlibrary.h"
#include "circuitwidget.h"
#include "componentlist.h"
#include "propdialog.h"
#include "circuit.h"
#include "node.h"
#include "utils.h"

#include "stringprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("SubPackage",str)

QString SubPackage::m_lastPkg = "";

Component* SubPackage::construct( QString type, QString id )
{ return new SubPackage( type, id ); }

LibraryItem* SubPackage::libraryItem()
{
    return new LibraryItem(
        tr("Package"),
        "Other",
        "resistordip.png",
        "Package",
        SubPackage::construct );
}

SubPackage::SubPackage( QString type, QString id )
          : Chip( type, id )
{
    m_linkCursor = QCursor( QPixmap(":/expose.png"), 10, 10 );

    m_width  = 4;
    m_height = 8;
    m_area = QRect(0, 0, m_width*8, m_height*8);

    m_changed = false;
    m_fakePin = false;
    m_isLS    = true;
    m_graphical = true;
    m_boardMode = false;
    m_name = "";

    //m_lsColor = QColor( 235, 240, 255 );
    m_icColor = QColor( 50, 50, 70 );

    m_boardModeAction = new QAction( tr("Board Mode") );
    m_boardModeAction->setCheckable( true );
    QObject::connect( m_boardModeAction, &QAction::triggered, [=](){ boardModeSlot(); } );
    m_boardMode = false;
    
    setAcceptHoverEvents( true );
    setZValue(-3 );
    
    m_pkgeFile = "";

    QString enums = "None,Logic,Board,Shield,Module;"
      +tr("None")+","+tr("Logic")+","+tr("Board")+","+tr("Shield")+","+tr("Module");

    addPropGroup( { tr("Main"), {
        new StrProp <SubPackage>("SubcType", tr("Type"), enums
                                , this, &SubPackage::subcTypeStr, &SubPackage::setSubcTypeStr,0,"enum" ),

        new IntProp <SubPackage>("Width", tr("Width"),""
                                , this, &SubPackage::width, &SubPackage::setWidth ,0,"uint" ),

        new IntProp <SubPackage>("Height", tr("Height"),""
                                , this, &SubPackage::height, &SubPackage::setHeight,0,"uint" ),

        new StrProp <SubPackage>("Name",tr("Name"),""
                                , this, &SubPackage::name, &SubPackage::setName ),

        new StrProp <SubPackage>("Package_File", tr("Package File"),""
                                , this, &SubPackage::packageFile, &SubPackage::setPackageFile, propHidden ),

        new BoolProp<SubPackage>("Logic_Symbol", tr("Logic Symbol"),""
                                , this, &SubPackage::logicSymbol, &SubPackage::setLogicSymbol ),

        new BoolProp<SubPackage>("Custom_Color", tr("Custom Color"),""
                                , this, &SubPackage::customColor, &SubPackage::setCustomColor ),

        new StrProp<SubPackage>("BckGndColor", tr("Background Color"), ""
                         , this, &SubPackage::pkgColorStr, &SubPackage::setPkgColorStr,0,"color" ),

        new StrProp <SubPackage>("Background", tr("Background image"),""
                                , this, &SubPackage::background, &SubPackage::setBackground, 0,"file" ),

        new BoolProp<SubPackage>("Border", tr("Border"),""
                                , this, &SubPackage::border, &SubPackage::setBorder ),

        new StrProp <SubPackage>("BckGndData", "",""
                                , this, &SubPackage::bckGndData, &SubPackage::setBckGndData,propHidden ),

        new StrProp <SubPackage>("Pins", "",""
                                , this, &SubPackage::packagePins, &SubPackage::setPackagePins,propHidden ),
    }, 0} );
}
SubPackage::~SubPackage()
{
    delete m_boardModeAction;
}

void SubPackage::setWidth( int width )
{
    if( width < 1 ) width = 1;
    if( m_width == width ) return;
    m_changed = true;

    m_width = width;
    m_area = QRect(0, 0, m_width*8, m_height*8);
    update();
    Circuit::self()->update();
}

void SubPackage::setHeight( int height )
{
    if( height < 1 ) height = 1;
    if( m_height == height ) return;
    m_changed = true;

    m_height = height;
    m_area = QRect( 0, 0, m_width*8, m_height*8 );
    update();
    Circuit::self()->update();
}

void SubPackage::setSubcTypeStr( QString s )
{
    if( m_subcType == s ) return;

    SubPackage* currentBoard = Circuit::self()->getBoard();
    if( s =="Board" || s =="Shield" || s =="Module" )
    {
        if( currentBoard && currentBoard != this ) // Only one board Package can be in the circuit
        {
            qDebug() << "SubPackage::setSubcTypeStr: ERROR: Only one Board allowed";
            return;
        }
        Circuit::self()->setBoard( this );
        m_isBoard = true;
    }
    else if( currentBoard == this ) Circuit::self()->setBoard( nullptr );

    m_subcType = s;

    if( m_showVal && (m_showProperty == "SubcType") )
        setValLabelText( m_subcType );
}

void SubPackage::hoverMoveEvent( QGraphicsSceneHoverEvent* event ) 
{
    if( (event->modifiers() & Qt::ShiftModifier) && !(event->modifiers() & Qt::ControlModifier) )
    {
        m_fakePin = true;
        
        int xPos = snapToGrid4( (int)event->pos().x() );
        int yPos = snapToGrid4( (int)event->pos().y() );
        
        if( xPos == 0 && yPos >= 8 && yPos <= m_height*8-8 ) // Left
        {
            m_angle = 180;
            m_p1X = -8;
            m_p1Y = yPos;
            m_p2X = 0;
            m_p2Y = yPos;
        }
        else if( xPos == m_width*8 && yPos >= 8 && yPos <= m_height*8-8 ) // Right
        {
            m_angle = 0;
            m_p1X = m_width*8+8;
            m_p1Y = yPos;
            m_p2X = m_width*8;
            m_p2Y = yPos;
        }
        else if( yPos == 0 && xPos >= 8&& xPos <= m_width*8-8 ) // Top 
        {
            m_angle = 90;
            m_p1X = xPos;
            m_p1Y = -8;
            m_p2X = xPos;
            m_p2Y = 0;
        }
        else if( yPos == m_height*8 && xPos >= 8 && xPos <= m_width*8-8 ) // Bottom
        {
            m_angle = 270;
            m_p1X = xPos;
            m_p1Y = m_height*8+8;
            m_p2X = xPos;
            m_p2Y = m_height*8;
        }
        else //m_fakePin = false;
        {
            m_angle = 180;
            m_p1X = xPos;
            m_p2X = xPos+1;
            m_p1Y = m_p2Y = yPos;
        }
        Circuit::self()->update();
    }
    else QGraphicsItem::hoverMoveEvent(event);
}

void SubPackage::hoverLeaveEvent( QGraphicsSceneHoverEvent* event ) 
{
    m_fakePin = false;
    Circuit::self()->update();
    QGraphicsItem::hoverLeaveEvent(event);
}

void SubPackage::mousePressEvent( QGraphicsSceneMouseEvent* event )
{
    if( m_selecComp ) return; // Used when linking or creating Boards to set this as main component

    if( m_fakePin )
    {
        event->accept();
        m_fakePin = false;

        QColor color = m_isLS ? Qt::black : QColor( 250, 250, 200 );

        m_eventPin = new PackagePin( m_angle, QPoint(m_p1X,m_p1Y ), "name", 0, this );

        m_eventPin->setPinId( "Id" );
        m_eventPin->setLabelColor( color );
        m_eventPin->setLabelPos();
        if( m_p2X == m_p1X+1) m_eventPin->setLength( 1 );
        else                  m_eventPin->setLabelText( "Name" );
        m_pkgePins.append( m_eventPin );

        editPin();
        Circuit::self()->update();
    }
    else Component::mousePressEvent( event );
}

void SubPackage::contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu )
{
    QAction* loadAction = menu->addAction( QIcon(":/open.svg"),tr("Load Package") );
    QObject::connect( loadAction, &QAction::triggered, [=](){ loadPackage(); } );

    QAction* saveAction = menu->addAction( QIcon(":/save.svg"),tr("Save Package") );
    QObject::connect( saveAction, &QAction::triggered, [=](){ slotSave(); } );

    menu->addSeparator();

    if( this->isBoard() )
    {
        m_boardModeAction->setChecked( m_boardMode );
        menu->addAction( m_boardModeAction );
    }
    QAction* mainCompAction = menu->addAction( QIcon(":/subcl.png"),tr("Select Exposed Components") );
    QObject::connect( mainCompAction, &QAction::triggered, [=](){ mainComp(); } );

    Component::contextMenu( event, menu );
}

void SubPackage::compSelected( Component* comp )
{
    if( comp) comp->setMainComp( !comp->isMainComp() );
    else      Linker::compSelected( comp );
}

void SubPackage::boardModeSlot()
{
    m_boardMode = m_boardModeAction->isChecked();
    setBoardMode( m_boardMode );
    Component::m_boardMode = m_boardMode;
    Circuit::self()->update();
}

void SubPackage::setBoardMode( bool mode )
{
    for( Connector* con : *Circuit::self()->conList() )
    {
        if( con ) con->setVisib( !mode );
    }
    for( Node* nod : *Circuit::self()->nodeList() ) nod->setHidden( mode );

    for( Component* comp : *Circuit::self()->compList() )
    {
        if( comp->itemType() == "Package" ) continue;

        if( mode )   // We are in Board mode
        {
            comp->setCircPos( comp->pos() );
            comp->setCircRot( comp->rotation() );
            comp->setCircHflip( comp->hflip() );
            comp->setCircVflip( comp->vflip() );

            if( comp->boardRot() != -1e+6 )  // Board Rotation already defined
            {
                comp->setPos( comp->boardPos() + this->pos() );
                comp->setRotation( comp->boardRot() );
                comp->setHflip( comp->boardHflip() );
                comp->setVflip( comp->boardVflip() );
            }
        }else        // We are in Circuit mode
        {
            comp->setBoardPos( comp->pos()-this->pos() );
            comp->setBoardRot( comp->rotation() );
            comp->setBoardHflip( comp->hflip() );
            comp->setBoardVflip( comp->vflip() );

            comp->setPos( comp->circPos() );
            comp->setRotation( comp->circRot() );
            comp->setHflip( comp->circHflip() );
            comp->setVflip( comp->circVflip() );
        }
        comp->setHidden( mode, false, mode );
    }
}

void SubPackage::addNewPin( QString id, QString type, QString label, int pos, int xpos, int ypos, int angle, int length, int space )
{
    PackagePin* pin = new PackagePin( angle, QPoint(xpos, ypos), m_id+"-"+id, pos-1, this ); // pos in package starts at 1

    QColor color = m_isLS ? Qt::black : QColor( 250, 250, 200 );

    pin->setLabelColor( color );
    pin->setPos( QPoint( xpos, ypos ) );
    pin->setPinAngle( angle );
    pin->setLength( length );
    pin->setSpace( space );
    pin->setLabelText( label );
    pin->setIsBus( type == "bus" );
    pin->setInverted( type == "inverted" || type == "inv" );
    pin->setUnused( type == "unused" || type == "nc" );
    if( type == "nul" ) pin->setPinType( Pin::pinNull );
    if( type == "rst" ) pin->setPinType( Pin::pinRst );

    //m_ePin.emplace_back( pin );
    //m_pin.emplace_back( pin );
    m_pkgePins.append( pin );
}

void SubPackage::editPin()
{
    if( !m_eventPin ) return;
    m_angle = m_eventPin->pinAngle();

    EditDialog* editDialog = new EditDialog( this, m_eventPin, nullptr );
    QObject::connect( editDialog, &EditDialog::finished, [=](int r){ editFinished(r); } );

    editDialog->exec();
    editDialog->deleteLater();
}

void SubPackage::editFinished( int )
{
    /// FIXME UNDOREDO: if( m_changed ) Circuit::self()->saveState();
}

void SubPackage::deleteEventPin()
{
    if( !m_eventPin ) return;
    m_changed = true;

    m_pkgePins.removeOne( m_eventPin );

    Component::deletePin( m_eventPin );
    m_eventPin = nullptr;
    
    Circuit::self()->update();
}

void SubPackage::setPinId( QString id )
{
    m_eventPin->setPinId( id.replace("-","") );
    m_changed = true;
}

void SubPackage::setPinName( QString name )
{
    m_eventPin->setLabelText( name );
    m_changed = true;
}

void SubPackage::setPinAngle( int i )
{
    int angle = 0;
    if     ( i == 1 ) angle = 180;
    else if( i == 2 ) angle = 90;
    else if( i == 3 ) angle = 270;
    m_eventPin->setPinAngle( angle );
    m_eventPin->setLabelPos();
    update();
}

void SubPackage::setPinSpace( double space )
{
    m_eventPin->setSpace( space );
}

void SubPackage::invertPin( bool invert )
{
    m_eventPin->setInverted( invert );
    Circuit::self()->update();
    m_changed = true;
}

void SubPackage::unusePin( bool unuse )
{
    m_eventPin->setUnused( unuse );
    Circuit::self()->update();
    m_changed = true;
}

void SubPackage::setPointPin( bool point )
{
    int length = 8;
    if( point ) length = 1;
    int oldLength = m_eventPin->length();
    if( length == oldLength ) return;

    int deltaL = 8;
    if( oldLength > 1 ) deltaL = -8;

    m_eventPin->setLength( length );

    if     ( m_angle == 180 ) m_eventPin->moveBy(-deltaL, 0 );// Left
    else if( m_angle == 0 )   m_eventPin->moveBy( deltaL, 0 );// Right
    else if( m_angle == 90 )  m_eventPin->moveBy( 0,-deltaL );// Top
    else if( m_angle == 270 ) m_eventPin->moveBy( 0, deltaL );// Bottom

    Circuit::self()->update();
    m_changed = true;
}

void SubPackage::setBusPin( bool bus )
{
    m_eventPin->setIsBus( bus );

    Circuit::self()->update();
    m_changed = true;
}

void SubPackage::setBckGndData( QString data )
{
    m_bckGndData = data;
    Chip::setBckGndData( data );
    updtProperties();
}

void SubPackage::embeedBackground( QString pixmapPath )
{
    QByteArray ba = fileToByteArray( pixmapPath, "SubPackage::setBackground");
    QString bckData( ba.toHex() );
    setBckGndData( bckData );
}

void SubPackage::setBackground( QString bck )
{
    m_background = bck;

    if( bck.isEmpty() ) setBckGndData("");
    else                Chip::setBackground( bck );
    update();
}

void SubPackage::setLogicSymbol( bool ls )
{
    QColor labelColor = ls ? QColor( 0, 0, 0 ) : QColor( 250, 250, 200 );
    for( Pin* pin : m_pkgePins ) pin->setLabelColor( labelColor );

    Chip::setLogicSymbol( ls );
}

void SubPackage::setPackageFile( QString package )
{
    QDir circuitDir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    QString fileNameAbs = circuitDir.absoluteFilePath( package );

    if( !QFile::exists( fileNameAbs ) ) package = "";
    m_pkgeFile = circuitDir.relativeFilePath( package );
    if( package.isEmpty() ) return;

    for( Pin* pin : m_pkgePins ) deletePin( pin );
    m_pkgePins.clear();

    QString pkgText = fileToString( fileNameAbs, "SubPackage::setPackageFile");
    QString pkgStr  = convertPackage( pkgText );
    m_isLS = package.endsWith("_LS.package");
    initPackage( pkgStr );
    setLogicSymbol( m_isLS );

    m_label.setPlainText( m_name );

    Circuit::self()->update();
    m_changed = false;
}

QString SubPackage::packagePins()
{
    if( m_pkgePins.isEmpty() ) return " "; // Force to save Pins property even if empty

    QString pins;
    int pP = 1;
    for( Pin* pin : m_pkgePins ) { pins += pinStrEntry( pin ); pP++; }
    return pins;
}

void SubPackage::setPackagePins(QString pinsStr)
{
    if (!m_pkgeFile.isEmpty()) return;
    if (pinsStr.trimmed().isEmpty()) return;

    QStringView delim = QStringView(u"&#xa;");
    QStringView view{pinsStr};
    // Split on the literal "&#xa;"
    const auto pins = view.split(delim, Qt::SkipEmptyParts);

    for (QStringView pin : pins)
    {
        QVector<propStr_t> properties = parseProps(pin);
        if (properties.isEmpty()) continue;

        QStringView item = properties.takeFirst().name;
        if (item == u"Pin"_qs)       // or: if (item == "Pin")
            setPinStr(properties);
    }
}

QString SubPackage::pinStrEntry( Pin* pin )
{
    QString type;
    if     ( pin->inverted() ) type = "inv";
    else if( pin->unused()   ) type = "nc";
    else if( pin->isBus()    ) type = "bus";
    else if( pin->pinType() == Pin::pinNull ) type = "nul";
    else if( pin->pinType() == Pin::pinRst  ) type = "rst";

    QString pinStr = "Pin";
    pinStr += "; type="  +type;
    pinStr += "; xpos="  +QString::number( pin->x() );
    pinStr += "; ypos="  +QString::number( pin->y() );
    pinStr += "; angle=" +QString::number( pin->pinAngle() );
    pinStr += "; length="+QString::number( pin->length() );
    pinStr += "; space=" +QString::number( pin->space() );
    pinStr += "; id="    +pin->pinId().split("-").last();
    pinStr += "; label=" +pin->getLabelText();

    return pinStr+"&#xa;";
}

void SubPackage::slotSave()
{
    QDir pdir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    QString pkgeFile = pdir.absoluteFilePath( m_pkgeFile );

    const QString dir = pkgeFile;
    QString fileName = QFileDialog::getSaveFileName( 0l, tr("Save Package"), dir,
                                                     tr("Packages (*.package);;All files (*.*)"));
    if( fileName.isEmpty() ) return;
    savePackage( fileName );
}

void SubPackage::loadPackage()
{
    QDir pkgDir;
    QString dir;

    if( m_lastPkg == "" )
    {
        pkgDir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
        dir = pkgDir.absoluteFilePath( m_pkgeFile );
    }else{
        pkgDir = QFileInfo( m_lastPkg ).absoluteDir();
        dir = pkgDir.absolutePath();
    }
    QString fileName = QFileDialog::getOpenFileName( 0l, tr("Load Package File"), dir,
                       tr("Packages (*.package);;All files (*.*)"));

    if( fileName.isEmpty() ) return; // User cancels loading

    Circuit::self()->saveCompChange( m_id, "Package_File", fileName );

    QDir pdir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    QString pkgeFile = pdir.relativeFilePath( fileName );
    setPackageFile( pkgeFile );
    m_lastPkg = fileName;
}

void SubPackage::savePackage( QString fileName )
{
    if( !fileName.endsWith(".package") ) fileName.append(".package");

    QFile file( fileName );

    if( !file.open(QFile::WriteOnly | QFile::Text) )
    {
          QMessageBox::warning(0l, "Circuit::saveCircuit",
          tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
          return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    //out << "<!DOCTYPE SimulIDE>\n\n";
    out << "<!-- This file was generated by SimulIDE -->\n\n";
    out << "<packageB name=\""  + m_name
           +"\" width=\""       + QString::number( m_width )
           +"\" height=\""      + QString::number( m_height )
           +"\" background=\""  + m_background
           +"\" custom_color=\""+ (m_customColor ? "true" : "false")
           +"\" bckgndcolor=\"" + m_pkgColor.name()
           +"\" border=\""      + (m_border ? "true" : "false")
           +"\" type=\""        + subcTypeStr()
           +"\" logic_symbol=\""+ (m_isLS ? "true" : "false")
           +"\" >\n\n";

    int pP = 1;
    for( Pin* pin : m_pkgePins ) { out << pinEntry( pin ); pP++; }

    out << "    \n";
    out << "</packageB>\n";

    file.close();
    QApplication::restoreOverrideCursor();

    QDir dir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();

    m_pkgeFile = dir.relativeFilePath( fileName );
    m_lastPkg = fileName;
    m_changed = false;
}

QString SubPackage::pinEntry( Pin* pin )
{
    QString xpos   = "xpos=\""  +QString::number( pin->x() )+"\"";
    QString ypos   = "ypos=\""  +QString::number( pin->y() )+"\"";
    QString angle  = "angle=\"" +QString::number( pin->pinAngle() )+"\"";
    QString length = "length=\""+QString::number( pin->length() )+"\"";
    QString id     = "id=\""    +pin->pinId().split("-").last().replace("<","&lt;").replace( " ", "" )+"\"";
    QString label  = "label=\"" +pin->getLabelText().replace("<","&lt;")+"\""; //.replace("<","&#x3C;").replace("=","&#x3D;").replace(">","&#x3E;")+"\"";
    QString space  = "space=\"" +QString::number( pin->space() )+"\"";
    QString type;
    if     ( pin->unused()   ) type = "nc";
    else if( pin->isBus()    ) type = "bus";
    else if( pin->inverted() ) type = "inv";
    else if( pin->pinType() == Pin::pinNull ) type = "nul";
    else if( pin->pinType() == Pin::pinRst  ) type = "rst";

    type = "type=\""+type+"\"";

    return "    <pin "
            +adjustSize( type, 11 )
            +adjustSize( xpos, 12 )
            +adjustSize( ypos, 12 )
            +adjustSize( angle, 12 )
            +adjustSize( length, 12 )
            +adjustSize( space, 12 )
            +adjustSize( id, 10 )
            +adjustSize( label, 12 )
            +"/>\n";
}

void SubPackage::setCustomColor( bool c )
{
    m_customColor = c;
    updateColor();
    updtProperties();
}

void SubPackage::slotProperties()
{
    Component::slotProperties();
    updtProperties();
}

void SubPackage::updtProperties()
{
    if( !m_propDialog ) return;

    m_propDialog->showProp("BckGndColor", m_customColor );
    m_propDialog->showProp("Border", m_hasBckGndData );
    m_propDialog->adjustWidgets();
}

QString SubPackage::adjustSize( QString str, int size )
{
    while( str.length() < size ) str.append(" ");
    return str;
}

void SubPackage::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Chip::paint( p, o, w );

    //p->setPen( Qt::black );
    //p->setBrush( Qt::transparent );
    //p->drawRoundedRect( m_area, 1, 1);

    if( m_fakePin )
    {
        QPen pen = p->pen();
        pen.setWidth( 2 );
        pen.setColor( Qt::gray );
        p->setPen(pen);
        p->drawLine( m_p1X, m_p1Y, m_p2X, m_p2Y);
}   }

//_____________________________________________________________
//_____________________________________________________________

#undef tr
#define tr(str) simulideTr("EditDialog",str)

EditDialog::EditDialog( SubPackage* pack, Pin* eventPin, QWidget* parent )
          : QDialog( parent, Qt::WindowCloseButtonHint )
{
    m_package = pack;

    m_nameLabel = new QLabel( tr("Pin Name:") );
    m_nameLineEdit = new QLineEdit;
    m_nameLineEdit->setText( eventPin->getLabelText() );
    m_nameLabel->setBuddy( m_nameLineEdit );
    QHBoxLayout* nameLayout = new QHBoxLayout;
    nameLayout->addWidget( m_nameLabel );
    nameLayout->addWidget( m_nameLineEdit );

    m_idLabel = new QLabel( tr("Pin Id:   ") );
    m_idLineEdit = new QLineEdit;
    m_idLineEdit->setText( eventPin->pinId().split( "-" ).last() );
    m_idLabel->setBuddy( m_idLineEdit );
    QHBoxLayout* idLayout = new QHBoxLayout;
    idLayout->addWidget( m_idLabel );
    idLayout->addWidget( m_idLineEdit );

    m_spaceLabel = new QLabel( tr("Space to Label:") );
    m_spaceBox   = new QDoubleSpinBox();
    m_spaceBox->setValue( eventPin->space() );
    QHBoxLayout* spaceLayout = new QHBoxLayout;
    spaceLayout->addWidget( m_spaceLabel );
    spaceLayout->addWidget( m_spaceBox );

    int angle = eventPin->pinAngle();
    int i = 0;
    if     ( angle == 180 ) i = 1;
    else if( angle == 90  ) i = 2;
    else if( angle == 270 ) i = 3;
    m_angleLabel = new QLabel( tr("Pin Angle:") );
    m_angleBox = new QComboBox();
    m_angleBox->addItems( {tr("Right"), tr("Left"), tr("Top"), tr("Bottom")} );
    m_angleBox->setCurrentIndex( i );
    QHBoxLayout* angleLayout = new QHBoxLayout;
    angleLayout->addWidget( m_angleLabel );
    angleLayout->addWidget( m_angleBox );

    m_invertCheckBox = new QCheckBox(tr("Invert Pin"));
    m_invertCheckBox->setChecked( eventPin->inverted() );
    m_unuseCheckBox  = new QCheckBox(tr("Unused Pin"));
    m_unuseCheckBox->setChecked( eventPin->unused() );

    m_pointCheckBox = new QCheckBox(tr("Point Pin"));
    m_pointCheckBox->setChecked( (eventPin->length() < 7) );

    m_busCheckBox = new QCheckBox(tr("Bus Pin"));
    m_busCheckBox->setChecked( eventPin->isBus() );

    QDialogButtonBox* bb = new QDialogButtonBox( QDialogButtonBox::Ok );
    QPushButton* okBtn = bb->button(QDialogButtonBox::Ok);
    okBtn->setAutoDefault(true);
    okBtn->setDefault(true);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addLayout( nameLayout );
    layout->addLayout( idLayout );
    layout->addLayout( spaceLayout );
    layout->addLayout( angleLayout );
    layout->addWidget( m_invertCheckBox );
    layout->addWidget( m_unuseCheckBox );
    layout->addWidget( m_pointCheckBox );
    layout->addWidget( m_busCheckBox );
    layout->addWidget( bb );

    QFontMetrics fm(m_nameLabel->font());
    double scale = fm.horizontalAdvance(" ") / 2.0;

    m_nameLineEdit->setFixedWidth( 60*scale );
    m_idLineEdit->setFixedWidth( 60*scale );
    m_spaceBox->setFixedWidth( 60*scale );
    m_angleBox->setFixedWidth( 60*scale );

    setLayout( layout );
    setWindowTitle( tr("Edit Pin ")+eventPin->getLabelText() );

    QObject::connect( bb, &QDialogButtonBox::accepted, [=](){ accept(); } );

    QObject::connect( m_nameLineEdit, &QLineEdit::textChanged,
                      [=](const QString &s){ m_package->setPinName(s); } );

    QObject::connect( m_idLineEdit, &QLineEdit::textEdited,
                      [=](const QString &s){ m_package->setPinId(s); } );

    QObject::connect( m_spaceBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                      [=](double s){ m_package->setPinSpace(s); } );

    QObject::connect( m_angleBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                      [=](int a){ m_package->setPinAngle(a); }  );

    QObject::connect( m_invertCheckBox, &QCheckBox::toggled,
                      [=](bool t){ m_package->invertPin(t); } );

    QObject::connect( m_unuseCheckBox,  &QCheckBox::toggled,
                      [=](bool t){ m_package->unusePin(t); } );

    QObject::connect( m_pointCheckBox,  &QCheckBox::toggled,
                      [=](bool t){ this->setPointPin(t); } );

    QObject::connect( m_busCheckBox,  &QCheckBox::toggled,
                      [=](bool t){ this->setBusPin(t); } );
}

void EditDialog::setPointPin( bool p )
{
    m_package->setPointPin( p );
    if( m_invertCheckBox->isChecked() ) m_invertCheckBox->setChecked( !p );
}

void EditDialog::setBusPin( bool b )
{
    m_package->setBusPin( b );
    if( m_invertCheckBox->isChecked() ) m_invertCheckBox->setChecked( !b );
}

