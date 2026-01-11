/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QWheelEvent>

#include "oscwidget.h"
#include "oscope.h"
#include "utils.h"
#include "mainwindow.h"

OscWidget::OscWidget( QWidget* parent , Oscope* oscope )
         : QDialog( parent )
{
    setupUi(this);

    setWindowFlags( Qt::Window | Qt::WindowTitleHint | Qt::Tool
                   | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint );

    m_oscope = oscope;
    m_blocked = false;
    m_action = actNone;
    m_channel = 4;
    QString color = m_oscope->getColor( m_channel ).name();

    channelGroup->setId( chButton1, 0 );
    channelGroup->setId( chButton2, 1 );
    channelGroup->setId( chButton3, 2 );
    channelGroup->setId( chButton4, 3 );
    channelGroup->setId( allButton, 4 );

    autoGroup->addButton( autoCheck1, 0 );
    autoGroup->addButton( autoCheck2, 1 );
    autoGroup->addButton( autoCheck3, 2 );
    autoGroup->addButton( autoCheck4, 3 );
    autoGroup->addButton( autoCheck5, 4 );
    autoGroup->button( 4 )->setChecked( true );
    autoLabel->setStyleSheet( "background-color:"+color );

    triggerGroup->addButton( triggerCheck1, 0 );
    triggerGroup->addButton( triggerCheck2, 1 );
    triggerGroup->addButton( triggerCheck3, 2 );
    triggerGroup->addButton( triggerCheck4, 3 );
    triggerGroup->addButton( triggerCheck5, 4 );
    triggerGroup->button( 4 )->setChecked( true );
    trigLabel->setStyleSheet( "background-color:"+color );

    hideGroup->addButton( hideCheck1, 0 );
    hideGroup->addButton( hideCheck2, 1 );
    hideGroup->addButton( hideCheck3, 2 );
    hideGroup->addButton( hideCheck4, 3 );
    hideGroup->addButton( hideCheck5, 4 );
    hideGroup->button( 4 )->setChecked( true );
    //hideLabel->setStyleSheet( "background-color:"+color );

    trackGroup->addButton( trackCheck1, 1 );
    trackGroup->addButton( trackCheck2, 2 );
    trackGroup->addButton( trackCheck4, 4 );
    trackGroup->button( 1 )->setChecked( true );

    for( int i=0; i<5; ++i )
    {
        QString color = m_oscope->getColor( i ).name();
        channelGroup->button( i )->setStyleSheet("QPushButton:checked{background-color:"+color+"; border: 3px solid gray; border-radius: 2px; border-style: inset;}\
                                                  QPushButton{background-color:"+color+"; }" );
        channelGroup->button( i )->adjustSize();
        autoGroup->button( i )->setStyleSheet( "background-color:"+color );
        triggerGroup->button( i )->setStyleSheet( "background-color:"+color );
        hideGroup->button( i )->setStyleSheet( "background-color:"+color );
    }
    mainLayout->setDirection( QBoxLayout::RightToLeft );

    timeDivBox->setUnitStr( "s" );
    timePosBox->setUnitStr( "s" );
    voltDivBox->setUnitStr( "V" );
    voltPosBox->setUnitStr( "V" );

    on_channelGroup_buttonClicked( 4 ); // Avoid size changes at first click
    allButton->setChecked( true );
}
OscWidget::~OscWidget(){}

void OscWidget::on_timeDivDial_valueChanged( int DialPos )
{
    if( m_oscope->autoSC() > 3 )
    {
        uint64_t timeDiv = m_oscope->timeDiv();
        uint64_t   delta = timeDiv/100;
        if( delta < 1 ) delta = 1;

        if( DialPos < m_timeDivDialPos ) timeDiv += delta;
        else                             timeDiv -= delta;

        m_oscope->setTimeDiv( timeDiv );
    }
    m_timeDivDialPos = DialPos;
}

void OscWidget::on_timeDivBox_valueChanged( double timeDiv ) // User entered value
{
    if( m_blocked ) return;
    if( m_oscope->autoSC() < 4 ) return;

    QString unit = timeDivBox->suffix().remove("s");
    unitToVal( timeDiv, unit );
    m_oscope->setTimeDiv( timeDiv );
}

void OscWidget::updateTimeDivBox( uint64_t timeDiv )
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

void OscWidget::on_timePosDial_valueChanged( int DialPos )
{
    int ch = m_channel;
    int aut = m_oscope->autoSC();
    if( aut == 4 || aut != ch )
    {
        bool nextCh = false;
        if( ch == 4 ) { ch = 0; nextCh = true; }

        int64_t timePos = m_oscope->timePos( ch );
        int64_t    delta = m_oscope->timeDiv()/100;
        if( delta < 1 ) delta = 1;

        if( DialPos < m_timePosDialPos ) timePos += delta;
        else                             timePos -= delta;

        if( nextCh )
            for( int i=1; i<4; ++i ) m_oscope->setTimePos( i, timePos );
        m_oscope->setTimePos( ch, timePos );
    }
    m_timePosDialPos = DialPos;
}

void OscWidget::on_timePosBox_valueChanged( double timePos )
{
    if( m_blocked ) return;
    int ch = m_channel;
    if( ch < 4 && m_oscope->autoSC() == ch ) return;

    QString unit = timePosBox->suffix().remove("s");
    unitToVal( timePos, unit );

    if( ch == 4 ){
        ch = 0;
        for( int i=1; i<4; ++i ) m_oscope->setTimePos( i, timePos );
    }
    m_oscope->setTimePos( ch, timePos );
}

void OscWidget::updateTimePosBox( int ch, int64_t timePos)
{
    m_blocked = true;
    if( m_channel < 4 && m_channel != ch ) return;

    double val = timePos;
    QString unit = " p";
    int Vdecimals = 0;
    valToUnit( val, unit, Vdecimals )

    timePosBox->setDecimals( Vdecimals );
    timePosBox->setValue( val );
    timePosBox->setSuffix( unit+"s" );
    m_blocked = false;
}

void OscWidget::on_voltDivDial_valueChanged( int DialPos )
{
    int ch = m_channel;
    int aut = m_oscope->autoSC();
    if( aut == 4 || aut != ch )
    {
        bool nextCh = false;
        if( ch == 4 ) { ch = 0; nextCh = true; }

        double voltDiv = m_oscope->voltDiv( ch );
        double delta = voltDiv/100;
        if( DialPos < m_voltDivDialPos ) voltDiv += delta;
        else                             voltDiv -= delta;

        if( nextCh )
            for( int i=1; i<4; ++i ) m_oscope->setVoltDiv( i, voltDiv );
        m_oscope->setVoltDiv( ch, voltDiv );
    }
    m_voltDivDialPos = DialPos;
}

void OscWidget::on_voltDivBox_valueChanged( double voltDiv )
{
    if( m_blocked ) return;
    int ch = m_channel;
    int aut = m_oscope->autoSC();
    if( aut < 4 && aut == ch ) return;

    QString unit = voltDivBox->suffix().remove("V");
    unitToVal( voltDiv, unit );

    if( ch == 4 ){
        ch = 0;
        for( int i=1; i<4; ++i ) m_oscope->setVoltDiv( i, voltDiv/1e12 );
    }
    m_oscope->setVoltDiv( ch, voltDiv/1e12 );
}

void OscWidget::updateVoltDivBox( int ch, double voltDiv )
{
    m_blocked = true;
    if( m_channel < 4 && m_channel != ch ) return;

    double val  = voltDiv*1e12;
    QString unit = " p";
    int Vdecimals = 0;
    valToUnit( val, unit, Vdecimals );

    voltDivBox->setDecimals( Vdecimals );
    voltDivBox->setValue( val );
    voltDivBox->setSuffix( unit+"V" );
    m_blocked = false;
}

void OscWidget::on_voltPosDial_valueChanged( int DialPos )
{
    int ch = m_channel;
    int aut = m_oscope->autoSC();
    if( aut == 4 || aut != ch )
    {
        bool nextCh = false;
        if( ch == 4 ) { ch = 0; nextCh = true; }

        double voltPos = m_oscope->voltPos( ch );
        double   delta = m_oscope->voltDiv( ch )/100;

        if( DialPos < m_voltPosDialPos ) voltPos -= delta;
        else                             voltPos += delta;

        if( nextCh )
            for( int i=1; i<4; ++i ) m_oscope->setVoltPos( i, voltPos );
        m_oscope->setVoltPos( ch, voltPos );
    }
    m_voltPosDialPos = DialPos;
}

void OscWidget::on_voltPosBox_valueChanged( double voltPos )
{
    if( m_blocked ) return;
    int ch = m_channel;

    QString unit = voltPosBox->suffix().remove("V");
    unitToVal( voltPos, unit );

    if( ch == 4 ){
        ch = 0;
        for( int i=1; i<4; ++i ) m_oscope->setVoltPos( i, voltPos/1e12 );
    }
    m_oscope->setVoltPos( ch, voltPos/1e12 );
}

void OscWidget::updateVoltPosBox(int ch, double voltPos )
{
    m_blocked = true;
    if( m_channel < 4 && m_channel != ch ) return;
    double  val  = voltPos*1e12;
    QString unit = " p";
    int Vdecimals = 0;
    valToUnit( val, unit, Vdecimals )

    voltPosBox->setDecimals( Vdecimals );
    voltPosBox->setValue( val );
    voltPosBox->setSuffix( unit+"V" );
    m_blocked = false;
}

void OscWidget::on_filterBox_valueChanged( double filter )
{
    if( m_blocked ) return;
    m_blocked = true;
    m_oscope->setFilter( filter );
    m_blocked = false;
}

void OscWidget::setFilter( double filter )
{
    filterBox->setValue( filter );
}

void OscWidget::on_channelGroup_buttonClicked( int ch )
{
    m_channel = ch;

    QString color = m_oscope->getColor( ch ).name();
    timePosBox->setStyleSheet( "background-color:"+color );
    voltDivBox->setStyleSheet( "background-color:"+color );
    voltPosBox->setStyleSheet( "background-color:"+color );

    if( ch < 4 ){
        updateTimePosBox( ch, m_oscope->timePos( ch ) );
        updateVoltDivBox( ch, m_oscope->voltDiv( ch ) );
        updateVoltPosBox( ch, m_oscope->voltPos( ch ) );
}    }

void OscWidget::on_autoGroup_buttonClicked( int ch )
{ m_oscope->setAutoSC( ch ); }

void OscWidget::setAuto( int ch )
{
    autoGroup->button( ch )->setChecked( true );
    QString color = m_oscope->getColor( ch ).name();
    autoLabel->setStyleSheet( "background-color:"+color );
}

void OscWidget::on_triggerGroup_buttonClicked( int ch )
{ m_oscope->setTrigger( ch ); }

void OscWidget::setTrigger( int ch )
{
    triggerGroup->button( ch )->setChecked( true );
    QString color = m_oscope->getColor( ch ).name();
    trigLabel->setStyleSheet( "background-color:"+color );
}

void OscWidget::on_hideGroup_buttonClicked( int ch )
{
    if( ch == 4 )
    {
        for( int i=0; i<4; ++i ) m_oscope->hideChannel( i, false );
        hideChannel( ch, hideGroup->button(ch)->isChecked() );
    }
    else m_oscope->hideChannel( ch, hideGroup->button(ch)->isChecked() );
}

void OscWidget::hideChannel( int ch, bool hide )
{
    hideGroup->button( ch )->setChecked( hide );
    if( ch<4 && hide ) hideGroup->button( 4 )->setChecked( false );
}

void OscWidget::on_trackGroup_buttonClicked( int ch )
{ m_oscope->setTracks( ch ); }

void OscWidget::closeEvent( QCloseEvent* event )
{
    if( !parent() ) return;
    m_oscope->expand( false );
    QWidget::closeEvent( event );
}

void OscWidget::mousePressEvent( QMouseEvent* event )
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

void OscWidget::mouseMoveEvent( QMouseEvent* event )
{
    int pos = event->globalX();
    if( m_action == actMove )
    {
        double timeX = m_oscope->timeDiv()*10;
        double sizeX = plotDisplay->sizeX();
        double deltaT = (m_mousePos - pos)*timeX/sizeX;

        for( int i=0; i<4; ++i )
            m_oscope->setTimePos( i, m_oscope->timePos( i )+deltaT );
    }
    m_mousePos = pos;
}

