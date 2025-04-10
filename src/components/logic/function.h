/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "iocomponent.h"
#include "scriptbase.h"

class LibraryItem;
class CustomButton;
class QGraphicsProxyWidget;

class Function : public IoComponent, public ScriptBase
{
    public:
        Function( QString type, QString id );
        ~Function();
        
 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        void stamp() override;
        void voltChanged() override;
        void runEvent() override { IoComponent::runOutputs(); }

        QString functions() { return m_funcList.join(","); }
        void setFunctions( QString f );

        virtual void remove() override;
        
        void setNumInputs( int inputs );
        void setNumOutputs( int outs );

        bool   getInputState( int pin );
        double getInputVoltage( int pin );
        void   setOutputState( int pin, bool s );
        void   setOutputVoltage( int pin, double v );
        double getOutputVoltage( int pin );
        
    public slots:
        void onbuttonclicked( int i );
        void loadData();
        void saveData();
        
    protected:
        virtual void contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu ) override;

    private:
        void createScript();
        void updateArea( uint ins, uint outs );

        bool m_compiled;

        asIScriptFunction* m_voltChanged;
        QStringList m_funcList;

        QList<CustomButton*> m_buttons;
        QList<QGraphicsProxyWidget*> m_proxys;

        QString m_lastDir;
};
