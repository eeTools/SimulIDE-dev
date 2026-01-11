/***************************************************************************
 *   Copyright (C) 2022 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QDebug>

#include "scriptbase.h"
#include "scriptstdstring.h"
#include "scriptarray.h"
#include "circuitwidget.h"
#include "mainwindow.h"
#include "utils.h"
#include "asdebugger.h"

void ScriptBase::MessageCallback( const asSMessageInfo* msg )
{
    QString type = " ERROR ";
    if     ( msg->type == asMSGTYPE_WARNING     ) type = " WARNING ";
    else if( msg->type == asMSGTYPE_INFORMATION ) type = " INFO ";

    QString deb = QString( msg->section )+" line: "+QString::number( msg->row )
                  +" "+QString::number( msg->col )+type+QString( msg->message );

    if( m_debugger )
    {
        m_debugger->outPane()->appendLine( deb.remove("\n") );
        if     ( type == " ERROR "  ) m_debugger->scriptError( msg->row );
        else if( type == " WARNING ") m_debugger->scriptWarning( msg->row );
    }
    else CircuitWidget::self()->simDebugMessage( deb.remove("\n") );
    //qDebug() << msg->section << "line:" << msg->row << msg->col << type << msg->message;
}

void print( std::string &msg )
{
    qDebug() << msg.c_str();
}

ScriptBase::ScriptBase( QString name )
          : eElement( name )
{
    m_aEngine  = nullptr;
    m_asModule = nullptr;
    m_context  = nullptr;
    m_debugger = nullptr;

    m_scriptFile = "script";

    m_aEngine = asCreateScriptEngine();
    if( m_aEngine == 0 ) { qDebug() << "Failed to create script engine."; return; }

    m_context = m_aEngine->CreateContext();
    if( m_context == 0 ) { qDebug() << "Failed to create the context."; return; }

    m_aEngine->SetEngineProperty( asEP_AUTO_GARBAGE_COLLECT   , false );
    m_aEngine->SetEngineProperty( asEP_BUILD_WITHOUT_LINE_CUES, true );

    //m_aEngine->SetMessageCallback( asFUNCTION( MessageCallback ), 0, asCALL_CDECL );
    m_aEngine->SetMessageCallback(asMETHOD( ScriptBase, MessageCallback ), this, asCALL_THISCALL);
    RegisterStdString( m_aEngine );
    RegisterScriptArray( m_aEngine, true );

    m_aEngine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL);

    m_jit = nullptr;
#ifdef __x86_64__
    m_jit = new asCJITCompiler( JIT_NO_SUSPEND | JIT_SYSCALL_NO_ERRORS );
    m_aEngine->SetEngineProperty( asEP_INCLUDE_JIT_INSTRUCTIONS, 1 );
    m_aEngine->SetJITCompiler( m_jit );
#endif
}
ScriptBase::~ScriptBase()
{
    m_aEngine->GarbageCollect( asGC_FULL_CYCLE );  // Automatic garbage collection is disabled, doing it manually

    if( m_context ) m_context->Release();
    if( m_aEngine ) m_aEngine->ShutDownAndRelease();
    if( m_jit )     delete m_jit;
}

void ScriptBase::setScriptFile( QString scriptFile, bool )
{
    m_scriptFile = scriptFile;
    m_scriptFolder = QFileInfo( scriptFile ).absolutePath();
    setScript( fileToString( scriptFile, "ScriptBase::setScriptFile" ) );
}

void ScriptBase::setScript( QString script )
{
    m_script = script;
}

int ScriptBase::compileScript()
{
    if( !m_aEngine ) return -1;

    m_aEngine->GarbageCollect( asGC_FULL_CYCLE );
    m_asModule = m_aEngine->GetModule( 0, asGM_ALWAYS_CREATE );

    int r = compileSection( m_scriptFile, m_script );
    if( r < 0 ) return -1;

    r = m_asModule->Build();
    if( r < 0 ) { qDebug() << Qt::endl << m_elmId+" ScriptBase::compileScript Error"<< Qt::endl; return -1; }

    //qDebug() << "\nScriptBase::compileScript: Build() Success\n";
    return 0;
}

int ScriptBase::compileSection( QString sriptFile, QString text )
{
    int ok = 0;

    QStringList lines = text.split("\n");
    text.clear();
    for( QString line : lines )                // Get includes
    {
        if( line.contains("#include") )
        {
            QString file = line.remove("#include").remove(" ");
            while( file.startsWith(" ") ) file = file.right( file.length()-1 );

            if( file.startsWith("\"") )
            {
                file = file.right( file.length()-1 );
                file = file.split("\"").first();
                file.prepend( m_scriptFolder+"/" );
            }
            else if( file.startsWith("<") )
            {
                file = file.remove("<").split(">").first();
                file.prepend( MainWindow::self()->getDataFilePath("scriptlib")+"/" );
            }
            line = fileToString( file, "ScriptBase::compileScript" );
            int r = compileSection( file, line );
            if( r < 0 ) ok = r;
            line.clear();
        }
        text.append( line+"\n");
    }
    std::string script = text.toStdString();

    int r = m_asModule->AddScriptSection( sriptFile.toLocal8Bit().data(), &script[0], script.size() );
    if( r < 0 ) { qDebug() << "\nScriptBase::compileSection: AddScriptSection() failed\n"; return -1; }

    return ok;
}

/*int ScriptBase::SaveBytecode(asIScriptEngine *engine, const char *outputFile)
{
    CBytecodeStream stream;
    int r = stream.Open( outputFile );
    if( r < 0 )
    {
        qDebug() << "Failed to open output file for writing";
        return -1;
    }

    asIScriptBase *mod = engine->GetModule("build");
    if( mod == 0 )
    {
        qDebug() << "Failed to retrieve the compiled bytecode";
        return -1;
    }

    r = mod->SaveByteCode( &stream );
    if( r < 0 )
    {
        qDebug() <<  "Failed to write the bytecode";
        return -1;
    }

    qDebug() <<  "Bytecode successfully saved";

    return 0;
}*/

int ScriptBase::callFunction0( asIScriptFunction* func, asIScriptContext* context )
{
    context->Prepare( func );
    return context->Execute();
}

void ScriptBase::callFunction( asIScriptFunction* func )
{
    prepare( func );
    execute();
}

void ScriptBase::execute()
{
    m_status = m_context->Execute();
    if( m_status != asEXECUTION_FINISHED ) printError( m_context );
}

void ScriptBase::printError( asIScriptContext* context ) // The execution didn't finish as we had planned. Determine why.
{
    if( m_status == asEXECUTION_ABORTED )
        qDebug() << "The script was aborted before it could finish. Probably it timed out.";
    else if( m_status == asEXECUTION_EXCEPTION )
    {
        qDebug() << "The script ended with an exception." ;

        // Write some information about the script exception
        asIScriptFunction* func = m_context->GetExceptionFunction();
        qDebug() << "func:" << func->GetDeclaration();
        qDebug() << "modl:" << func->GetModuleName();
        qDebug() << "sect:" << func->GetScriptSectionName();
        qDebug() << "line:" << context->GetExceptionLineNumber();
        qDebug() << "desc:" << context->GetExceptionString();
    }
    else qDebug() << "The script ended for some unforeseen reason:" << m_status;
}
