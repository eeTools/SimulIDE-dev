/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "iopin.h"

class IoPort;

class Stm32Pin : public IoPin//, public QemuModule
{
    //friend class Stm32;

    public:
        Stm32Pin( uint8_t port, int i, QString id, Component* mcu );
        ~Stm32Pin();

        void initialize() override;
        void stamp() override;

        void voltChanged() override;

        void setPinMode( pinMode_t mode );

        void setOutState( bool high ) override;
        void scheduleState( bool high, uint64_t time ) override;

        void setPortState( bool high );

        void setPull( bool p );
        bool setAlternate( bool a );
        void setAnalog( bool a );

    protected:
        void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

        void setPinState( bool high );
        //QString m_id;

        uint8_t m_port;

        bool m_pull;
        bool m_analog;
        bool m_alternate;

        double m_pullAdmit;

        uint16_t m_pinMask;
};
