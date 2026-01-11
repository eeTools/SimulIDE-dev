/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QApplication>
#include <QTranslator>
#include <QStandardPaths>
#include <QtGui>

#include "mainwindow.h"
#include "circuitwidget.h"
#include "batchtest.h"

void myMessageOutput( QtMsgType type, const QMessageLogContext &context, const QString &msg )
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char* file     = context.file ? context.file : "";
    const char* function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        if( CircuitWidget::self() ) CircuitWidget::self()->simDebugMessage( msg );
        fprintf( stderr, "%s \n", localMsg.constData() );
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}

QString langFile( QString locale )
{
    QString langF = ":/simulide_"+locale+".qm";

    if( !QFile::exists( langF ) ) langF = "";

    return langF;
}

int main( int argc, char *argv[] )
{
    qInstallMessageHandler( myMessageOutput );

#ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    QApplication app( argc, argv );

    QSettings settings( QStandardPaths::standardLocations( QStandardPaths::AppDataLocation).first()+"/simulide.ini",  QSettings::IniFormat, 0l );

    QString locale = QLocale::system().name();
    if( settings.contains( "language" ) ) locale = settings.value( "language" ).toString();

    QString langF = langFile( locale );
    if( langF == "" )
    {
        locale = QLocale::system().name().split("_").first();
        langF = langFile( locale );
    }
    if( langF == "" ) langF = ":/simulide_en.qm";

    QTranslator translator;
    translator.load( langF );
    app.installTranslator( &translator );
    app.setApplicationVersion( APP_VERSION );

    MainWindow window;
    window.setLoc( locale );
    window.show();

    if( argc > 1 )
    {
        QString arg = QString::fromStdString( argv[1] );

        if( arg == "-test" )
        {
            if( argc > 2 ){
                arg = QString::fromStdString( argv[2] );
                QTimer::singleShot( 300, [arg](){ BatchTest::doBatchTest( arg ); } );
            }
        }
        else if( arg.endsWith(".sim2") || arg.endsWith(".sim1"))
        {
            if( argc > 2 ){
                QString arg2 = QString::fromStdString( argv[2] );
                if( arg2 == "-nogui") window.hideGui();
            }
            QString file = "file://";
            if( arg.startsWith( file ) ) arg.replace( file, "" ).replace("\r\n", "" ).replace("%20", " ");
        #ifdef _WIN32
            if( arg.startsWith( "/" )) arg.remove( 0, 1 );
        #endif
            QTimer::singleShot( 300, CircuitWidget::self()
                              , [arg]()->void{ CircuitWidget::self()->loadCirc( arg ); } );
        }
    }

    return app.exec();
}

