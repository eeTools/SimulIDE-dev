/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

//#define DEBUG_EVENTS

#include "e-node.h"
#include "e-element.h"
#include "analogclock.h"

enum simState_t{
    SIM_STOPPED=0,
    SIM_ERROR,
    SIM_STARTING,
    SIM_PAUSED,
    SIM_WAITING,
    SIM_RUNNING,
    SIM_DEBUG,
};

#include <QElapsedTimer>
#include <QFuture>

class BaseProcessor;
class Updatable;
class eElement;
class Socket;
class eNode;
class CircMatrix;

class Simulator : public QObject
{
        friend class eNode;

    Q_OBJECT
    public:
        Simulator( QObject* parent=0 );
        ~Simulator();

 static Simulator* self() { return m_pSelf; }

         void addEvent( uint64_t time, eElement* el );
         void cancelEvents( eElement* el );

        void startSim( bool paused=false );
        void pauseSim();
        void resumeSim();
        void stopSim();

        void setWarning( int warning ) { m_warning = warning; }
        
        uint64_t fps() { return m_fps; }
        void setFps( uint64_t fps );
        uint64_t psPerFrame() { return m_psPF; }
        uint64_t simPsPF() { return m_simPsPF; }

        uint64_t psPerSec() { return m_psPerSec; } // Speed picosecond/second
        void setPsPerSec( uint64_t psPs );

        uint64_t stepSize() { return m_stepSize; }
        void setStepSize( uint64_t stepSize ) { m_stepSize = stepSize; }

        uint64_t stepsPerSec() { return m_stepsPS; }
        void setStepsPerSec( uint64_t sps );

        void  setSlopeSteps( int steps ) { m_slopeSteps = steps; }
        int slopeSteps( ) { return m_slopeSteps; }

        void  setMaxNlSteps( uint32_t steps ) { m_maxNlstp = steps; }
        uint32_t maxNlSteps( ) { return m_maxNlstp; }
        
        bool isRunning() { return (m_state >= SIM_STARTING); }
        bool isPaused()  { return (m_state == SIM_PAUSED); }
        bool isPauseDebug() { return (m_state == SIM_PAUSED && m_debug == true); }

        uint64_t circTime() { return m_circTime; }

        void timerEvent( QTimerEvent* e );

        double realSpeed() { return m_realSpeed; } // 0 to 10000 => 0 to 100%

        simState_t simState() { return m_state; }

        inline void notCorverged() { m_converged = false; }

        void addToEnodeList( eNode* nod );

        void addToElementList( eElement* el );
        void remFromElementList( eElement* el );
        
        void addToUpdateList( Updatable* el );
        void remFromUpdateList( Updatable* el );

        void addToSocketList( Socket* el );
        void remFromSocketList( Socket* el );

    private:
 static Simulator* m_pSelf;

        // Accelerate calls from eNode:
        inline void addToChangedNodes( eNode* nod ) { nod->nextCH = m_changedNode; m_changedNode = nod; }
        inline void addToChangedList( eElement* el ) { el->nextChanged = m_voltChanged; m_voltChanged = el; }
        inline void addToNoLinList( eElement* el ) { el->nextChanged = m_nonLinear;  m_nonLinear = el; }

        void createNodes();
        void resetSim();
        void runCircuit();
        inline void solveCircuit();
        inline void solveMatrix();

        inline void clearEventList();

        //inline void stopTimer();
        //inline void initTimer();
        eElement* m_firstEvent;

#ifdef DEBUG_EVENTS
        uint64_t m_events;
        uint64_t m_maxEvents;
        uint64_t m_totEvents;
        uint64_t m_findEvents;
        uint64_t m_maxfinds;
#endif

        QFuture<void> m_CircuitFuture;

        CircMatrix* m_matrix;

        QMap<int, QString> m_errors;
        QMap<int, QString> m_warnings;

        QList<eNode*> m_eNodeList;

        eNode*    m_changedNode;
        eElement* m_voltChanged;
        eElement* m_nonLinear;

        QList<eElement*> m_elementList;
        QList<Updatable*> m_updateList;
        QList<Socket*> m_socketList;

        volatile simState_t m_state;
        simState_t m_oldState;

        bool m_debug;
        bool m_converged;
        bool m_pauseCirc;

        int m_error;
        int m_warning;
        int m_timerId;
        int m_timerTick_ms;
        int m_slopeSteps;

        double m_realFPS;
        uint64_t m_fps;
        uint32_t m_NLstep;
        uint32_t m_maxNlstp;

        uint64_t m_psPerSec;
        uint64_t m_stepSize;  ///
        uint64_t m_stepsPS;   ///
        uint64_t m_psPF;
        uint64_t m_simPsPF;
        double   m_realSpeed;

        uint64_t m_timerTime;
        uint64_t m_circTime;
        uint64_t m_tStep;
        uint64_t m_lastStep;
        uint64_t m_refTime;
        uint64_t m_lastRefT;
        uint64_t m_loopTime;
        uint64_t m_guiTime;
        uint64_t m_updtTime;
        double   m_simLoad;

        QElapsedTimer m_RefTimer;
        AnalogClock m_analogClock;
};
