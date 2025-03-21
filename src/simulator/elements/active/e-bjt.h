/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "e-element.h"

class eBJT : public eElement
{
    public:

        eBJT( QString id );
        virtual ~eBJT();

        void initialize() override;
        void stamp() override;
        void voltChanged() override;

        double gain() { return m_gain; }
        void setGain( double gain );

        bool pnp()              { return m_PNP; }
        void setPnp( bool pnp ) { m_PNP = pnp; m_changed = true; }

        double threshold() { return m_vCrit; }
        void   setThreshold( double vCrit );

    protected:
        double limitStep( double vnew, double vold );

        double m_baseCurr;
        double m_voltBE;
        double m_voltBC;
        double m_gain;
        double m_vt;
        double m_satCur;
        double m_vCrit;
        double m_rgain;
        double m_fgain;

        bool m_PNP;

        eElement m_BEjunction;
        eElement m_BCjunction;

        ePin* m_BC;
        ePin* m_CB;
        ePin* m_BE;
        ePin* m_EB;
        ePin* m_CE;
        ePin* m_EC;
};
