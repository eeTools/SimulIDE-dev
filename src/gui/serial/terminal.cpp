/***************************************************************************
 *   Copyright (C) 2026 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QTextStream>
#include <QFileDialog>
#include <QDebug>

#include "terminal.h"
#include "simulator.h"

Terminal::Terminal( QWidget* parent )
        : QDialog( parent )
        , Updatable()
{
    setupUi( this );

    splitter->setSizes({200,100});

    Simulator::self()->addToUpdateList( this );
}

void Terminal::updateStep()
{
    if( m_textBuffer.isEmpty() ) return;

    dataTextEdit->insertHtml( m_textBuffer.replace("\n", "<br>") );
    dataTextEdit->ensureCursorVisible();
    m_textBuffer.clear();

    //if( this->document()->characterCount() > 100000 )
    //    setPlainText( this->toPlainText().right( 90000 ) );
    //
    //moveCursor( QTextCursor::End );
}

void Terminal::on_sendButton_clicked()
{
    QString mode = sendBox->currentText();
    if   ( mode == "ASCII" ) sendText();
    else if( mode == "HEX" ) sendValue( 16 );
    else if( mode == "DEC" ) sendValue( 10 );
    else if( mode == "OCT" ) sendValue( 8 );
    else if( mode == "BIN" ) sendValue( 2 );
}

void Terminal::sendText()
{
    QString data = inputTextEdit->toPlainText();
    emit sendBytes( data.toUtf8() );
    m_textBuffer.append("<br><font color='yellow'>"+ data +"</font><br>");
}

void Terminal::sendValue( int base )
{
    QString hexText = inputTextEdit->toPlainText();
    if( hexText.isEmpty() ) return;

    QStringList parts = hexText.split(' ', Qt::SkipEmptyParts );
    hexText.clear();
    QByteArray data;
    bool ok;
    for( QString part : parts ) {
        uint8_t val = part.toUInt( &ok, base );
        data.append( val );
        hexText.append( part );
        hexText.append(" ");
    }
    emit sendBytes( data );
    m_textBuffer.append("<br><font color='yellow'>"+ hexText +"</font><br>");
    //dataTextEdit->insertHtml("<font color='yellow'>Sent Hex: " + hexText + "</font>");
}

void Terminal::received(uint8_t byte  )
{
    QString mode = printBox->currentText();

    if( mode == "ASCII") {
        QString text;
        text.append( QChar(byte) );
        m_textBuffer.append( text );
    } else {
        int base = 16;
        int width = 2;
        if( mode == "HEX") {
            base = 16;
            width = 2;
        } else if( mode == "DEC") {
            base = 10;
            width = 3;
        } else if( mode == "OCT") {
            base = 8;
            width = 3;
        } else if( mode == "BIN") {
            base = 2;
            width = 8;
        }
        QString num = QString::number( byte, base );
        num = num.rightJustified( width, '0');
        num += " ";

        if( mode == "HEX") num = num.toUpper();
        m_textBuffer.append( num );
    }
}

void Terminal::on_loadFileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Send File", "", "All Files (*)");
    if( fileName.isEmpty()) return;

    QFile file(fileName);
    if( file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in( &file );
        QString content = in.readAll();
        file.close();
        inputTextEdit->setPlainText(content);
    } else {
        qDebug() << "Cannot open file: " << file.errorString();
    }
}

void Terminal::on_clearSend_clicked()
{
    inputTextEdit->clear();
}

void Terminal::on_clearReceive_clicked()
{
    dataTextEdit->clear();
}

void Terminal::on_saveLogButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName( this, "Save Log", "", "Text Files (*.txt);;All Files (*)");
    if( fileName.isEmpty()) return;

    QFile file( fileName );
    if( file.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        QTextStream out( &file );
        out << dataTextEdit->toPlainText();
        file.close();
    } else {
        qDebug() << "Cannot save file" <<  file.errorString();
    }
}

void Terminal::closeEvent( QCloseEvent* event )
{
    event->accept();
    emit closed();
}
