/***************************************************************************
 *   Copyright (C) 2023 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>

#include "console.h"
#include "watched.h"
#include "mainwindow.h"
#include "simulator.h"

Console::Console( Watched* cpu, QWidget* parent )
       : QPlainTextEdit( parent )
{
    m_watched = cpu;
    m_sendCommand = false;
    m_command = "";

    document()->setMaximumBlockCount( 100 );

    QFont font;
    font.setFamily("Ubuntu Mono");
    font.setWeight( QFont::Normal );
    font.setFixedPitch(true);
    font.setPixelSize( 13*MainWindow::self()->fontScale() );
    setFont( font );

    QPalette p = palette();
    p.setColor( QPalette::Base, Qt::black );
    p.setColor( QPalette::Text, QColor( 150,255,100) );
    setPalette( p);

    Simulator::self()->addToUpdateList( this );

    //appendHtml("<p style=\"color:#FFFFFF;\">></p>");
}

void Console::updateStep()
{
    if( !m_buffer.isEmpty() )
    {
        QTextCharFormat tf = currentCharFormat();
        tf.setForeground( QColor( 0xB4FF64 ) );
        setCurrentCharFormat( tf );

        insertPlainText( m_buffer );
        tf.setForeground( QColor( 0xFFFFFF ) );
        setCurrentCharFormat( tf );
        m_buffer.clear();

        //if( text.endsWith("\n")) appendHtml("<p style=\"color:#FFFFFF;\">></p>");
        QScrollBar* bar = verticalScrollBar();
        bar->setValue( bar->maximum() );
    }
    if( m_sendCommand )
    {
        m_sendCommand = false;
        if( !m_command.isEmpty() ){
            m_watched->command( m_command );
            m_command = "";
        }
    }
}

void Console::appendText( QString text )
{
    m_buffer += text;
}

void Console::appendLine( QString line )
{
    appendHtml("<p style=\"color:#B4FF64;\">" + line + "</p>");
    //appendHtml("<p style=\"color:#FFFFFF;\">></p>");

    QScrollBar* bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}

void Console::keyPressEvent( QKeyEvent* e )
{
    if( m_sendCommand ) return;
    if( !Simulator::self()->isRunning() ) return;

    switch (e->key()) {
        case Qt::Key_Backspace:
        case Qt::Key_Left:
        case Qt::Key_Right:/*{
            if( this->textCursor().positionInBlock() < 2 ) return;
            QPlainTextEdit::keyPressEvent(e);
        }break;*/
        case Qt::Key_Up:
        case Qt::Key_Down: break;
        case Qt::Key_Return:{
            m_sendCommand = true;
            QPlainTextEdit::keyPressEvent(e);
        }break;
        default:{
            if( m_command.isEmpty() ){
                QTextCharFormat tf = currentCharFormat();
                tf.setForeground( QColor( 0xFFFFFF ) );
                setCurrentCharFormat( tf );
            }
            m_command += e->text();
            QPlainTextEdit::keyPressEvent(e);
        }
    }
}

void Console::mousePressEvent( QMouseEvent* e)
{
    setFocus();
}

void Console::mouseDoubleClickEvent( QMouseEvent* e )
{
}

void Console::contextMenuEvent( QContextMenuEvent* e )
{
}
