/***************************************************************************
 *   Copyright (C) 2026 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/


#pragma once

#include <QDialog>

#include "updatable.h"
#include "ui_terminal.h"

class Terminal : public QDialog, public Updatable, private Ui::Terminal
{
    Q_OBJECT

    public:
        Terminal( QWidget* parent );

        void updateStep() override;

        void received( uint8_t byte ) ;

    signals:
        void sendBytes( QByteArray data );
        void closed();

    private slots:
        void on_sendButton_clicked();
        void on_clearSend_clicked();
        void on_clearReceive_clicked();
        void on_loadFileButton_clicked();
        void on_saveLogButton_clicked();

    protected:
        void closeEvent( QCloseEvent* event ) override;

    private:
        void sendText();
        void sendValue( int base );

        QString m_textBuffer;
};
