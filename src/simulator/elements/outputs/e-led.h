/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "e-resistor.h"

class eLed : public eResistor
{
    public:
        eLed( QString id );
        ~eLed();

        virtual void stamp() override;
        virtual void initialize() override;
        virtual void voltChanged() override;

        double threshold() { return m_threshold; }
        void  setThreshold( double threshold ) { m_threshold = threshold; m_changed = true; }

        double maxCurrent()             { return m_maxCurrent; }
        void  setMaxCurrent( double c ) { m_maxCurrent = c; m_changed = true; }

        virtual void   setResistance( double resist ) override ;
        virtual double resistance() override { return m_impedance; }

        double brightness() { return m_brightness; }
        double overCurrent() { return m_avgCurrent/m_maxCurrent; }

        void updateBright();

    protected:
        virtual void updateVI() override;

        bool m_converged;

        uint64_t m_prevStep;
        uint32_t m_intensity;

        double m_maxCurrent;
        double m_avgCurrent;
        double m_totalCurrent;
        double m_lastCurrent;
        double m_lastPeriod;
        double m_brightness;

        double m_voltPN;
        double m_threshold;
        double m_impedance;
        double m_lastThCurrent;
};
