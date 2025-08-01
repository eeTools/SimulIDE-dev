/***************************************************************************
 *   Copyright (C) 2023 by santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QDialog>

#include "ui_managecomps.h"

class TreeItem;


class manCompDialog : public QDialog, private Ui::manCompDialog
{
    public:
        manCompDialog( QWidget* parent );

        void initialize();

    private slots:
        void slotItemChanged( QTableWidgetItem* item );

    private:
        //void reject();
        void addItem( TreeItem* treeItem );

        QMap<QTableWidgetItem*, TreeItem*> m_treeToList;
        QMap<QTableWidgetItem*, TreeItem*> m_treeToShort;

        bool m_initialized;
};
