/***************************************************************************
 *   Copyright (C) 2023 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "component.h"
#include "e-element.h"
#include "dialwidget.h"

class Dialed : public Component
{
    public:
        Dialed( QString type, QString id );
        ~Dialed();

        QList<ComProperty*> dialProps();

        bool slider() { return m_slider; }
        void setSlider( bool s );

        double scale() { return m_dialW.scale(); }
        void setScale( double s );

        virtual void setHidden( bool hid, bool hidArea=false, bool hidLabel=false ) override;

        virtual void setLinkedValue( double v, int i=0 ) override;

    public slots:
        virtual void dialChanged( int );

    protected:
        virtual void updateProxy(){;}

        bool m_needUpdate;
        bool m_slider;

        QRectF m_areaDial;
        QRectF m_areaComp;

        DialWidget m_dialW;
        QGraphicsProxyWidget* m_proxy;
};
