/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QMainWindow>
#include <QDir>

class ComponentList;
class CircuitWidget;
class EditorWindow;
class FileWidget;
class Installer;
class QVBoxLayout;
class QLineEdit;
class QPushButton;
class QSplitter;
class QSettings;

enum Langs {
    English = 0,
    Chinese,
    Traditional_Chinese,
    Czech,
//    Dutch,
//    French,
    German,
//    Italian,
//    Russian,
    Spanish,
//    Portuguese,
    Pt_Brasil,
//    Slovak,
//    Turkish
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow();
        ~MainWindow();

        QSettings* settings();
        QSettings* compSettings();

        void hideGui();
        
        void readSettings();
        
        void setFile( QString file );
        void setState( QString state );

        QString loc();
        void setLoc( QString loc );

        Langs lang() { return m_lang; }
        void setLang( Langs lang );

        void setDefaultFontName( const QString& fontName );
        QString defaultFontName() { return m_fontName; }
        
        float fontScale() { return m_fontScale; }
        void setFontScale( float scale ) { m_fontScale = scale; }

        int autoBck() { return m_autoBck; }
        void setAutoBck( int secs ) { m_autoBck = secs; }

        int autoupdate() { return m_autoUpdt; }
        void setAutoUpdate( int a ) { m_autoUpdt = a; }

        int revision() { return m_revision; }

        QString getHelp( QString name, bool save=true );

        QString getConfigPath( QString file );            // Get file path in config folder
        QString getDataFilePath( QString file );          // Get file path in data folder, first user folder, if not SimulIDE folder
        QString getCircFilePath( QString file );          // Get file path in circuit folder
        QString getUserFilePath( QString f );             // Get file path in user folder
        QString userPath() { return m_userDir; }          // User folder path
        void setUserPath( QString p );
        void getUserPath();                               // File open Dialog
        

        Installer* installer() { return m_installer; }

 static MainWindow* self() { return m_pSelf; }

    protected:
        void closeEvent( QCloseEvent* event );
        void keyPressEvent( QKeyEvent* event );

    private slots:
        void searchChanged();
        void clearSearch();

    private:
        void createWidgets();
        void writeSettings();
        QString getFilePath( QString filename, QString directory );

        Langs m_lang;

        bool m_blocked;

        QString m_fontName;
        float m_fontScale;
        int m_autoBck;
        int m_autoUpdt;

        QDir m_configDir;

        QSettings* m_settings;
        QSettings* m_compSettings;
        
        int m_revision;
        QString m_version;
        QString m_styleSheet;
        QString m_state;
        QString m_file;
        QString m_userDir;

        QHash<QString, QString> m_help;

        Installer*     m_installer;
        CircuitWidget* m_circuitW;
        ComponentList* m_components;
        QWidget*       m_listWidget;
        QLineEdit*     m_searchComponent;
        QPushButton*   m_clearButton;
        FileWidget*    m_fileTree;
        EditorWindow*  m_editor;
        QTabWidget*    m_sidepanel;
        
        QSplitter*  m_mainSplitter;

 static MainWindow* m_pSelf;
};
