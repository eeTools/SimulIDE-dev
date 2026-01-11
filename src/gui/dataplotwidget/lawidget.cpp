/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QMouseEvent>
#include <QFileDialog>

#include "lawidget.h"
#include "logicanalizer.h"
#include "mainwindow.h"
#include "circuit.h"
#include "utils.h"

LaWidget::LaWidget( QWidget* parent , LAnalizer* la )
        : QDialog( parent )
{
    setupUi(this);

    setWindowFlags( Qt::Window | Qt::WindowTitleHint | Qt::Tool
                   | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint );

    m_analizer = la;
    m_blocked = false;
    m_action = actNone;

    busGroup->setId( busBox_1, 0 );
    busGroup->setId( busBox_2, 1 );
    busGroup->setId( busBox_3, 2 );
    busGroup->setId( busBox_4, 3 );
    busGroup->setId( busBox_5, 4 );
    busGroup->setId( busBox_6, 5 );
    busGroup->setId( busBox_7, 6 );
    busGroup->setId( busBox_8, 7 );

    for( int i=0; i<4; ++i )
    {
        QString color = m_analizer->getColor( i ).name();
        busGroup->button( i )->setStyleSheet( "background:"+color );
        busGroup->button( i+4 )->setStyleSheet( "background:"+color );
    }
    timeDivBox->setUnitStr( "s" );
    timePosBox->setUnitStr( "s" );
}
LaWidget::~LaWidget(){}

void LaWidget::on_timeDivDial_valueChanged( int DialPos )
{
    uint64_t timeDiv = m_analizer->timeDiv();
    uint64_t   delta = timeDiv/100;
    if( delta < 1 ) delta = 1;

    if( DialPos < m_timeDivDialPos ) timeDiv += delta;
    else                             timeDiv -= delta;

    m_analizer->setTimeDiv( timeDiv );
    m_timeDivDialPos = DialPos;
}

void LaWidget::on_timeDivBox_valueChanged( double timeDiv ) // User entered value
{
    if( m_blocked ) return;

    QString unit = timeDivBox->suffix().remove("s");
    unitToVal( timeDiv, unit );
    m_analizer->setTimeDiv( timeDiv );
}

void LaWidget::updateTimeDivBox( uint64_t timeDiv )
{
    m_blocked = true;
    double val = timeDiv;
    QString unit = " p";
    int Vdecimals = 0;
    valToUnit( val, unit, Vdecimals );

    timeDivBox->setDecimals( Vdecimals );
    timeDivBox->setValue( val );
    timeDivBox->setSuffix( unit+"s" );
    m_blocked = false;
}

void LaWidget::on_timePosDial_valueChanged( int DialPos )
{
    int64_t timePos = m_analizer->timePos();
    int64_t   delta = m_analizer->timeDiv()/100;
    if( delta < 1 ) delta = 1;

    if( DialPos < m_timePosDialPos ) timePos += delta;
    else                             timePos -= delta;

    m_analizer->setTimePos( timePos );
    m_timePosDialPos = DialPos;
}

void LaWidget::on_timePosBox_valueChanged( double timePos )
{
    if( m_blocked ) return;

    QString unit = timePosBox->suffix().remove("s");
    unitToVal( timePos, unit );
    m_analizer->setTimePos( timePos );
}

void LaWidget::updateTimePosBox( int64_t timePos )
{
    m_blocked = true;

    double val = timePos;
    QString unit = " p";
    int Vdecimals = 0;
    valToUnit( val, unit, Vdecimals )

    timePosBox->setDecimals( Vdecimals );
    timePosBox->setValue( val );
    timePosBox->setSuffix( unit+"s" );
    m_blocked = false;
}

void LaWidget::on_busGroup_buttonClicked( int ch )
{
    m_analizer->setIsBus( ch, busGroup->button( ch )->isChecked() );
}

void LaWidget::setIsBus( int ch, bool b )
{
    busGroup->button( ch )->setChecked( b );
    m_analizer->setIsBus( ch, b );
}

void LaWidget::on_rThresholdBox_valueChanged( double v )
{
    if( m_blocked ) return;
    m_analizer->setThresholdR( v );
}

void LaWidget::updateThresholdR( double v )
{
    m_blocked = true;
    rThresholdBox->setValue( v );
    m_blocked = false;
}

void LaWidget::on_fThresholdBox_valueChanged( double v )
{
    if( m_blocked ) return;
    m_analizer->setThresholdF( v );
}

void LaWidget::updateThresholdF( double v )
{
    m_blocked = true;
    fThresholdBox->setValue( v );
    m_blocked = false;
}

void LaWidget::on_triggerBox_currentIndexChanged( int index )
{ m_analizer->setTrigger( index ); }


void LaWidget::on_condEdit_editingFinished()
{ m_analizer->updateConds( condEdit->text() ); }

void LaWidget::on_exportData_clicked()
{
    QString fileName = QFileDialog::getSaveFileName( this, tr("Export Data"), m_analizer->getExportFile(),
                                                     tr("VCD files (*.vcd);;All files (*.*)") );
    if( fileName.isEmpty() ) return;
    if( !fileName.endsWith(".vcd") ) fileName += ".vcd";

    m_analizer->dumpData( fileName );
}

void LaWidget::closeEvent( QCloseEvent* event )
{
    if( !parent() ) return;
    m_analizer->expand( false );
    QWidget::closeEvent( event );
}

void LaWidget::mousePressEvent( QMouseEvent* event )
{
    m_mousePos = event->globalX();
    if( event->button() == Qt::LeftButton )
    {
        m_action = actMove;
        setCursor( Qt::ClosedHandCursor );
    }
    else if( event->button() == Qt::MiddleButton )
    {
        m_action = actTime;
        plotDisplay->setTimeZero( m_mousePos );
    }
}

void LaWidget::mouseMoveEvent( QMouseEvent* event )
{
    int pos = event->globalX();

    if( m_action == actMove )
    {
        double timePos = m_analizer->timePos();
        double timeX = m_analizer->timeDiv()*10;
        double sizeX = plotDisplay->sizeX();
        double deltaT = (m_mousePos - pos)*timeX/sizeX;
        m_analizer->setTimePos( timePos+deltaT );
    }
    m_mousePos = pos;
}

