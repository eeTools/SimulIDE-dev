/***************************************************************************
 *   Copyright (C) 2023 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QStandardItemModel>
#include <QGraphicsProxyWidget>
#include <math.h>
#include <QDebug>
#include <QSizeGrip>

#include "watcher.h"
#include "valuewidget.h"
#include "vallabelwidget.h"
#include "headerwidget.h"
#include "mainwindow.h"
#include "circuitview.h"
#include "e_mcu.h"
#include "watched.h"
#include "console.h"
#include "scriptcpu.h"

Watcher::Watcher( QWidget* parent, Watched* cpu, bool proxy )
       : QWidget( parent )
{
    setupUi(this);

    setWindowFlags( Qt::Window | Qt::WindowTitleHint | Qt::Tool
                   | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint );

    m_core = cpu;
    m_console = nullptr;
    m_proxy   = nullptr;
    m_header = false;

    int spacing = proxy ? 0 : 2;

    m_valuesLayout = new QBoxLayout( QBoxLayout::TopToBottom, this );
    m_valuesLayout->setContentsMargins(0, 0, 0, 0);
    m_valuesLayout->setSpacing( spacing );
    m_valuesLayout->setContentsMargins( 0, 0, 0, 4 );
    m_valuesLayout->addStretch();
    valuesWidget->setLayout( m_valuesLayout );

    if( proxy ) return;

    float scale = MainWindow::self()->fontScale();

    QFont font;
    font.setFamily("Ubuntu Mono");
    font.setBold( true );
    font.setPixelSize( round(12.5*scale) );

    m_registerModel = new QStandardItemModel( this );
    regView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    regView->setModel( m_registerModel );
    regView->setFont( font );

    m_variableModel = new QStandardItemModel( this );
    varView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    varView->setFont( font );
    varView->setModel( m_variableModel );

    splitter->setSizes( {{50,320}} );
    splitter_2->setSizes( {100,30} );

    connect( varView, &QListView::activated, this, &Watcher::VarDoubleClick );
    connect( regView, &QListView::activated, this, &Watcher::RegDoubleClick );
}

void Watcher::setProxy( QGraphicsProxyWidget* p )
{
    m_proxy = p;

    setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
    horizontalLayout->setContentsMargins( 0, 0, 0, 0 );
    setStyleSheet("background-color: rgba(0,0,0,0)");

    splitter_2->setHidden( true );
    regView->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
    varView->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
    splitter->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
    splitter_2->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
    //this->resize( 100, 50 );

    setCursor( Qt::OpenHandCursor );
    m_proxy->setCursor( Qt::OpenHandCursor );
}

void Watcher::addHeader()
{
    m_header = true;
    HeaderWidget* header = new HeaderWidget("Name","Type", this );
    int i = m_valuesLayout->count();
    m_valuesLayout->insertWidget( i, header );
}

void Watcher::updateValues()
{
    if( !m_core ) return;
    for( ValueWidget* vw : m_values ) vw->updateValue();
    for( ValLabelWidget* vw : m_valLabels ) vw->updateValue();
}

void Watcher::setRegisters( QStringList regs )
{
    regs.sort();
    for( QString reg : regs ) addRegister( reg, "uint8" );
}

void Watcher::addRegister( QString name, QString type, QString unit )
{
    if( m_typeTable.keys().contains( name ) ) return;
    m_typeTable[ name ] = type;
    m_unitTable[ name ] = unit;
    if( m_proxy ) insertValue( name );
    else          m_registerModel->appendRow( new QStandardItem(name) );
}

void Watcher::setVariables( QStringList vars )
{
    vars.sort();
    m_variableModel->clear();
    for( QString var : vars ) m_variableModel->appendRow( new QStandardItem(var) );
}

void Watcher::addVariable( QString name, QString type, QString unit )
{
    if( m_typeTable.keys().contains( name ) ) return;
    m_typeTable[ name ] = type;
    m_unitTable[ name ] = unit;
    if( m_proxy ) insertValue( name );
    else          m_variableModel->appendRow( new QStandardItem(name) );
}

void Watcher::loadVarSet( QStringList varSet )
{
    for( QString name : varSet ) insertValue( name );
}

QStringList Watcher::getVarSet()
{
    QStringList varset;

    for( QString name : m_values.keys() )
    {
        int index = m_valuesLayout->indexOf( m_values[name] );
        int i;
        for( i=0; i<varset.size(); i++ )
        {
            QString n = varset.at(i);
            if( index < m_valuesLayout->indexOf( m_values[n] ) ) break;
        }
        varset.insert( i, name );
    }
    return varset;
}

void Watcher::addConsole()
{
    if( m_console ) return;
    m_console = new Console( m_core );
    m_valuesLayout->insertWidget( m_valuesLayout->count()-1, m_console );
}

void Watcher::addWidget( QWidget* widget )
{
    int i = m_valuesLayout->count();
    m_valuesLayout->insertWidget( i, widget );
}

void Watcher::RegDoubleClick( const QModelIndex& index )
{
    QString name = m_registerModel->item(index.row())->text();
    insertValue( name );
}

void Watcher::VarDoubleClick( const QModelIndex& index )
{
    QString name = m_variableModel->item(index.row())->text();
    insertValue( name );
}

void Watcher::insertValue( QString name )
{
    if( m_values.keys().contains( name ) ) return;
    if( !m_proxy && !m_header ) addHeader();

    QString type = m_typeTable.value( name );
    QString unit = m_unitTable.value( name );

    int last = m_console ? 1 : 0;

    if( m_proxy )
    {
        ValLabelWidget* valwid = new ValLabelWidget( name, type, unit, m_core, this );
        m_valuesLayout->insertWidget( m_valuesLayout->count()-last, valwid );

        m_valLabels[name] = valwid;

        int sizeH = m_valLabels.size()*11+7;
        //if( sizeH < this->height() ) sizeH = this->height();

        this->setMinimumHeight( sizeH );
        this->resize( 120, sizeH );
    }
    else
    {
        ValueWidget* valwid = new ValueWidget( name, type, unit, m_core, this );
        m_values[name] = valwid;
        m_valuesLayout->insertWidget( m_valuesLayout->count()-last, valwid );
    }
}

void Watcher::mousePressEvent( QMouseEvent* event )
{
    if( m_proxy && event->button() == Qt::LeftButton )
    {
        m_mousePos = CircuitView::self()->mapToScene(
                     CircuitView::self()->mapFromGlobal( event->globalPos() ) );

        m_proxy->setCursor( Qt::ClosedHandCursor );
    }
    else QWidget::mousePressEvent( event );
}

void Watcher::mouseMoveEvent( QMouseEvent* event )
{
    if( !m_proxy )
    {
        QWidget::mouseMoveEvent( event );
        return;
    }
    QPointF pos = CircuitView::self()->mapToScene(
                     CircuitView::self()->mapFromGlobal( event->globalPos() ) );

    QPointF deltaF = pos-m_mousePos;
    QPoint delta = deltaF.toPoint();
    if( delta == QPoint(0,0) ) return;

    m_proxy->setPos( m_proxy->pos() + delta );

    m_mousePos = pos;
}

void Watcher::mouseReleaseEvent( QMouseEvent* event )
{
    if( m_proxy ) m_proxy->setCursor( Qt::OpenHandCursor );
    else          QWidget::mouseMoveEvent( event );
}

void Watcher::paintEvent( QPaintEvent *event )
{
    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing ); // smooth borders
    painter.setBrush( QBrush(Qt::transparent)) ; // visible color of background
    painter.setPen( Qt::darkGray ); // thin border color

    painter.drawRoundedRect( 2, 2, width()-4, height()-4, 5, 5 );

    QWidget::paintEvent( event );
}
