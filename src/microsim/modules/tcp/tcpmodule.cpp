/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>
#include <QtNetwork>
#include <QTcpSocket>
#include <QMenu>

#include "tcpmodule.h"
#include "simulator.h"
#include "iopin.h"


TcpModule::TcpModule( QString name )
         : eElement( name )
{
    m_debug = false;

    m_connectSM = new QSignalMapper();
    QObject::connect( m_connectSM, &QSignalMapper::mappedInt,
                     [=](int i){ tcpConnected(i); } );

    m_discontSM = new QSignalMapper();
    QObject::connect( m_discontSM, &QSignalMapper::mappedInt,
                     [=](int i){ tcpDisconnected(i); } );

    m_readyReSM = new QSignalMapper();
    QObject::connect( m_readyReSM, &QSignalMapper::mappedInt,
                     [=](int i){ tcpReadyRead(i); } );

    Simulator::self()->addToUpdateList( this );

}
TcpModule::~TcpModule()
{
    TcpModule::initialize();
}

void TcpModule::initialize()
{
    for( tcpConnection_t* conn : m_tcpConnections )
    {
        conn->socket->disconnectFromHost();
        conn->socket->waitForDisconnected( 1000 );
        conn->socket->deleteLater();
        delete conn;
    }
    m_tcpConnections.clear();
}

void TcpModule::updateStep()
{
    for( tcpConnection_t* conn : m_tcpConnections )
    {
        switch( conn->action ) {
            case tcpNone: break;
            case tcpConnect:
            {
                QTcpSocket* tcpSocket = conn->socket;
                if( tcpSocket->state() == QAbstractSocket::UnconnectedState )
                {
                    if( m_debug ) qDebug() << "TcpModule - Connecting Socket"<<conn->number<<"to"<<conn->host<< conn->port;
                    tcpSocket->connectToHost( conn->host, conn->port );
                }
                else if( m_debug ) qDebug() << "TcpModule - Error Connecting Socket"<<conn->number<<"to"<<conn->host<<conn->port
                                            <<"\n              State = "<<tcpSocket->state();
            }break;
            case tcpClose:
            {
                QTcpSocket* tcpSocket = conn->socket;
                if( !tcpSocket ) return;

                if( tcpSocket->state() == QAbstractSocket::ConnectedState )
                {
                    if( m_debug ) qDebug() << "TcpModule - Disconnecting Socket"<<conn->number<<"from"<<conn->host<< conn->port;
                    tcpSocket->disconnectFromHost();
                }
                else if( m_debug )
                    qDebug() << "TcpModule - Error Disconnecting Socket"<<conn->number<<"from"<<conn->host<< conn->port
                             <<"\n              State = "<<tcpSocket->state();
            }break;
        }
        conn->action = tcpNone;
    }
}

void TcpModule::closeSocket( int link )
{
    if( !m_tcpConnections.contains( link ) ) return;
    tcpConnection_t* conn = m_tcpConnections.value( link );
    conn->action = tcpClose;
}

void TcpModule::sendMsg( QString msg, int link )
{
    if( !m_tcpConnections.contains( link ) ) return;
    tcpConnection_t* conn = m_tcpConnections.value( link );
    QTcpSocket* tcpSocket = conn->socket;

    if( tcpSocket->state() == QAbstractSocket::ConnectedState )
    {
        QByteArray toSend = msg.toUtf8();
        if( m_debug ) qDebug() << "TcpModule - Sending data to Socket:"<<conn->number<<"\n"<<toSend;
        int bytes = tcpSocket->write( toSend+"\n\0" );
        if( m_debug ) qDebug() << "TcpModule -"<<bytes<<"Bytes Written to Socket"<<conn->number;
    }
    else if( m_debug ) qDebug() << "TcpModule - Error Sending data: Socket"<<conn->number<<"not connected\n";
}

void TcpModule::connectTo( int link, QString host, int port )
{
    if( m_tcpConnections.contains( link ) )
    {
        tcpConnection_t* conn = m_tcpConnections.value( link );
        conn->action = tcpConnect;
    }else{
        QTcpSocket* tcpSocket = new QTcpSocket();
        tcpConnection_t* conn = new tcpConnection_t;

        conn->socket = tcpSocket;
        conn->host   = host;
        conn->port   = port;
        conn->number = link;
        conn->action = tcpConnect;

        m_tcpConnections[link] = conn;

        QObject::connect( tcpSocket, &QTcpSocket::connected ,
                        m_connectSM, QOverload<>::of(&QSignalMapper::map), Qt::UniqueConnection );
        m_connectSM->setMapping( tcpSocket, link );

        QObject::connect( tcpSocket, &QTcpSocket::disconnected,
                        m_discontSM, QOverload<>::of(&QSignalMapper::map), Qt::UniqueConnection );
        m_discontSM->setMapping( tcpSocket, link );

        QObject::connect( tcpSocket, &QTcpSocket::readyRead,
                       m_readyReSM, QOverload<>::of(&QSignalMapper::map), Qt::UniqueConnection );
        m_readyReSM->setMapping( tcpSocket, link );
    }
}

// Replies from TCP --------------------------------------------------------------------------

void TcpModule::tcpConnected( int link )
{
    if( !m_debug ) return;
    tcpConnection_t* conn = m_tcpConnections.value( link );
    qDebug() << "TcpModule - Socket"<<link<<"Connected to"<<conn->host<<conn->port;
}
void TcpModule::tcpDisconnected( int link )
{
    if( !m_debug ) return;
    tcpConnection_t* conn = m_tcpConnections.value( link );
    qDebug() << "TcpModule - Socket"<<link<<"Disconnected from"<<conn->host<<conn->port;
}

void TcpModule::tcpReadyRead( int link )
{
    tcpConnection_t* conn = m_tcpConnections.value( link );

    QString msg = conn->socket->readAll();
    received( msg, link ) ;

    if( !m_debug ) return;
    qDebug() << "TcpModule - Socket"<<link
             <<"Received from Host:" << conn->host <<"Port:"<< conn->port
             <<"\n"<< msg;
}

