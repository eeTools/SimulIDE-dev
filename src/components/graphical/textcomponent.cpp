/***************************************************************************
 *   Copyright (C) 2017 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QCursor>
#include <QPainter>
#include <QTextDocument>
#include <QGuiApplication>

#include "textcomponent.h"
#include "itemlibrary.h"
#include "circuit.h"
#include "simulator.h"
#include "circuit.h"
#include "circuitwidget.h"

#include "stringprop.h"
#include "doubleprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("TextComponent",str)

Component* TextComponent::construct( QString type, QString id )
{ return new TextComponent( type, id ); }

LibraryItem* TextComponent::libraryItem()
{
    return new LibraryItem(
        tr("Text"),
        "Graphical",
        "text.png",
        "TextComponent",
        TextComponent::construct );
}

TextComponent::TextComponent( QString type, QString id )
             : LinkerComponent( type, id )
{
    m_graphical = true;
    m_changed   = false;
    m_opac = 1;
    m_color = QColor( 255, 255, 220 );
    m_font  = "Helvetica [Cronyx]";

    QFont sansFont( m_font, 10 );
    #if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    sansFont.setWeight( QFont::Medium );
    #else
    sansFont.setWeight( QFont::Normal);
    #endif
    sansFont.setFixedPitch(true);

    QColor color = Qt::darkBlue;
    m_fontColor = color.name();
    m_text = new QGraphicsTextItem( this );
    m_text->setTextInteractionFlags( Qt::NoTextInteraction );
    m_text->setTextWidth( 90 );
    m_text->setFont( sansFont );
    m_textString = "... TEXT ...";
    m_text->setPlainText( m_textString );
    m_text->setPos( 0, 0 );
    m_text->document()->setTextWidth(-1);
    m_text->setDefaultTextColor( color );
    m_text->setCursor( Qt::OpenHandCursor );
    //m_text->installEventFilter( this );

    m_margin = 5;
    m_docMargin = m_text->document()->documentMargin();
    m_context = false;
    m_border = 1;
    updateGeometry( 0, 0, 0 );
    
    setFontSize( 10 );

    Simulator::self()->addToUpdateList( this );

    QObject::connect( m_text->document(), &QTextDocument::contentsChange
                    , [=](int f, int cr, int ca){ updateGeometry(f,cr,ca); } );

    addPropGroup( { tr("Main"), {
        new IntProp <TextComponent>("Margin", tr("Margin"),"_px"
                                   , this, &TextComponent::margin, &TextComponent::setMargin ),

        new IntProp <TextComponent>("Border", tr("Border"),"_px"
                                   , this, &TextComponent::border, &TextComponent::setBorder,0,"uint" ),

        new StrProp <TextComponent>("Color", tr("Background Color"),""
                                   , this, &TextComponent::colorStr,&TextComponent::setColorStr,0,"color" ),

        new DoubProp<TextComponent>("Opacity", tr("Opacity"),""
                                   , this, &TextComponent::opac, &TextComponent::setOpac )
    }, groupNoCopy} );

    addPropGroup( { tr("Text"), {
        new StrProp <TextComponent>("Font", tr("Font"),""
                                   , this, &TextComponent::getFont , &TextComponent::setFont ),

        new StrProp <TextComponent>("Font_Color", tr("Font Color"),""
                                   , this, &TextComponent::fontColor, &TextComponent::setFontColor,0,"color" ),

        new IntProp <TextComponent>("Font_Size", tr("Font Size") ,"_px"
                                   , this, &TextComponent::fontSize , &TextComponent::setFontSize ),

        new BoolProp<TextComponent>("Fixed_Width", tr("Fixed_Width"),""
                                   , this, &TextComponent::fixedW, &TextComponent::setFixedW ),

        new StrProp <TextComponent>("Text", tr("Text"),""
                                   , this, &TextComponent::getText, &TextComponent::setText,0,"textEdit" )
    }, groupNoCopy} );
}
TextComponent::~TextComponent()
{
    delete m_text;
}

void TextComponent::updateStep()
{
    if( !m_changed && m_linkedComp.isEmpty() ) return;
    m_changed = false;

    QString text = m_textString;

    for( int i=0; i<m_linkedComp.size(); ++i )
    {
        Component* comp = m_linkedComp.at( i );
        comp->update();
        QString data = comp->getValLabelText();
        text.replace( "$data"+QString::number(i), data );
    }
    m_text->document()->setPlainText( text );
}

void TextComponent::updateGeometry(int, int, int)
{
    m_text->document()->setTextWidth(-1);
    
    int margin = m_margin;
    if( margin < 0 ) margin = 0;
    m_area = QRect(-margin,-margin, m_text->boundingRect().width()+margin*2, m_text->boundingRect().height()+margin*2 );
    
    Circuit::self()->update();
}

int TextComponent::margin() { return m_margin; }

void TextComponent::setMargin( int margin )
{
    // Maintain compatibility with older versions:
    if( margin < 0 ) m_text->document()->setDocumentMargin( m_docMargin+margin );
    else             m_text->document()->setDocumentMargin( m_docMargin );
    m_margin = margin;
    updateGeometry( 0, 0, 0 );
}

void TextComponent::setColorStr( QString color )
{
    m_color = QColor( color);
    update();
}

void TextComponent::setOpac( qreal op )
{
    if     ( op > 1 ) op = 1;
    else if( op < 0 ) op = 0;
    m_opac = op;
    update();
}

void TextComponent::setFont( QString font )
{
    if( font == "" ) return;
    m_font = font;

    QFont Tfont = m_text->font();
    Tfont.setFamily( font );
    m_text->setFont( Tfont );
    updateGeometry( 0, 0, 0 );
}

void TextComponent::setFontSize( int size )
{
    if( size < 1 ) return;
    m_fontSize = size;

    QFont font = m_text->font();
    font.setPixelSize( size );
    m_text->setFont( font );
    updateGeometry( 0, 0, 0 );
}

void TextComponent::setFontColor( QString color )
{
    m_fontColor = color;
    m_text->setDefaultTextColor( QColor( color) );
    update();
}

void TextComponent::setFixedW( bool fixedW ) 
{ 
    m_fixedW = fixedW;
    
    QFont font = m_text->font();
    font.setFixedPitch( fixedW );
    m_text->setFont( font );
    updateGeometry( 0, 0, 0 );
}

QString TextComponent::getText()
{
    QString text = m_textString;
    return text.replace("\n", "&#xa;").replace("\"", "&#x22;")
            .replace("=", "&#x3D").replace("<", "&#x3C").replace(">", "&#x3E");
}

void TextComponent::setText( QString text )
{
    m_textString = text.replace("&#xa;", "\n").replace("&#x22;", "\"")
            .replace("&#x3D", "=").replace("&#x3C", "<").replace("&#x3E", ">");
    m_changed = true;
    if( !Simulator::self()->isRunning() ) updateStep();
}

void TextComponent::createLinks( QList<Component*>* compList )
{
    m_changed = true;
    Linker::createLinks( compList );
    if( !Simulator::self()->isRunning() ) updateStep();
}

void TextComponent::compSelected( Component* comp )
{
    m_changed = true;
    Linker::compSelected( comp );
    if( !Simulator::self()->isRunning() ) updateStep();
}

void TextComponent::setLinkedString( QString str, int i )
{
    if( i == 0 ) setText( str );
    else         setText( m_textString+str );
}

void TextComponent::setLinkedValue( double v, int i )
{
    QString str = QString::number( v );

    if( i == 0 ) setText( str );
    else         setText( m_textString+"/n"+str );
}

void TextComponent::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );

    QPen pen( Qt::black, m_border, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p->setPen( pen );

    double opacity = p->opacity();
    p->setOpacity( opacity*m_opac );
    p->drawRect( m_area );
    p->setOpacity( opacity );

    if( m_border > 0 )
    {
        p->setBrush( Qt::transparent );
        p->drawRect( m_area );
    }
    Component::paintSelected( p );
}
