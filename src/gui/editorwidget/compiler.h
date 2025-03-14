﻿/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QString>
#include <QProcess>

#include "compbase.h"

class OutPanelText;
class CodeEditor;

class Compiler : public QObject, public CompBase
{
    Q_OBJECT

    public:
        Compiler( CodeEditor* editor, OutPanelText* outPane );
        ~Compiler();

        QString compName() { return m_compName; }
        void setCompName( QString ) {;}

        virtual QString toolPath() { return m_toolPath; }
        virtual void setToolPath( QString path );

        QString includePath() { return m_inclPath; }
        void setIncludePath( QString path );

        QString extraArgs() { return m_extraArgs; }
        void setextraArgs( QString s ) { m_extraArgs = s; }

        QString family() { return m_family; }
        void setFamily( QString f ) { m_family = f; }

        QString device() { return m_device; }
        void setDevice( QString d ) { m_device = d; }

        QString fileName() { return m_fileName; }
        QString buildPath() { return m_buildPath; }
        QString file() { return m_file ; }

        void clearCompiler();
        void loadCompiler( QString file );
        virtual int compile( bool debug );

        virtual void compilerProps();

        bool isProjectFile( QString file ) { return m_fileList.contains( file ); }

        void readSettings();

        bool checkCommand( QString c );

        OutPanelText* outPane() { return m_outPane; }

    protected:
        void addFilePropHead();

        virtual void preProcess(){;}
        virtual bool postProcess(){return false;}

        int getErrors();
        virtual int getErrorLine( QString txt );
        int getFirstNumber( QString txt );
        void compiled( QString firmware );

        int runBuildStep( QString fullCommand );
        QString replaceData( QString str );
        void toolChainNotFound();

        CodeEditor* m_editor;

        bool m_uploadHex;
        bool m_fileProps;

        QString m_compName;
        QString m_toolPath;
        QString m_inclPath;
        QString m_buildPath;
        QString m_extraArgs;
        QStringList m_command;
        QStringList m_arguments;
        QStringList m_argsDebug;
        QStringList m_fileList;

        QString m_compilerType;
        QString m_family;
        QString m_device;
        QString m_firmware;
        QString m_file;
        QString m_fileDir;
        QString m_fileName;
        QString m_fileExt;

        QProcess m_compProcess;

        OutPanelText* m_outPane;
};
