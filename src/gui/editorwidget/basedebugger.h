/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QMap>

#include "compiler.h"

struct codeLine_t{
    QString file;
    int     lineNumber;
    bool operator!=(const codeLine_t& lhs ) const
    { return lhs.file != file || lhs.lineNumber != lineNumber; }
    bool operator==(const codeLine_t& lhs ) const
    { return lhs.file == file && lhs.lineNumber == lineNumber; }
};

class BaseDebugger : public Compiler    // Base Class for all debuggers
{
        friend class eMcu;

    public:
        BaseDebugger( CodeEditor* parent, OutPanelText* outPane );
        ~BaseDebugger();

        virtual bool upload();

        void run();
        void pause();
        bool stepFromLine( bool over=false );
        void stepDebug();

        void setLstType( int type ) { m_lstType = type; }
        void setLangLevel( int level ) { m_langLevel = level; }

        void setLineToFlash( codeLine_t line, int addr );

        int getValidLine( codeLine_t pc );
        bool isMappedLine( codeLine_t line );

        QString getVarType( QString var );

        static QString getValueInFile( QString line, QString word );

        int flashToSourceSize() { return m_flashToSource.size(); }
        
        bool m_stepOver;

    protected:
        virtual void preProcess() override;
        virtual bool postProcess() override;

        bool isNoValid( QString line );

        bool m_debugStep;
        bool m_running;
        bool m_over;

        codeLine_t m_prevLine;

        int  m_exitPC;
        //QList<int>* m_brkPoints;

        //int m_lastLine;
        int m_lstType;   // Bit0: 0 doesn't use ":" (gpasm ), 1 uses ":" (avra, gavrasm)
                         // Bit1: position of flash address (0 or 1)
        int m_langLevel; // 0 for asm, 1 for high level
        int m_codeStart;

        QString m_appPath;
        
        QMap<QString, QString> m_typesList;
        QMap<QString, QString> m_varTypes;     // Variable name-Type got from source file
        QMap<int, codeLine_t> m_flashToSource; // Map flash adress to Source code line
        //QHash<int, int> m_sourceToFlash;        // Map Source code line to flash adress
        QMap<QString, int> m_functions;        // Function name list->start Address
        QList<int>          m_funcAddr;         // Function start Address list
};
