/***************************************************************************
 *   Copyright (C) 2023 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "basedebugger.h"

class ScriptCpu;

class asDebugger : public BaseDebugger
{
    public:
        asDebugger( CodeEditor* parent, OutPanelText* outPane );
        ~asDebugger();

        virtual bool upload() override;
        virtual int  compile( bool debug ) override;

        void scriptError( int line );
        void scriptWarning( int line );

    protected:
        //virtual bool postProcess() override;

    private:
        ScriptCpu* m_device;
};
