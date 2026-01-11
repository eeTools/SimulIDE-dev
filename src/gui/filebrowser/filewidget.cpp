/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QMenu>
#include <QSettings>
#include <QContextMenuEvent>

#include "filewidget.h"
#include "mainwindow.h"

FileWidget* FileWidget::m_pSelf = 0l;

FileWidget::FileWidget( QWidget* parent )
          : QSplitter( parent )
{
    m_pSelf = this;
    m_blocked = false;
    
    this->setOrientation( Qt::Vertical );
    
    m_bookmarks = new QListWidget( this );
    this->addWidget( m_bookmarks );

    QWidget* pathWidget = new QWidget( this );
    QHBoxLayout* hLayout = new QHBoxLayout( pathWidget );
    hLayout->setContentsMargins(0,0,0,0);
    m_cdUpButton = new QPushButton( this );
    m_cdUpButton->setIcon( QIcon(":/cdup.png") );
    m_cdUpButton->setToolTip( tr("cd Up") );
    hLayout->addWidget( m_cdUpButton );
    m_path = new QLineEdit( this );
    hLayout->addWidget( m_path );
    pathWidget->setFixedHeight( 24 );
    this->addWidget( pathWidget );

    /*m_searchFiles = new QLineEdit( this );
    m_searchFiles->setPlaceholderText( tr( "Search Files (Doesn't work)" ));
    m_searchFiles->setFixedHeight( 24 );
    this->addWidget( m_searchFiles );*/
    
    m_fileBrowser = new FileBrowser( this );
    m_fileBrowser->setPath( QDir::rootPath() );
    this->addWidget( m_fileBrowser );

    QSettings* settings = MainWindow::self()->settings();
    QDir setDir( settings->fileName() );
    setDir.cdUp( );
    QString settingsDir = setDir.absolutePath() ;
    
    addEntry("FileSystem", QDir::rootPath() );
    addEntry("Home",       QDir::homePath() );
    //addEntry("Examples",   MainWindow::self()->getFilePath("examples") );
    //addEntry("Data",       MainWindow::self()->getFilePath("data") );
    addEntry("User Data",  MainWindow::self()->userPath() );
    addEntry("Settings",   settingsDir );

    connect(m_bookmarks, &QListWidget::itemClicked,
            this, &FileWidget::itemClicked,
            Qt::UniqueConnection);
             
    // connect( m_searchFiles, SIGNAL( editingFinished() ),
    //         this,          SLOT( searchChanged() ), Qt::UniqueConnection);

    connect(m_cdUpButton, &QPushButton::released,
            m_fileBrowser, &FileBrowser::cdUp,
            Qt::UniqueConnection);

    connect(m_path, &QLineEdit::editingFinished,
            this, &FileWidget::pathChanged,
            Qt::UniqueConnection);
             
    int size = settings->beginReadArray("bookmarks");
    
    for( int i=0; i<size; i++ ) 
    {
        settings->setArrayIndex(i);
        addBookMark( settings->value("path").toString() );
    }
    settings->endArray();

    this->setSizes( {100, 10, 300} );
}
FileWidget::~FileWidget(){}

void FileWidget::writeSettings()
{
    QSettings* settings = MainWindow::self()->settings();
    settings->beginWriteArray("bookmarks");
    
    for( int i=0; i<m_bookmarkList.size(); i++ ) 
    {
        settings->setArrayIndex(i);
        settings->setValue("path", m_bookmarkList.at(i) );
    }
    settings->endArray(); 
    
    while( m_bookmarks->count() ) delete m_bookmarks->takeItem( 0 );
}

void FileWidget::addEntry( QString name, QString path )
{
    if( !QDir( path ).exists() ) return;

    QListWidgetItem* item = new QListWidgetItem( name, m_bookmarks, 0 );
    item->setData( 4, path );
    
    QFont font;
    font.setPixelSize( 11*MainWindow::self()->fontScale() );
    font.setWeight(QFont::DemiBold);
    item->setFont( font );
    item->setIcon( QIcon(":/open.png") );
}

void FileWidget::addBookMark( QString path )
{
    QDir dir = QDir( path );
    QString dirPath = dir.absolutePath();
    QString name    = dir.dirName()+" ("+dirPath+")";

    if( !m_bookmarkList.contains( dirPath ) )
    {
        m_bookmarkList.append( dirPath );
        addEntry( name, dirPath );
}   }

void FileWidget::remBookMark()
{
    QListWidgetItem* item =  m_bookmarks->takeItem( m_bookmarks->currentRow() );
    m_bookmarkList.removeOne( item->data( 4 ).toString() );
    delete item;
}

void FileWidget::itemClicked( QListWidgetItem* item  )
{
    QString path = item->data( 4 ).toString();
    m_fileBrowser->setPath( path );
}

void FileWidget::searchChanged()
{
    //QString filter = m_searchFiles->text();
    //m_fileBrowser->searchFiles( filter );
}

void FileWidget::setPath( QString path )
{
    m_blocked = true;
    m_path->setText( path );
    m_blocked = false;
}

void FileWidget::pathChanged()
{
    if( m_blocked ) return;        // Avoid circular call with setPath()
    
    QString path = m_path->text();
    m_fileBrowser->setPath( path );
}

void FileWidget::contextMenuEvent( QContextMenuEvent* event )
{
    QListWidgetItem* item =  m_bookmarks->currentItem();
    if( !item ) return;
    
    if( m_bookmarkList.contains( item->data( 4 ).toString() ) )
    {
        QPoint eventPos = event->globalPos();
        QMenu menu;

        QAction* remBookMarkAction =
            menu.addAction(QIcon(":/remove.svg"), tr("Remove Bookmark"));
        connect(remBookMarkAction, &QAction::triggered,
                this, [this]() { remBookMark(); });
                 
        menu.exec( eventPos );
}   }
