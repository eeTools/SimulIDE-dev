/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <vector>
#include <QString>

class ePin;

class eElement
{
    public:
        eElement( QString id );
        virtual ~eElement();

        virtual void initialize(){;}
        virtual void stamp(){;}

        virtual void runEvent(){;}
        virtual void voltChanged(){;}

        virtual void setNumEpins( int n );

        virtual ePin* getEpin( int num );
        virtual void setEpin( int num, ePin* pin );

        QString getId(){ return m_elmId; }

        void pauseEvents();
        void resumeEvents();

        static constexpr double cero_doub = 1e-9;
        static constexpr double low_imp   = 1e-7;
        static constexpr double high_imp  = 1e7;

        // Simulator engine
        eElement* nextChanged;
        bool added;

        eElement* nextEvent;
        uint64_t eventTime;

    protected:
        uint64_t m_pendingTime;

        std::vector<ePin*> m_ePin;

        QString m_elmId;

        bool m_changed;
        double m_step;
};
