/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "mech_contact.h"

class CustomButton;
class QGraphicsProxyWidget;

class SwitchBase : public MechContact
{
    public:
        SwitchBase( QString type, QString id );
        ~SwitchBase();

        virtual void updateStep() override;

        virtual void setAngle( double angle ) override;
        virtual void rotateAngle( double a ) override;
        void rotateText( double angle );

        virtual void setHidden( bool hid, bool hidArea=false, bool hidLabel=false ) override;

        QString key() { return m_key; }
        void setKey( QString key );

        QString linkCondition() { return m_condition; }
        void setLinkCondition( QString condition );

        CustomButton* button() { return m_button; }

        virtual void onbuttonclicked();
        virtual void keyEvent( QString key, bool pressed ){;}

    protected:
        //virtual void setflip() override;

        QString m_key;
        QString m_condition;

        CustomButton* m_button;
        QGraphicsProxyWidget* m_proxy;

        QMetaObject::Connection m_keyEventConn;
};
