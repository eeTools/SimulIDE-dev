/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QRegularExpression>

#include "findreplace.h"
#include "codeeditor.h"
#include "basedebugger.h"



FindReplace::FindReplace( QWidget* parent )
           : QDialog( parent )
{
    setupUi(this);
    m_editor = nullptr;

    setWindowFlags( Qt::Window | Qt::WindowTitleHint | Qt::Tool
                   | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint );
}

void FindReplace::on_prevButton_clicked()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    m_editor->setFound( extraSelections );
    find( false );
}
void FindReplace::on_nextButton_clicked()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    m_editor->setFound( extraSelections );
    find( true );
}

void FindReplace::on_allButton_clicked()
{
    QTextCursor tc = m_editor->textCursor();
    int pos = tc.position();
    tc.setPosition( 0 );
    m_editor->setTextCursor( tc );
    QList<QTextEdit::ExtraSelection> extraSelections;

    int i=0;
    while( find( true ) )
    {
        QTextEdit::ExtraSelection extra;
        extra.cursor = m_editor->textCursor();
        extra.format.setBackground( QBrush(QColor(210,210,255)) );
        extraSelections.append( extra );
        i++;
    }
    m_editor->showMsg( tr("Found %n occurrence(s)", "", i) );

    tc.setPosition( pos );
    m_editor->setTextCursor( tc );
    m_editor->setFound( extraSelections );
}

void FindReplace::on_replaceButton_clicked()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    m_editor->setFound( extraSelections );

    if( m_editor->textCursor().hasSelection() )
        m_editor->textCursor().insertText( replaceEdit->text() );
}

void FindReplace::on_replFindButton_clicked()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    m_editor->setFound( extraSelections );

    if( m_editor->textCursor().hasSelection() )
        m_editor->textCursor().insertText( replaceEdit->text() );
    find( true );
}

void FindReplace::on_replAllButton_clicked()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    m_editor->setFound( extraSelections );

    m_textCursor.setPosition( 0 );
    m_editor->setTextCursor( m_textCursor );

    int i=0;
    while( find( true ) )
    {
        m_editor->textCursor().insertText( replaceEdit->text() );
        i++;
    }
    m_editor->showMsg( tr("Replaced %n occurrence(s)", "", i) );
}

void FindReplace::on_closeButton_clicked()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    m_editor->setFound( extraSelections );
    hide();
}

bool FindReplace::find( bool next )
{
    if( !m_editor ) return false;

    bool result = false;
    QString toSearch = findEdit->text();

    QTextDocument::FindFlags flags;

    if( !next )               flags |= QTextDocument::FindBackward;
    if( caseS->isChecked() )  flags |= QTextDocument::FindCaseSensitively;
    if( whole->isChecked() )  flags |= QTextDocument::FindWholeWords;
    if( regexp->isChecked() )
    {
        QRegularExpression reg(toSearch, (caseS->isChecked() ? QRegularExpression::NoPatternOption
                                                             : QRegularExpression::CaseInsensitiveOption));

        int pos = m_textCursor.position();
        QString text = m_editor->toPlainText();

        QRegularExpressionMatch match = reg.match(text, pos);
        if (!match.hasMatch() && !next) { // backward search
            match = reg.match(text, 0, QRegularExpression::NormalMatch); // wrap around
        }

        if (match.hasMatch()) {
            m_textCursor.setPosition(match.capturedStart());
            m_textCursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
            m_editor->setTextCursor(m_textCursor);
            result = true;
        } else result = false;
    }

    return result;
}

