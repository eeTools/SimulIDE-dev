/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QSplitter>

#include "filebrowser.h"

class QListWidget;
class QPushButton;
class QListWidgetItem;

class FileWidget : public QSplitter
{
    Q_OBJECT
    
    public:
        FileWidget( QWidget* parent );
        ~FileWidget();

 static FileWidget* self() { return m_pSelf; }

        void addBookMark( QString path );
        
        void setPath( QString path );
        
        void writeSettings();

        void addEntry( QString name, QString path );

    private slots:
        void itemClicked( QListWidgetItem* );
        void remBookMark();
        void pathChanged();
        void searchChanged();

    protected:
        void contextMenuEvent( QContextMenuEvent* event );

    private:
 static FileWidget* m_pSelf;

        QStringList m_bookmarkList;
        
        FileBrowser* m_fileBrowser;
        QListWidget* m_bookmarks;
        //QLineEdit*   m_searchFiles;
        QPushButton* m_cdUpButton;
        QLineEdit*   m_path;

        bool m_blocked;
};
