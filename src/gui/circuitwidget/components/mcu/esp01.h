/***************************************************************************
 *   Copyright (C) 2021 by santiago González                               *
 *   santigoro@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#ifndef ESP01_H
#define ESP01_H

#include "e-element.h"
#include "component.h"
#include "usartmodule.h"

class LibraryItem;
class QTcpSocket;

class MAINMODULE_EXPORT Esp01 : public Component , public UsartModule, public eElement
{
    Q_OBJECT

    public:
        Esp01( QObject* parent, QString type, QString id );
        ~Esp01();

        enum espAction_t{
            espNone =0,
            tcpConnect,
            tcpSend,
            tcpClose,
        };

        static Component* construct( QObject* parent, QString type, QString id );
        static LibraryItem* libraryItem();

        int baudrate() { return m_baudrate; }
        void setBaudrate( int br ) { m_baudrate = br;}

        virtual void stamp() override;
        virtual void updateStep() override;
        virtual void runEvent() override;

        virtual void byteReceived( uint8_t data ) override;
        virtual void frameSent( uint8_t data ) override;

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget );

    public slots:
        void tcpConnected();
        void tcpDisconnected();
        void tcpBytesWritten( qint64 );
        void tcpReadyRead();

    private:
        void proccessInput();
        void sendReply();
        void connectReply( QByteArray OP );

        QTcpSocket* m_tcpSocket;

        int m_baudrate;
        int m_mode;
        int m_multCon;
        int m_link;

        QString m_buffer;
        QByteArray m_tcpData;
        int m_dataLenght;

        QByteArray m_uartReply;
        int m_replyIndex;

        QString m_host;
        int     m_port;

        QByteArray m_OK;
        QByteArray m_ERROR;

        espAction_t m_action;
};

#endif