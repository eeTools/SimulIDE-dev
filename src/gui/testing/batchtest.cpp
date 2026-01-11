/***************************************************************************
 *   Copyright (C) 2024 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QTimer>
#include <QDebug>

#include "batchtest.h"
#include "component.h"
#include "circuitwidget.h"

bool BatchTest::m_running = false;
QString BatchTest::m_currentFile;
QStringList BatchTest::m_failedTests;
QStringList BatchTest::m_circFiles;
QList<Component*> BatchTest::m_testUnits;

void BatchTest::doBatchTest( QString folder )
{
    QDir dir = QDir(folder);
    if( !dir.exists() )
    {
        qDebug() <<"Folder doesn't exist:" << Qt::endl << folder;
        return;
    }
    m_failedTests.clear();
    m_circFiles.clear();

    prepareTest( dir );

    m_running = true;
    runNextCircuit();
}

void BatchTest::prepareTest( QDir baseDir )
{
    QStringList circList = baseDir.entryList( {"*.sim2","*.sim1"}, QDir::Files );

    for( QString file : circList )
        m_circFiles.append( baseDir.absoluteFilePath( file ) );

    QStringList dirList = baseDir.entryList( {"*"}, QDir::Dirs );
    for( QString dir : dirList )
    {
        if( dir == "." || dir == "..") continue;
        if( !baseDir.cd( dir ) ) continue;

        prepareTest( baseDir );
        baseDir.cd("..");
    }
}

void BatchTest::runNextCircuit()
{
    CircuitWidget::self()->powerCircOff();

    if( m_circFiles.isEmpty() )  // All tests completed
    {
        m_running = false;

        if( m_failedTests.isEmpty() ) qDebug() << "All tests passed";
        else {
            qDebug() << m_failedTests.size() << "Tests failed:";
            for( QString file : m_failedTests ) qDebug() << file;
        }
        return;
    }
    m_currentFile = m_circFiles.takeFirst();

    qDebug() << "Testing" << m_currentFile;
    CircuitWidget::self()->loadCirc( m_currentFile );

    m_testUnits.clear();
    CircuitWidget::self()->powerCircOn();
    checkFinished();
}

void BatchTest::checkFinished()
{
    if( m_running ) QTimer::singleShot( 100, BatchTest::checkFinished );
    else            BatchTest::runNextCircuit();
}

void BatchTest::addTestUnit( Component* c )
{
    if( !m_testUnits.contains( c ) ) m_testUnits.append( c );
}

void BatchTest::testCompleted( Component* c, bool ok ) // A test unit completed (could be more in this Circuit)
{
    m_testUnits.removeAll( c );

    if( !ok ){  // Test failed
        if( !m_failedTests.contains( m_currentFile) ) m_failedTests.append( m_currentFile );
    }
    if( m_testUnits.isEmpty() ) m_running = false; // All test units in this Circuit finished
}

