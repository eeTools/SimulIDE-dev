/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "e-element.h"
#include "component.h"
#include "usartmodule.h"

class LibraryItem;
class QTcpSocket;
class QSignalMapper;

class Esp01 : public Component, public UsartModule, public eElement
{
    public:
        Esp01( QString type, QString id );
        ~Esp01();

        enum espAction_t{
            espNone =0,
            tcpConnect,
            tcpSend,
            tcpClose,
            uartReply
        };

        static Component* construct( QString type, QString id );
        static LibraryItem* libraryItem();

        int baudrate() { return m_baudrate; }
        void setBaudrate( int br ) { m_baudrate = br;}

        bool debug() { return m_debug; }
        void setDebug( bool d) { m_debug = d; }

        void setSerialMon( bool s );

        virtual void stamp() override;
        virtual void updateStep() override;
        virtual void runEvent() override;

        virtual void setIdLabel( QString id ) override;

        virtual void byteReceived( uint8_t byte ) override;
        virtual void frameSent( uint8_t data ) override;

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

    public slots:
        void slotOpenTerm();
        void tcpConnected( int link );
        void tcpDisconnected( int link );
        void tcpReadyRead( int link );

    protected:
        virtual void contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu ) override;

    private:
        void reset();
        void command();
        void connectTcp( int link );
        void connectReply( QByteArray OP, int link );

        bool m_conWIFI;
        //bool m_conTCP;
        bool m_debug;

        int m_baudrate;
        int m_mode;
        int m_multCon;
        int m_link;

        QString m_buffer;
        QByteArray m_tcpData;
        int m_dataLenght;

        QByteArray m_uartReply;

        QString m_host;
        int     m_port;

        QByteArray m_OK;
        QByteArray m_ERROR;

        espAction_t m_action;

        QHash<int, QTcpSocket*> m_tcpSockets;

};


