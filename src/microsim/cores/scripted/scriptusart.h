/***************************************************************************
 *   Copyright (C) 2023 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "scriptperif.h"
#include "mcuuart.h"

class ScriptCpu;

class asIScriptFunction;

class ScriptUsart : public McuUsart, public ScriptPerif
{
    public:
        ScriptUsart( eMcu* mcu, QString name, int number );
        ~ScriptUsart();

        virtual void reset() override;
        virtual void byteReceived( uint8_t data ) override;
        virtual void frameSent( uint8_t data ) override;

        virtual QStringList registerScript( ScriptCpu* cpu ) override;
        virtual void startScript() override;

    private:
        //QString m_uartName;

        asIScriptFunction* m_byteReceived;
        asIScriptFunction* m_frameSent;
};
