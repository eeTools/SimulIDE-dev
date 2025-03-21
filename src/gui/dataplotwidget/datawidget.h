/***************************************************************************
 *   Copyright (C) 2020 by Santiago González                               *
 *                                                                         */

#pragma once

#include <QWidget>

#include "ui_datawidget.h"

class Oscope;

class DataWidget : public QWidget, private Ui::DataWidget
{
    Q_OBJECT

    public:
        DataWidget(QWidget* parent=0 , Oscope* oscope=0) ;

        void setColor( int ch, QColor c );
        void setData( int ch, QString freq );
        void setTunnel( int ch, QString name );

        bool eventFilter( QObject* object, QEvent* event) override;

        QHBoxLayout* getLayout() { return mainLayout; }

    public slots:
        void on_expandButton_clicked();
        void on_channel0_editingFinished();
        void on_channel1_editingFinished();
        void on_channel2_editingFinished();
        void on_channel3_editingFinished();

    private:
        Oscope* m_oscope;

        QList<QLineEdit*> m_chNames;
};
