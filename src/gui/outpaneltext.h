/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QObject>
#include <QRegularExpression>

#include "updatable.h"

class OutHighlighter;

class OutPanelText : public QPlainTextEdit, public Updatable
{
    Q_OBJECT
    public:
        OutPanelText( QWidget* parent=0 );
        ~OutPanelText();

        virtual void updateStep() override;

        void appendText( const QString text ) { m_textBuffer.append( text ); }
        void appendLine( const QString text );

    private:
        QString m_textBuffer;
 
        OutHighlighter* m_highlighter;
};

// CLASS OutHighlighter ****************************************

class OutHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

    public:
        OutHighlighter( QTextDocument* parent = 0 );
        ~OutHighlighter();

    protected:
        void highlightBlock( const QString &text );

    private:
        struct HighlightingRule
        {
            QRegularExpression pattern;
            QTextCharFormat format;
        };
        QVector<HighlightingRule> highlightingRules;
};
