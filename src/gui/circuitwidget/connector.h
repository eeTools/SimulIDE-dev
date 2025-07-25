/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "compbase.h"
#include "updatable.h"

class ConnectorLine;
class Pin;

class Connector : public CompBase, public Updatable
{
    friend class Node;
    friend class ConnectorLine;

    public:
        Connector( QString type, QString id, Pin* startpin, Pin* endpin = nullptr );
        ~Connector();

        void updateStep() override;

        QString pListStr() { return m_pointList.join(","); }
        QStringList pointList() { refreshPointList(); return m_pointList; }
        void setPointListStr( QString pl );
        void setPointList( QStringList pl );

        void dummySetter( QString ) {;}

        QString startPinId();
        QString endPinId();
        Pin* startPin() { return m_startPin;}
        void setStartPin( Pin* pin ) { m_startPin = pin; }
        Pin* endPin() { return m_endPin; }
        void setEndPin( Pin* pin) { m_endPin = pin; }

        double getVoltage();

        QList<ConnectorLine*>* lineList() { return &m_conLineList; }

        void incActLine() { if( m_actLine < m_conLineList.size()-1 ) m_actLine += 1; }

        ConnectorLine* addConLine( int x1, int y1, int x2, int y2, int index );
        void addConLine( ConnectorLine* line, int index );
        void remNullLines();
        void refreshPointList();
        void updateConRoute( Pin* nod, QPointF this_point );
        void closeCon( Pin* endpin );
        void splitCon( int index, Pin* pin0, Pin* pin2 );

        void updateLines();
        void animate( bool an );

        void setVisib(  bool vis );
        void setSelected( bool selected );
        
        void setIsBus( bool bus );
        bool isBus() { return m_isBus; }
        
        void move( QPointF delta );
        void isMoved();

        void remove();
        void remLines();

        bool m_freeLine;

        double getCurrent();

    private:
        void remConLine( ConnectorLine* line  );
        void updateCon();
        void connectLines( int index1, int index2 );
        void disconnectLines( int index1, int index2 );

        int m_actLine;
        int m_lastindex;
        
        bool m_isBus;
        bool m_animate;


        double m_step;
        double m_current;
        double m_currentSpeed;

        Pin*    m_startPin;
        Pin*    m_endPin;

        QStringList  m_pointList;

        QList<ConnectorLine*> m_conLineList;
};
