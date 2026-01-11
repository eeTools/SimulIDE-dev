/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class QTextDocument;

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

    public:
        Highlighter( QTextDocument* parent = 0);
        ~Highlighter();

        QStringList readSyntaxFile( QString fileName );

        void addObjects( QStringList patterns );
        void addMembers( QStringList patterns );
        void setMembers( QStringList patterns );
        void setExtraTypes( QStringList types );

    protected:
        void highlightBlock( const QString &text );

    private:
        struct HighlightRule
        {
            QRegularExpression pattern;
            QTextCharFormat format;
        };
        void addRule( QTextCharFormat, QString );
        void processRule( HighlightRule rule, QString lcText );

        bool m_multiline;
        
        QVector<HighlightRule> m_rules;
        QVector<HighlightRule> m_objectRules;
        QVector<HighlightRule> m_memberRules;
        QVector<HighlightRule> m_extraRules;

        QRegularExpression m_multiStart;
        QRegularExpression m_multiEnd;
        QTextCharFormat m_multiFormat;
};
