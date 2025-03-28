/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "component.h"
#include "e-element.h"
#include "e-led.h"

class LibraryItem;

class LedRgb : public Component, public eElement
{
    public:
        LedRgb( QString type, QString id );
        ~LedRgb();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        virtual void stamp() override;
        virtual void updateStep() override;
        
        double threshold_R() { return m_led[0]->threshold(); }
        void   setThreshold_R( double threshold );
        double maxCurrent_R() { return m_led[0]->maxCurrent(); }
        void   setMaxCurrent_R( double current );
        double res_R() { return m_led[0]->resistance(); }
        void   setRes_R( double resist );

        double threshold_G() { return m_led[1]->threshold(); }
        void   setThreshold_G( double threshold );
        double maxCurrent_G() { return m_led[1]->maxCurrent(); }
        void   setMaxCurrent_G( double current );
        double res_G() { return m_led[1]->resistance(); }
        void   setRes_G( double resist );

        double threshold_B() { return m_led[2]->threshold(); }
        void   setThreshold_B( double threshold );
        double maxCurrent_B() { return m_led[2]->maxCurrent(); }
        void   setMaxCurrent_B( double current );
        double res_B() { return m_led[2]->resistance(); }
        void   setRes_B( double resist );

        bool isComCathode() { return m_commonCathode; }
        void setComCathode( bool ct );

        virtual void setHidden( bool hid, bool hidArea=false, bool hidLabel=false ) override;
        
        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

    private:
        void createMatrix();
        void deleteMatrix();

        bool m_commonCathode = true;

        double bright[3];

        eLed* m_led[3];
};
