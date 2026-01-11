/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "outpaneltext.h"
#include "mainwindow.h"
#include "simulator.h"

OutPanelText::OutPanelText( QWidget* parent )
            : QPlainTextEdit( parent )
            , Updatable()
{
    m_textBuffer = "";

    m_highlighter = new OutHighlighter( document() );

    QPalette p;
    p.setColor( QPalette::Base, QColor( 35, 30, 60 ) );
    p.setColor( QPalette::Text, QColor( 190, 190, 150) );
    setPalette(p);
    
    QFont font;
    font.setFamily("Ubuntu Mono");
    font.setWeight( QFont::Normal );
    font.setFixedPitch(true);
    font.setPixelSize( 13*MainWindow::self()->fontScale() );
    setFont( font );

    this->document()->setDocumentMargin( 9 );

    setMaximumBlockCount( 1000 );
}
OutPanelText::~OutPanelText(){}

void OutPanelText::appendLine( const QString text )
{
    m_textBuffer.append( text+"\n" );
    if( !Simulator::self() || !Simulator::self()->isRunning() )
    {
        updateStep();
        repaint();
    }
}

void OutPanelText::updateStep()
{
    if( m_textBuffer.isEmpty() ) return;

    moveCursor( QTextCursor::End );
    insertPlainText( m_textBuffer );
    m_textBuffer.clear();

    if( this->document()->characterCount() > 100000 )
        setPlainText( this->toPlainText().right( 90000 ) );

    moveCursor( QTextCursor::End );
}

// CLASS OutHighlighter ***********************************************

OutHighlighter::OutHighlighter( QTextDocument* parent )
              : QSyntaxHighlighter( parent )
{
    HighlightingRule rule;
    QTextCharFormat format;

    format.setForeground( QColor(110, 180, 100) );
    rule.pattern = QRegularExpression( "/[^\n]*" );
    rule.format = format;
    highlightingRules.append( rule );

    format.setForeground( QColor(140, 150, 240) );
    rule.pattern = QRegularExpression("\\b[0-9]+\\b");
    rule.format = format;
    highlightingRules.append(rule);

    format.setForeground( QColor(140, 150, 240) );
    rule.pattern = QRegularExpression("0[xX][0-9a-fA-F]+");
    rule.format = format;
    highlightingRules.append(rule);

    format.setForeground( QColor(50, 200, 100) );
    rule.pattern = QRegularExpression( "\".*\"" );
    rule.format = format;
    highlightingRules.append( rule );

    format.setForeground( QColor(200, 255, 200) );
    format.setFontWeight( QFont::Bold );
    rule.pattern = QRegularExpression("\\bSUCCESS\\b");
    rule.format = format;
    highlightingRules.append(rule);
    
    format.setForeground( QColor(255, 255, 200) );
    rule.pattern = QRegularExpression("SENT:");
    rule.format = format;
    highlightingRules.append(rule);

    format.setForeground( QColor(255, 200, 100) );
    rule.pattern = QRegularExpression("\\bWARNING\\b");
    rule.format = format;
    highlightingRules.append(rule);

    format.setForeground( QColor(100, 50, 0) );
    format.setBackground( QColor(255, 255, 100) );
    rule.pattern = QRegularExpression("\\bERROR\\b");
    rule.format = format;
    highlightingRules.append(rule);
}
OutHighlighter::~OutHighlighter(){}

void OutHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : highlightingRules)
    {
        QRegularExpressionMatchIterator it =
            rule.pattern.globalMatch(text);

        while (it.hasNext())
        {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(),
                      match.capturedLength(),
                      rule.format);
        }
    }
}



#include "moc_outpaneltext.cpp"
