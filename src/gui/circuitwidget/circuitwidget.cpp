/***************************************************************************
 *   Copyright (C) 2010 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QSplitter>
#include <QToolButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QSettings>

#include "circuitwidget.h"
#include "editorwindow.h"
#include "mainwindow.h"
#include "simulator.h"
#include "circuit.h"
#include "appdialog.h"
#include "filebrowser.h"
#include "currentwidget.h"
#include "infowidget.h"
#include "about.h"
#include "utils.h"

CircuitWidget* CircuitWidget::m_pSelf = 0l;

CircuitWidget::CircuitWidget( QWidget *parent  )
             : QWidget( parent )
             , m_verticalLayout( this )
             , m_circView( this )
             , m_outPane( this )
             , m_circToolBar( this )
             , m_fileMenu( this )
             , m_infoMenu( this )
{
    setObjectName( "CircuitWidget" );
    m_pSelf = this;

    m_hideGui = false;

    m_appPropW = nullptr;
    m_about = nullptr;

    m_verticalLayout.setObjectName( "verticalLayout" );
    m_verticalLayout.setContentsMargins(0, 0, 0, 0);
    m_verticalLayout.setSpacing(0);

    m_verticalLayout.addWidget( &m_circToolBar );

    m_mainSplitter = new QSplitter( this );
    m_mainSplitter->setObjectName("CircuitSplitter");
    m_mainSplitter->setOrientation( Qt::Vertical );
    m_verticalLayout.addWidget( m_mainSplitter );

    m_currentWidget = new CurrentWidget( this );

    m_infoWidget = new InfoWidget( this );
    m_infoWidget->setTargetSpeed( 100 );


    QWidget* topWidget = new QWidget;
    QVBoxLayout* panelLayout = new QVBoxLayout( topWidget );
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);
    panelLayout->addWidget( m_currentWidget );
    panelLayout->addWidget( m_infoWidget );

    m_panelSplitter = new QSplitter( this );
    m_panelSplitter->setObjectName("Panelplitter");
    m_panelSplitter->setOrientation( Qt::Horizontal );
    m_panelSplitter->addWidget( topWidget );
    m_panelSplitter->addWidget( &m_outPane );
    m_panelSplitter->setSizes( {170, 500} );

    m_mainSplitter->addWidget( &m_circView );
    m_mainSplitter->addWidget( m_panelSplitter );
    m_mainSplitter->setSizes( {500, 100} );

    QFont font( MainWindow::self()->defaultFontName(), 10, QFont::Bold );
    double scale = MainWindow::self()->fontScale();
    font.setPixelSize( 14*scale );
    m_msgLabel = new QLabel( this );
    m_msgLabel->setFont( font );
    m_msgLabel->setMaximumSize( 200*scale, 20*scale );

    createActions();
    updateRecentFileActions();
    createToolBars();

    m_lastCircDir = MainWindow::self()->settings()->value("lastCircDir").toByteArray();
    if( m_lastCircDir.isEmpty() )
    {
        QString appPath = QCoreApplication::applicationDirPath();
        m_lastCircDir = appPath + "./data/examples";
    }
    m_infoWidget->setRate();
}
CircuitWidget::~CircuitWidget() { }

void CircuitWidget::hideGui()
{
    m_circToolBar.hide();
    m_outPane.hide();
    m_panelSplitter->hide();
    m_hideGui = true;
}

void CircuitWidget::clear()
{
    if( m_appPropW )
    {
        m_appPropW->setParent( nullptr );
        m_appPropW->close();
        m_appPropW->deleteLater();
        m_appPropW = nullptr;
    }
    m_circView.clear();
    m_infoWidget->setCircTime( 0 );
}

void CircuitWidget::createActions()
{
    for( int i=0; i<MaxRecentFiles; i++ )
    {
        recentFileActs[i] = new QAction( this );
        recentFileActs[i]->setVisible( false );
        connect( recentFileActs[i], &QAction::triggered,
                 this,              &CircuitWidget::openRecentFile, Qt::UniqueConnection );
    }

    newCircAct = new QAction( QIcon(":/new.svg"), tr("New C&ircuit\tCtrl+N"), this);
    newCircAct->setStatusTip( tr("Create a new Circuit"));
    connect( newCircAct, &QAction::triggered,
                   this, &CircuitWidget::newCircuit, Qt::UniqueConnection );

    openCircAct = new QAction( QIcon(":/open.svg"), tr("&Open Circuit\tCtrl+O"), this);
    openCircAct->setStatusTip( tr("Open an existing Circuit"));
    connect( openCircAct, &QAction::triggered,
                    this, &CircuitWidget::openCirc, Qt::UniqueConnection );

    saveCircAct = new QAction( QIcon(":/save.svg"), tr("&Save Circuit\tCtrl+S"), this);
    saveCircAct->setStatusTip( tr("Save the Circuit to disk"));
    connect( saveCircAct, &QAction::triggered,
                    this, QOverload<>::of(&CircuitWidget::saveCirc), Qt::UniqueConnection );

    saveCircAsAct = new QAction( QIcon(":/saveas.svg"),tr("Save Circuit &As...\tCtrl+Shift+S"), this);
    saveCircAsAct->setStatusTip( tr("Save the Circuit under a new name"));
    connect( saveCircAsAct, &QAction::triggered,
                      this, &CircuitWidget::saveCircAs, Qt::UniqueConnection );

    zoomFitAct = new QAction( QIcon(":/zoomfit.svg"),tr("Zoom to fit"), this);
    zoomFitAct->setStatusTip( tr("Zoom Circuit to fit all components"));
    auto view = CircuitView::self();
    if (view) {
        connect(zoomFitAct, &QAction::triggered,
                view, &CircuitView::zoomToFit, Qt::UniqueConnection);
    }

    zoomSelAct = new QAction( QIcon(":/zoomsel.svg"),tr("Zoom to selected"), this);
    zoomSelAct->setStatusTip( tr("Zoom Circuit to fit all selected components"));
    connect( zoomSelAct, &QAction::triggered,
            CircuitView::self(), &CircuitView::zoomSelected, Qt::UniqueConnection );

    zoomOneAct = new QAction( QIcon(":/zoomone.svg"),tr("Zoom to Scale 1"), this);
    zoomOneAct->setStatusTip( tr("Zoom Circuit to Scale 1:1"));
    connect( zoomOneAct, &QAction::triggered,
             CircuitView::self(), &CircuitView::zoomOne, Qt::UniqueConnection );

    powerCircAct = new QAction( QIcon(":/poweroff.png"),tr("Start Simulation"), this);
    powerCircAct->setStatusTip(tr("Start Simulation"));
    powerCircAct->setIconText("Off");
    connect( powerCircAct, &QAction::triggered,
                     this, &CircuitWidget::powerCirc, Qt::UniqueConnection );

    pauseSimAct = new QAction( QIcon(":/pausesim.png"),tr("Pause Simulation"), this);
    pauseSimAct->setStatusTip(tr("Pause Simulation"));
    connect( pauseSimAct, &QAction::triggered,
             this, &CircuitWidget::pauseCirc, Qt::UniqueConnection );

    settAppAct = new QAction( QIcon(":/config.svg"),tr("Settings"), this);
    settAppAct->setStatusTip(tr("Settings"));
    connect( settAppAct, &QAction::triggered,
                   this, &CircuitWidget::settApp, Qt::UniqueConnection );

    infoAct = new QAction( QIcon(":/help.svg"),tr("SimulIDE Website"), this);
    infoAct->setStatusTip(tr("SimulIDE Website"));
    connect( infoAct, &QAction::triggered,
                this, &CircuitWidget::openInfo, Qt::UniqueConnection );
    
    aboutAct = new QAction( QIcon(":/about.svg"),tr("About SimulIDE"), this);
    aboutAct->setStatusTip(tr("About SimulIDE"));
    connect( aboutAct, &QAction::triggered,
                 this, &CircuitWidget::about, Qt::UniqueConnection );
    
    aboutQtAct = new QAction( QIcon(":/about.svg"),tr("About Qt"), this);
    aboutQtAct->setStatusTip(tr("About Qt"));
    connect(aboutQtAct,
            &QAction::triggered,
            qApp,
            &QApplication::aboutQt);
}

void CircuitWidget::createToolBars()
{
    m_circToolBar.setObjectName( "m_circToolBar" );

    double fs = MainWindow::self()->fontScale()*20;
    m_circToolBar.setIconSize( QSize( fs, fs ) );

    m_circToolBar.addAction( settAppAct );
    QWidget* spacer = new QWidget();
    spacer->setFixedWidth( 15 );
    m_circToolBar.addWidget( spacer );
    m_circToolBar.addSeparator();//..........................

    for( int i=0; i<MaxRecentFiles; i++ ) m_fileMenu.addAction( recentFileActs[i] );
    QToolButton* fileButton = new QToolButton( this );
    fileButton->setToolTip( tr("Last Circuits") );
    fileButton->setMenu( &m_fileMenu );
    fileButton->setIcon( QIcon(":/lastfiles.svg") );
    fileButton->setPopupMode( QToolButton::InstantPopup );
    m_circToolBar.addWidget( fileButton );

    m_circToolBar.addAction( newCircAct );

    m_circToolBar.addAction( openCircAct );
    m_circToolBar.addAction( saveCircAct );
    m_circToolBar.addAction( saveCircAsAct );
    spacer = new QWidget();
    spacer->setFixedWidth( 15 );
    m_circToolBar.addWidget( spacer );
    m_circToolBar.addSeparator();//..........................

    m_circToolBar.addAction( zoomFitAct );
    m_circToolBar.addAction( zoomSelAct );
    m_circToolBar.addAction( zoomOneAct );
    spacer = new QWidget();
    spacer->setFixedWidth( 20 );
    m_circToolBar.addWidget( spacer );
    m_circToolBar.addSeparator();//..........................

    m_circToolBar.addAction( powerCircAct );
    m_circToolBar.addAction( pauseSimAct );
    m_circToolBar.addSeparator();//..........................

    spacer = new QWidget();
    spacer->setFixedWidth( 15 );
    m_circToolBar.addWidget( spacer );
    m_circToolBar.addWidget( m_msgLabel );

    spacer = new QWidget( this );
    spacer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    spacer->setVisible( true );
    m_circToolBar.addWidget( spacer );

    m_infoMenu.addAction( infoAct );
    m_infoMenu.addAction( aboutAct );
    m_infoMenu.addAction( aboutQtAct );

    QToolButton* infoButton = new QToolButton( this );
    infoButton->setToolTip( tr("Info") );
    infoButton->setMenu( &m_infoMenu );
    infoButton->setIcon( QIcon(":/help.svg") );
    infoButton->setPopupMode( QToolButton::InstantPopup );
    m_circToolBar.addWidget( infoButton );
    
    m_circToolBar.addSeparator();//..........................
}

bool CircuitWidget::newCircuit()
{
    if( EditorWindow::self() && EditorWindow::self()->debugStarted() ) EditorWindow::self()->stop();
    else if( Simulator::self() ) powerCircOff();
    
    if( MainWindow::self()->windowTitle().endsWith('*') )
    {
        const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, "CircuitWidget::newCircuit",
                               tr("\nCircuit has been modified.\n"
                                  "Do you want to save your changes?\n"),
          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
                               
        if     ( ret == QMessageBox::Save )   saveCirc();
        else if( ret == QMessageBox::Cancel ) return false;
    }
    clear();

    Circuit::self()->setAutoBck( MainWindow::self()->autoBck() );

    m_curCirc = "";
    Simulator::self()->addToUpdateList( &m_outPane );

    MainWindow::self()->setFile( tr("New Circuit"));
    MainWindow::self()->settings()->setValue( "lastCircDir", m_lastCircDir );
    
    return true;
}

void CircuitWidget::openRecentFile()
{
    QAction* action = qobject_cast<QAction*>( sender() );
    if( !action ) return;

    QString file = action->data().toString();
    QFile pfile( file );
    if( pfile.exists() ) loadCirc( file );
    else{
        const QMessageBox::StandardButton ret
        = QMessageBox::warning( this, "CircuitWidget::openRecentFile",
                               tr("\nCan't find file:\n")+
                               file+"\n\n"+
                               tr("Do you want to remove it from Recent Circuits?\n"),
          QMessageBox::Yes | QMessageBox::No );

        if( ret != QMessageBox::Yes ) return;

        QSettings* settings = MainWindow::self()->settings();
        QStringList files = settings->value("recentCircList").toStringList();
        files.removeAll( file );
        settings->setValue("recentCircList", files );
        updateRecentFileActions();
    }
}

void CircuitWidget::openCirc()
{
    QString dir = m_lastCircDir;
    QString fileName = QFileDialog::getOpenFileName( 0l, tr("Load Circuit"), dir,
                                        tr("Circuits (*.sim*);;All files (*.*)"));
    loadCirc( fileName );
}

void CircuitWidget::loadCirc( QString path )
{
    if( path.isEmpty() || !(path.endsWith(".sim2") || path.endsWith(".sim1")) )
        return;

    if( !newCircuit() ) return;
    Circuit::self()->loadCircuit( path );

    m_curCirc = path;
    m_lastCircDir = path;
    MainWindow::self()->setFile(path.split("/").last());

    if( m_hideGui ) return;

    QSettings* settings = MainWindow::self()->settings();
    settings->setValue( "lastCircDir", m_lastCircDir );

    updateRecentFiles();

    m_infoWidget->setCircTime( 0 );
}

void CircuitWidget::saveCirc()
{
    if( m_curCirc.isEmpty() ) saveCircAs();
    else                      saveCirc( m_curCirc );
}

void CircuitWidget::saveCircAs()
{
    const QString dir = m_lastCircDir;
    QString fileName = QFileDialog::getSaveFileName( this, tr("Save Circuit"), dir,
                                                     tr("Circuits (*.sim*);;All files (*.*)") );
    if( fileName.isEmpty() ) return;

    saveCirc( fileName );
}

void CircuitWidget::saveCirc( QString file )
{
    if     (  file.endsWith(".sim1") ) file.replace(".sim1",".sim2");
    else if( !file.endsWith(".sim2") ) file.append(".sim2");

    if( Circuit::self()->saveCircuit( file ) )
    {
        m_curCirc = file;
        m_lastCircDir = file;
        MainWindow::self()->setFile( file.split("/").last() );
        MainWindow::self()->settings()->setValue( "lastCircDir", m_lastCircDir );
        updateRecentFiles();
    }
    else qDebug() << "\nError Saving Circuit:\n" << file;
}

void CircuitWidget::powerCirc()
{
    if( Simulator::self()->isPaused() )          powerCircOff();
    else if( powerCircAct->iconText() == "Off" ) powerCircOn();
    else if( powerCircAct->iconText() == "On" )  powerCircOff();
}

void CircuitWidget::powerCircOn()
{
    powerCircAct->setIcon( QIcon(":/poweron.png") );
    powerCircAct->setText(tr("Stop Simulation"));
    powerCircAct->setEnabled( true );
    powerCircAct->setIconText("On");
    pauseSimAct->setIcon( QIcon(":/pausesim.png") );
    pauseSimAct->setEnabled( true );
    MainWindow::self()->setState("▶");
    setMsg( " "+tr("Running")+" ", 0 );

    Simulator::self()->startSim();
}

void CircuitWidget::powerCircOff()
{
    if( Simulator::self()->isRunning() ) Simulator::self()->stopSim();

    powerCircAct->setIcon( QIcon(":/poweroff.png") );
    powerCircAct->setText(tr("Start Simulation"));
    powerCircAct->setIconText("Off");
    powerCircAct->setEnabled( true );
    pauseSimAct->setEnabled( false );
    pauseSimAct->setText(tr("Pause Simulation"));
    MainWindow::self()->setState("■");
    setMsg( " "+tr("Stopped")+" ", 1 );

    m_infoWidget->setRate( 0, 0 );
    Circuit::self()->update();
}

void CircuitWidget::powerCircDebug()
{
    if( Simulator::self()->isRunning() ) Simulator::self()->stopSim();

    powerCircAct->setIcon( QIcon(":/powerdeb.png") );
    powerCircAct->setIconText("Debug");
    powerCircAct->setEnabled( false );
    pauseSimAct->setEnabled( false );
    MainWindow::self()->setState("❚❚");
    setMsg( " "+tr("Debug")+"❚❚", 3 );

    m_infoWidget->setRate(-1 );  //m_rateLabel->setText( tr("Speed: Debugger") );

    Simulator::self()->startSim( true );
}

void CircuitWidget::pauseDebug()
{
    m_paused = false;
    Simulator::self()->pauseSim();
}

void CircuitWidget::debugPaused()
{
    if( m_paused ) return;
    m_paused = true;

    setMsg( " "+tr("Debug")+"❚❚", 3 );
    MainWindow::self()->setState("❚❚");
}

void CircuitWidget::resumeDebug()
{
    setMsg( " "+tr("Debug")+"  ▶ ", 4 );
    MainWindow::self()->setState("▶");
    Simulator::self()->resumeSim();
}

void CircuitWidget::pauseCirc()
{
    if( Simulator::self()->simState() > SIM_PAUSED )
    {
        Simulator::self()->pauseSim();
        setMsg( " "+tr("Paused")+" ", 1 );
        MainWindow::self()->setState("❚❚");
        pauseSimAct->setText(tr("Resume Simulation"));
        pauseSimAct->setIcon( QIcon(":/simpaused.png") );
        powerCircAct->setIcon( QIcon(":/poweroff.png") );
        powerCircAct->setIconText("Off");
    }
    else if( Simulator::self()->isPaused() )
    {
        Simulator::self()->resumeSim();
        setMsg( " "+tr("Running")+" ", 0 );
        MainWindow::self()->setState("▶");
        pauseSimAct->setText(tr("Pause Simulation"));
        pauseSimAct->setIcon( QIcon(":/pausesim.png") );
        powerCircAct->setIcon( QIcon(":/poweron.png") );
        powerCircAct->setIconText("On");
    }
}

void CircuitWidget::updtAppDialog()
{
    if( m_appPropW ) m_appPropW->updtValues();
}

void CircuitWidget::settApp()
{
    if( !m_appPropW )
    {
        m_appPropW = new AppDialog( this );
        QPoint p = mapToGlobal( QPoint(50, 50) );
        m_appPropW->move( p.x(), p.y() );
    }
    m_appPropW->show();
}

void CircuitWidget::openInfo()
{ QDesktopServices::openUrl(QUrl("http://simulide.com")); }

void CircuitWidget::about()
{
    if( !m_about ) m_about = new AboutDialog( this );
    m_about->show();
}

void CircuitWidget::setError( QString error )
{
    setMsg( error, 2 );
    m_infoWidget->setRate( -1 );
}

void CircuitWidget::setMsg( QString msg, int type )
{
    if     ( type == 0 ) m_msgLabel->setStyleSheet("QLabel { background-color: lightgreen; color: black;  font-weight: bold;}");
    else if( type == 1 ) m_msgLabel->setStyleSheet("QLabel { background-color: orange;     color: white;  font-weight: bold;}");
    else if( type == 2 ) m_msgLabel->setStyleSheet("QLabel { background-color: red;        color: yellow; font-weight: bold;}");
    else if( type == 3 ) m_msgLabel->setStyleSheet("QLabel { background-color: blue;       color: white;  font-weight: bold;}");
    else if( type == 4 ) m_msgLabel->setStyleSheet("QLabel { background-color: lightblue;  color: black;  font-weight: bold;}");
    m_msgLabel->setText( "   "+msg+"   " );
}

void CircuitWidget::updateRecentFiles()
{
    QSettings* settings = MainWindow::self()->settings();
    QStringList files = settings->value("recentCircList").toStringList();
    files.removeAll( m_curCirc );
    files.prepend( m_curCirc );
    while( files.size() > MaxRecentFiles ) files.removeLast();

    settings->setValue("recentCircList", files );
    updateRecentFileActions();
}

void CircuitWidget::updateRecentFileActions()
{
    QSettings* settings = MainWindow::self()->settings();
    QStringList files = settings->value("recentCircList").toStringList();

    int numRecentFiles = qMin( files.size(), (int)MaxRecentFiles );

    for( int i=0; i<numRecentFiles; i++ )
    {
        QString text = QString::number(i + 1)+"- "+ getFileName( files[i] );
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData( files[i] );
        recentFileActs[i]->setVisible( true );
    }
    for( int i=numRecentFiles; i<MaxRecentFiles; i++ ) recentFileActs[i]->setVisible(false);
}

#include "moc_circuitwidget.cpp"
