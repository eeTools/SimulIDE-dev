/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "component.h"
#include "e-element.h"
#include "usartmodule.h"

class LibraryItem;
class CustomButton;
class Terminal;
class QGraphicsProxyWidget;

class SerialTerm : public Component, public UsartModule, public eElement
{
    public:
        SerialTerm( QString type, QString id );
        ~SerialTerm();
        
 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        virtual void stamp() override;
        virtual void updateStep() override;
        virtual void runEvent() override;

        void setSerialMon( bool s );

        void sendByteArray( QByteArray data );
        virtual void sendByte( uint8_t data ) override;

        virtual void setIdLabel( QString id ) override;

        virtual void byteReceived( uint8_t byte ) override;
        virtual void frameSent( uint8_t data ) override;

        virtual void monitorClosed() override;
        void TerminalClosed();

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

        void onbuttonclicked();
        void slotOpenTerm();

    protected:
        virtual void setflip() override;
        virtual void contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu ) override;

    private:
        bool m_receiving;
        bool m_sending;

        Terminal* m_terminal;

        CustomButton* m_button;
        QGraphicsProxyWidget* m_proxy;

        QByteArray m_uartData;
};
