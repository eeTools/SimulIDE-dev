/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>
#include <QTextDocumentFragment>
#include <QCompleter>
#include <QAbstractItemView>
#include <QStringListModel>
#include <QScrollBar>
#include <QDebug>

#include "codeeditor.h"
#include "outpaneltext.h"
#include "comproperty.h"
#include "highlighter.h"
#include "basedebugger.h"
#include "mainwindow.h"
#include "editorwindow.h"
#include "propdialog.h"
#include "circuit.h"
#include "utils.h"

#include "stringprop.h"
#include "boolprop.h"

static QVector<QPair<QString, QString>> pairs = {
    {"(", ")"},
    {"{", "}"},
    {"[", "]"},
    {"\"", "\""},
    {"'", "'"}
};

QStringList CodeEditor::m_picInstr = QString("addlw addwf andlw andwf banksel bcf bov bsf btfsc btg btfss clrf clrw clrwdt comf decf decfsz goto incf incfsz iorlw iorwf movf movlw movwf reset retfie retlw return rlf rrf sublw subwf swapf xorlw xorwf").split(" ");
QStringList CodeEditor::m_avrInstr = QString("nop add adc adiw call inc sleep sub subi sbc sbci sbiw and andi or ori eor elpm fmul fmuls fmulsu mul muls smp com neg sbr cbr dec tst clr ser mul rjmp ijmp jmp rcall icall ret reti cpse cp cpc cpi sbrc sbrs sbic sbis brbs brbc breq brne brcs break brcc brsh brlo brmi brpl brge brlt brhs brhc brts brtc brvs brvc brie brid mov movw ldi lds ld ldd sts st std lpm in out push pop lsl lsr rol ror asr swap bset bclr sbi cbi bst bld sec clc sen cln sez clz sei cli ses cls sev clv set clt seh clh wdr des eicall eijmp lac las lat spm xch").split(" ");
QStringList CodeEditor::m_i51Instr = QString("nop ajmp ljmp rr inc jbc acall lcall rrc dec jb ajmp ret rl add jnb reti rlc addc jc jnz orl jmp jnc anl jz xrl mov sjmp ajmp movc subb mul cpl cjne push clr swap xch pop setb da djnz xchd movx").split(" ");

QList<CodeEditor*> CodeEditor::m_documents;

CodeEditor::CodeEditor( QWidget* parent, OutPanelText* outPane )
          : QPlainTextEdit( parent )
          , CompBase("File", "" )
{
    m_documents.append( this );

    m_outPane   = outPane;
    m_lNumArea  = new LineNumberArea( this );
    m_hlighter  = new Highlighter( document() );

    m_saveAtClose  = false;
    m_openFiles    = false;
    m_openCircuit  = false;
    m_loadCompiler = false;
    m_loadBreakp   = false;

    m_completer = nullptr;
    m_compiler  = nullptr;
    m_debugLine = 0;
    m_brkAction = 0;
    m_help = "";

    m_tab = EditorWindow::self()->tabString();

    setFont( EditorWindow::self()->getFont() );

    setAcceptDrops( false );

    QPalette p = palette();
    p.setColor( QPalette::Base, QColor( 255, 255, 249) );
    p.setColor( QPalette::Text, QColor( 0, 0, 0) );
    setPalette( p );

    m_completer = new QCompleter( this );
    m_completer->setWidget( this );
    m_completer->setCompletionMode( QCompleter::PopupCompletion );
    m_completer->setCaseSensitivity( Qt::CaseInsensitive );

    connect( m_completer, QOverload<const QString&>::of( &QCompleter::activated ),
             this       , &CodeEditor::insertCompletion );

    connect( this, &CodeEditor::blockCountChanged,
             this, &CodeEditor::updateLineNumberAreaWidth, Qt::UniqueConnection );

    connect( this, &CodeEditor::updateRequest,
             this, &CodeEditor::updateLineNumberArea, Qt::UniqueConnection);

    connect( this, &CodeEditor::cursorPositionChanged,
             this, &CodeEditor::highlightCurrentLine, Qt::UniqueConnection);
    
    setLineWrapMode( QPlainTextEdit::NoWrap );
    updateLineNumberAreaWidth( 0 );
    highlightCurrentLine();

    addPropGroup( { "type", {
        new StrProp <CodeEditor>("itemtype" ,"",""
                                , this, &CodeEditor::itemType, &CodeEditor::setItemType ),
            }, groupHidden} );

    QString enums = EditorWindow::self()->compilers().join(",")
            +","+EditorWindow::self()->assemblers().join(",");

    enums = "None,"+enums+";"+tr("None")+","+enums;

    addPropGroup( { tr("File Settings"), {
        new StrProp <CodeEditor>("Compiler", tr("Compiler"), enums
                                , this, &CodeEditor::compName, &CodeEditor::setCompName, 0, "enum"),

        new BoolProp<CodeEditor>("SaveAtClose", tr("Save Settings at file close"),""
                                , this, &CodeEditor::saveAtClose, &CodeEditor::setSaveAtClose ),

        new ComProperty("", "separator","","",0),

        new ComProperty("", tr("Actions after opening this file:"),"","",0),

        new BoolProp<CodeEditor>("LoadCompiler", tr("Load Compiler"),""
                                , this, &CodeEditor::loadCompiler, &CodeEditor::setLoadCompiler ),

        new BoolProp<CodeEditor>("LoadBreakp", tr("Load Breakpoints"),""
                                , this, &CodeEditor::loadBreakp, &CodeEditor::setLoadBreakp ),

        new BoolProp<CodeEditor>("OpenFiles", tr("Restore files"),""
                                , this, &CodeEditor::openFiles, &CodeEditor::setOpenFiles ),
    }, 0} );

    addPropGroup( { "Hidden", {
        //new StrProp <CodeEditor>("File", "File",""
        //                        , this, &CodeEditor::getFile, &CodeEditor::dummySetter, 0 ),

        new StrProp <CodeEditor>("Circuit", "Circuit",""
                                , this, &CodeEditor::circuit, &CodeEditor::setCircuit, 0 ),

        new StrProp <CodeEditor>("FileList", "FileList",""
                                , this, &CodeEditor::fileList, &CodeEditor::setFileList, 0 ),

        new StrProp <CodeEditor>("Breakpoints", "Breakpoints",""
                                , this, &CodeEditor::breakpoints, &CodeEditor::setBreakpoints, 0 ),
    }, groupHidden} );
}
CodeEditor::~CodeEditor()
{
    m_documents.removeAll( this );
}

void CodeEditor::setSyntaxFile( QString file )
{
    QStringList keyWords = m_hlighter->readSyntaxFile( file );
    addKeyWords( keyWords );
}

void CodeEditor::fileProps()
{
    if( !m_propDialog )
    {
        if( m_help == "" ) m_help = MainWindow::self()->getHelp( "codeeditor" );

        m_propDialog = new PropDialog( this, m_help );
        m_propDialog->setComponent( this, false );
    }
    m_propDialog->show();
}

QString CodeEditor::compName()
{
    if( m_compiler ) return m_compiler->compName();
    return "";
}

void CodeEditor::setCompName( QString name )
{
    if( name .isEmpty() ) return;

    if( m_compiler )
    {
        if( m_compiler->compName() == name  ) return ;
        delete m_compiler;
    }
    m_compiler = EditorWindow::self()->createDebugger( name , this );
    EditorWindow::self()->updateDoc();
}

QString CodeEditor::circuit()
{
    if( !m_openFiles ) return "";
    QString file = Circuit::self()->getFilePath();
    QDir fileDir = QFileInfo( m_file ).absoluteDir();
    file = fileDir.relativeFilePath( file );
    return file;
}
void CodeEditor::setCircuit( QString c )
{
    if( !m_openFiles ) return;
    QDir fileDir = QFileInfo( m_file ).absoluteDir();
    c = fileDir.absoluteFilePath( c );
    Circuit::self()->loadCircuit( c );
}

QString CodeEditor::breakpoints()
{
    if( !m_loadBreakp ) return "";
    QString brkListStr;
    for( int brk : m_brkPoints ) brkListStr.append( QString::number( brk )+"," );
    return brkListStr;
}

void CodeEditor::setBreakpoints( QString bp )
{
    if( !m_loadBreakp ) return;
    QStringList list = bp.split(",");
    list.removeAll("");
    for( QString brk : list ) addBreakPoint( brk.toInt() );
}

QString CodeEditor::fileList()
{
    if( !m_openFiles ) return "";
    QStringList list = EditorWindow::self()->getFiles();

    QString files;
    QDir fileDir = QFileInfo( m_file ).absoluteDir();
    for( QString file : list )
    {
        if( file == m_file ) continue;
        file = fileDir.relativeFilePath( file );
        files.append( file+",");
    }
    return files;
}

void CodeEditor::setFileList( QString fl )
{
    if( !m_openFiles ) return;
    QStringList fileList = fl.split(",");
    fileList.removeOne("");

    QDir fileDir = QFileInfo( m_file ).absoluteDir();
    for( QString file : fileList )
    {
        file = fileDir.absoluteFilePath( file );
        EditorWindow::self()->restoreFile( file );
    }
}

void  CodeEditor::addKeyWords( QStringList words )
{
    m_keyWords.append( words );

    QStringListModel* model = new QStringListModel( m_keyWords );
    m_completer->setModel( model );
}

void CodeEditor::setExtraTypes( QStringList types )
{
    m_hlighter->setExtraTypes( types );
}

void CodeEditor::setFunctions( QStringList words )
{
    m_keyWords.append( words );
    QStringListModel* model = new QStringListModel( m_keyWords + m_objects );
    m_completer->setModel( model );

    QStringList functs;
    for( QString func : words ) functs.append( func.split("(").first() );
    m_hlighter->addMembers( functs );
}

void CodeEditor::setMemberWords( QMap<QString, QStringList> mb )
{
    m_memberWords = mb;
   m_objects = mb.keys();

    QStringListModel* model = new QStringListModel( m_keyWords + m_objects );
    m_completer->setModel( model );

    m_hlighter->addObjects( m_objects );

    QStringList members;
    for( QString object : m_objects )
    {
        QStringList m = mb.value( object );
        for( QString member : m )
        {
            member = "."+member.split("(").first();
            if( !members.contains( member ) ) members.append( member );
        }
    }
    m_hlighter->setMembers( members );
}

void CodeEditor::insertCompletion( QString text )
{
    QTextCursor tc = textCursor();
    tc.select( QTextCursor::WordUnderCursor );
    tc.insertText( text );
    if( text.endsWith(")") && !text.endsWith("()") )
    {
        QString charact = "";
        while( charact != "(" ){
            tc.movePosition( QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            if( tc.atStart() or (tc.positionInBlock() == 0) ) break;
            charact = tc.selectedText()[0];
        }
        tc.removeSelectedText();
        tc.insertText( "()" );
        tc.movePosition( QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor );
    }
    setTextCursor( tc );
}

void CodeEditor::complete( QKeyEvent* e )
{
    QString text = e->text();
    auto ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);

    if( (ctrlOrShift && text.isEmpty()) || e->key() == Qt::Key_Delete)
    {
        return;
    }
    QString word = wordUnderCursor();

    QString lastChar = text.right(1);
    if( word.contains(".") )                // Check if we should match members
    {
        QStringList words = word.split(".");
        word = words.takeLast();
        QString object = words.takeLast();
        QStringList keyWords = m_memberWords.value( object );
        if( !keyWords.isEmpty() )       // This is an object with members
        {
            m_object = object;
            QStringListModel* model = new QStringListModel( keyWords );
            m_completer->setModel( model );
        }
        else m_completer->setModel( new QStringListModel( m_keyWords + m_objects ) );
    }
    else if( !m_object.isEmpty() )
    {
        m_completer->setModel( new QStringListModel( m_keyWords + m_objects ) );
        m_object.clear();
    }

    if( text.isEmpty() || (m_object.length()+word.length() < 2)  )
    {
        m_completer->popup()->hide();
        return;
    }

    if( word != m_completer->completionPrefix() )
    {
        m_completer->setCompletionPrefix( word  );
        m_completer->popup()->setCurrentIndex( m_completer->completionModel()->index(0, 0) );
    }

    auto cursRect = cursorRect();
    cursRect.setWidth(
        m_completer->popup()->sizeHintForColumn(0) +
        m_completer->popup()->verticalScrollBar()->sizeHint().width()
    );

    m_completer->complete( cursRect );
}

QString CodeEditor::wordUnderCursor()
{
    QTextCursor tc = textCursor();
    //tc.select( QTextCursor::WordUnderCursor );
    //return tc.selectedText();

    bool isStartOfWord = false;
    if( tc.atStart() || (tc.positionInBlock() == 0) )
        isStartOfWord = true;

    while( !isStartOfWord ){
        tc.movePosition( QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        if( tc.atStart() or (tc.positionInBlock() == 0) )
            isStartOfWord = true;

        else if( QChar( tc.selectedText()[0]).isSpace() )
            isStartOfWord = true;
    }
    return tc.selectedText().trimmed();
}

void CodeEditor::setFile( QString filePath )
{
    if( m_file == filePath ) return;
    m_file = filePath;
    m_id = getFileName( m_file );

    m_numLines = document()->blockCount();

    if( m_compiler ) delete m_compiler;
    m_compiler = nullptr;
    loadConfig();

    m_outPane->appendLine( "-------------------------------------------------------" );
    m_outPane->appendLine( tr(" File: ")+filePath+"\n" );

    QDir::setCurrent( m_file );
    QString extension = getFileExt( filePath );
    QString code = "00";

    if( extension == ".gcb" )
    {
        setSyntaxFile("gcbasic.syntax");
        if( !m_compiler ) m_compiler = EditorWindow::self()->createDebugger( "GcBasic", this );
    }
    else if( extension == ".cpp"
          || extension == ".c"
          || extension == ".ino"
          || extension == ".h"
          || extension == ".as" )
    {
        setSyntaxFile("cpp.syntax");
        if( extension == ".ino" )
        {   if( !m_compiler ) m_compiler = EditorWindow::self()->createDebugger( "Arduino", this );}
        else if( extension == ".as" )
        {   if( !m_compiler ) m_compiler = EditorWindow::self()->createDebugger( "AScript", this );}
        else if( extension == ".cpp" || extension == ".c") code = "10";
    }
    /*else if( extension == ".s" )
    {
        setSyntaxFile( "avrasm.syntax" );
        m_compiler = EditorWindow::self()->createDebugger( "Avrgcc-asm", this );
    }*/
    else if( extension == ".a51" ) // 8051
    {
        m_outPane->appendLine( "I51 asm\n" );
        setSyntaxFile("i51asm.syntax");
    }
    else if( extension == ".asm" ) // We should identify if pic, avr or i51 asm
    {
        m_outPane->appendText( tr("File recognized as: ") );

        int type = getSyntaxCoincidences();
        if( type == 1 )   // Is Pic
        {
            m_outPane->appendLine( "Pic asm\n" );
            setSyntaxFile("pic14asm.syntax");
        }
        else if( type == 2 )  // Is Avr
        {
            m_outPane->appendLine( "Avr asm\n" );
            setSyntaxFile("avrasm.syntax");
        }
        else if( type == 3 )  // Is 8051
        {
            m_outPane->appendLine( "I51 asm\n" );
            setSyntaxFile("i51asm.syntax");
        }
        else m_outPane->appendLine( "Unknown asm\n" );
    }
    else if( extension == ".xml"
         ||  extension == ".html"
         ||  extension == ".package"
         ||  extension == ".mcu"
         ||  extension == ".sim2"
         ||  extension == ".sim1"
         ||  extension == ".simu" )
    {
        setSyntaxFile("xml.syntax");
    }
    else if( getFileName( m_file ).toLower() == "makefile"  )
    {
        setSyntaxFile("makef.syntax");
    }
    else if( extension == ".hex"
         ||  extension == ".ihx" )
    {
        m_font.setLetterSpacing( QFont::PercentageSpacing, 110 );
        setFont( m_font );
        setSyntaxFile("hex.syntax");
    }
    else if( extension == ".js" )
    {
        setSyntaxFile("js.syntax");
    }
    /*else if( extension == ".sac" )
    {
        //m_compiler = new B16AsmDebugger( this, m_outPane );
    }*/
    if( !m_compiler ) m_compiler = EditorWindow::self()->createDebugger( "None", this, code );
    m_outPane->appendLine( "-------------------------------------------------------" );
}

int CodeEditor::getSyntaxCoincidences()
{
    QStringList lines = fileToStringList( m_file, "CodeEditor::getSintaxCoincidences" );

    double avr=1, pic=1, i51=1; // Avoid divide by 0
    int nlines = 0;
    int matches = 0;

    for( QString line : lines )
    {
        QStringList words = line.toLower().replace("\t", " ").split(" ");
        words.removeAll("");
        if( words.isEmpty() ) continue;
        line = words.first();
        if( line.contains(":") )
        {
            if( words.size() > 1 ) line = words.at(1);
            else continue;
        }
        if( line.isEmpty()
         || line.startsWith("#")
         || line.startsWith(";")
         || line.startsWith(".") ) continue;

        for( QString instruction : m_avrInstr )
        { if( line.contains( QRegularExpression( "\\b"+instruction+"\\b" ) )) {avr++; matches++;} }
        for( QString instruction : m_picInstr )
        { if( line.contains( QRegularExpression( "\\b"+instruction+"\\b" ) )) {pic++; matches++;} }
        for( QString instruction : m_i51Instr )
        { if( line.contains( QRegularExpression( "\\b"+instruction+"\\b" ) )) {i51++; matches++;} }
        if( matches > 200 || ++nlines > 400 ) break;
    }
    if( matches == 0 ) return 0;
    int result = 0;
    pic = matches/pic; avr = matches/avr; i51 = matches/i51;

    if     ( pic < 3 && pic < avr && pic < i51 ) result = 1;
    else if( avr < 3 && avr < pic && avr < i51 ) result = 2;
    else if( i51 < 3 && i51 < pic && i51 < avr ) result = 3;

    return result;
}

bool CodeEditor::compile( bool debug )
{
    if( document()->isModified() )
    {
        if( !EditorWindow::self()->save() )
        {
            m_outPane->appendLine( "Error: File not saved" );
            return false;
    }   }
    m_outPane->appendLine( "-------------------------------------------------------" );
    m_errors.clear();
    m_warnings.clear();
    int error = m_compiler->compile( debug );
    update();

    if( error == 0 ) return true;
    if( error > 0 ) // goto error line number
    {
        setTextCursor( QTextCursor(document()->findBlockByNumber( error-1 )));
        ensureCursorVisible();
    }
    else m_outPane->appendLine( "\n"+tr("     WARNING: Compilation Not Done")+"\n" );
    return false;
}

void CodeEditor::addBreakPoint( int line )
{
    if( EditorWindow::self()->debugState() == DBG_RUNNING ) return;
    if( EditorWindow::self()->debugState() > DBG_STOPPED )
        line = EditorWindow::self()->debugger()->getValidLine( {m_file, line} );
    if( line > 0 && !m_brkPoints.contains( line ) )
    {
        //QTextBlock block = document()->findBlockByNumber( line-1 );
        //UserData* data = (UserData*)block.userData();
        //if( !data ) data = new UserData();
        //data->breakp = true;
        //block.setUserData( data );

        m_brkPoints.append( line );
        update();
    }
}

void CodeEditor::remBreakPoint( int line )
{
    m_brkPoints.removeOne( line );
    //QTextBlock block = document()->findBlockByNumber( line-1 );
    //UserData* data = (UserData*)block.userData();
    //data->breakp = false;
    update();
}

void CodeEditor::startDebug()
{
    setReadOnly( true );
    QList<int> brkPoints;
    for( int line : m_brkPoints )
    {
        line = EditorWindow::self()->debugger()->getValidLine( {m_file, line} );
        if( line > 0 && !brkPoints.contains( line ) ) brkPoints.append( line );
    }
    m_brkPoints = brkPoints;
    update();
}

void CodeEditor::stopDebug()
{
    setReadOnly( false );
    setDebugLine( 0 );
    update();
}

void CodeEditor::showMsg( QString msg )
{
    m_outPane->appendLine( msg+"\n" );
}

void CodeEditor::updateScreen()
{
    setTextCursor( QTextCursor(document()->findBlockByLineNumber( m_debugLine-1 )));
    ensureCursorVisible();
    update();
}

void CodeEditor::keyPressEvent( QKeyEvent* event )
{
    if( m_completer && m_completer->popup()->isVisible() && event->key() == Qt::Key_Return )
    {
        event->ignore();
        return;
    }

    if( event->key() == Qt::Key_Plus && (event->modifiers() & Qt::ControlModifier) )
    {
        EditorWindow::self()->scaleFont( 1 );
    }
    else if( event->key() == Qt::Key_Minus && (event->modifiers() & Qt::ControlModifier) )
    {
        EditorWindow::self()->scaleFont( -1 );
    }
    else if( event->key() == Qt::Key_F && (event->modifiers() & Qt::ControlModifier) )
    {
        EditorWindow::self()->findReplaceDialog();
    }
    else if( event->key() == Qt::Key_Tab )
    {
        if( textCursor().hasSelection() ) indentSelection( false );
        else                              insertPlainText( m_tab );
    }
    else if( event->key() == Qt::Key_Backtab )
    {
        if( textCursor().hasSelection() ) indentSelection( true );
        else textCursor().movePosition( QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor , m_tab.size() );
    }
    else
    {
        QString indent;
        bool extraIndent = false;
        bool indentBrackect = false;
        if( event->key() == Qt::Key_Return )
        {
            QTextCursor tc = textCursor();

            int n0 = 0;
            int n = m_tab.size();
            QString line = tc.block().text();

            while(1)
            {
                QString part = line.mid( n0, n );
                if( part == m_tab ) { n0 += n; indent += m_tab; }
                else break;
            }

            if( tc.positionInBlock() != 0 ) // Indent brackets
            {
                tc.movePosition( QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor );
                if( tc.selectedText() == "{" ) extraIndent = true;
                tc.movePosition( QTextCursor::NextCharacter, QTextCursor::MoveAnchor );

                if( extraIndent ){
                    tc.movePosition( QTextCursor::NextCharacter, QTextCursor::KeepAnchor );
                    if( tc.selectedText() == "}" ) indentBrackect = true;
                    tc.movePosition( QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor );
                }
            }
        }
        QPlainTextEdit::keyPressEvent( event );

        if( event->key() == Qt::Key_Return )
        {
            insertPlainText( indent );
            if( extraIndent ){
                if( indentBrackect ){
                    insertPlainText( "\n"+indent );
                    moveCursor( QTextCursor::MoveOperation::PreviousBlock );
                    moveCursor( QTextCursor::MoveOperation::EndOfBlock );
                }
                insertPlainText( m_tab );
            }
        }else{
            QString text = event->text();
            for( int i=0; i<pairs.size(); ++i )  // Auto close pairs
            {
                QPair<QString, QString> pair = pairs.at(i);
                if( pair.first != text ) continue;
                if( !(m_autoClose & 1<<i) ) continue;
                insertPlainText( pair.second );
                moveCursor( QTextCursor::MoveOperation::Left );
                break;
            }
        }
        if( m_completer ) complete( event );
    }
}

void CodeEditor::deleteSelected()
{
    textCursor().insertText( "" );
}

void CodeEditor::contextMenuEvent( QContextMenuEvent* event )
{
    QMenu menu;

    QAction* undoAction = menu.addAction(QIcon(":/undo.svg"),tr("Undo")+"\tCtrl+Z");
    connect( undoAction, &QAction::triggered,
              this, &CodeEditor::undo, Qt::UniqueConnection );

    QAction* redoAction = menu.addAction(QIcon(":/redo.svg"),tr("Redo")+"\tCtrl+Y");
    connect( redoAction, &QAction::triggered,
              this, &CodeEditor::redo, Qt::UniqueConnection );

    menu.addSeparator();

    QAction* cutAction = menu.addAction(QIcon(":/cut.svg"),tr("Cut")+"\tCtrl+X");
    connect( cutAction, &QAction::triggered,
                  this, &CodeEditor::cut, Qt::UniqueConnection );

    QAction* copyAction = menu.addAction(QIcon(":/copy.svg"),tr("Copy")+"\tCtrl+C");
    connect( copyAction, &QAction::triggered,
                   this, &CodeEditor::copy, Qt::UniqueConnection );

    QAction* pasteAction = menu.addAction(QIcon(":/paste.svg"),tr("Paste")+"\tCtrl+V");
    connect( pasteAction, &QAction::triggered,
                    this, &CodeEditor::paste, Qt::UniqueConnection );

    QAction* removeAction = menu.addAction( QIcon( ":/remove.svg"),tr("Remove") );
    connect( removeAction, &QAction::triggered,
                     this, &CodeEditor::deleteSelected, Qt::UniqueConnection );

    if( !textCursor().hasSelection() ) removeAction->setDisabled( true );

    menu.addSeparator();

    QAction* reloadAction = menu.addAction(QIcon(":/reload.svg"), tr("Reload Document")+"\tCtrl+R");
    connect( reloadAction, &QAction::triggered,
             EditorWindow::self(), &EditorWindow::reload, Qt::UniqueConnection );

    menu.exec( event->globalPos() );
}

void CodeEditor::loadConfig()
{
    QString fileF = m_file +".cfg";
    QFile file( fileF );
    if( !file.exists() )  // No .cfg file, Still support old brk files and file info
    {
        QString fileF = m_file +".brk";
        QFile fileB( fileF );
        if( fileB.exists() )
        {
            QString bp = fileToString( fileF, "EditorWidget::loadConfig" );
            QStringList list = bp.split(",");
            list.removeOne("");
            for( QString brk : list ) addBreakPoint( brk.toInt() );
        }

        QString line = this->document()->findBlockByLineNumber(0).text();
        QString compiler = BaseDebugger::getValueInFile( line, "compiler" );
        if( !compiler.isEmpty() )
        {
            m_outPane->appendLine( tr("Found Compiler definition in file: ") + compiler );
            if( m_compiler != nullptr )
            {
                if( m_compiler->compName() == compiler ) return ;
                delete m_compiler;
            }
            m_compiler = EditorWindow::self()->createDebugger( compiler, this );

            if( m_compiler ){
                m_compiler->setFamily( BaseDebugger::getValueInFile( line, "family" ) );
                m_compiler->setDevice( BaseDebugger::getValueInFile( line, "device" ) );
            }
        }
        return;
    }//---------------------------------------------------------------------------

    QString doc = fileToString( fileF, "EditorWidget::loadConfig" );
    QStringList docLines = doc.split("\n");
    for (const QString &line : docLines)
    {
        if (!line.startsWith("<item")) continue;
        QVector<propStr_t> properties = parseXmlProps(line);

        propStr_t itemType = properties.takeFirst();
        if (itemType.name != "itemtype") continue;
        QString type = itemType.value.toString();

        if      (type == m_type) loadProperties(properties);          // CodeEditor
        else if (type == "Compiler" && m_compiler) m_compiler->loadProperties(properties); // Compiler
    }
}

void CodeEditor::saveConfig()
{
    if( !m_compiler ) return;

    QString fileName = m_file +".cfg";
    QFile file( fileName );
    if( file.exists() ) file.remove();

    QString config = "<document version=\""+QString( APP_VERSION )+"\" rev=\""+QString( REVNO )+"\" ";
    //config += "file=\""+m_file+"\" ";
    config += ">\n";
    config += this->toString();
    if( !compName().isEmpty() ) config += m_compiler->toString();
    config += "\n</document>";

    if( !file.open( QFile::WriteOnly | QFile::Text) )
    {
        m_outPane->appendLine( tr("Cannot write file")+":\n"+fileName+"\n"+file.errorString() );
        return;
    }
    QTextStream out( &file );
    out << config;
    file.close();
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax( 1, blockCount() );
    while( max >= 10 ) { max /= 10; ++digits; }
    return  fontMetrics().height() + fontMetrics().horizontalAdvance( QLatin1Char( '9' ) ) * digits;
}

void CodeEditor::updateLineNumberArea( const QRect &rect, int dy )
{
    if( dy ) m_lNumArea->scroll( 0, dy );
    else     m_lNumArea->update( 0, rect.y(), m_lNumArea->width(), rect.height() );
    if( rect.contains( viewport()->rect() ) ) updateLineNumberAreaWidth( 0 );
}

void CodeEditor::resizeEvent( QResizeEvent* e )
{
    QPlainTextEdit::resizeEvent( e );
    QRect cr = contentsRect();
    m_lNumArea->setGeometry( QRect( cr.left(), cr.top(), lineNumberAreaWidth(), cr.height() ) );
}

QList<int> CodeEditor::getFound()
{
    QList<int> found;
    for( QTextEdit::ExtraSelection es : m_found )
        found.append( es.cursor.blockNumber() );
    return found;
}

void CodeEditor::setFound( QList<QTextEdit::ExtraSelection> sel )
{
    m_found = sel;
    //update();
    highlightCurrentLine();
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    if( m_found.size() ) extraSelections.append( m_found );

    if( !isReadOnly() )
    {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor( 250, 240, 220 );

        selection.format.setBackground( lineColor );
        selection.format.setProperty( QTextFormat::FullWidthSelection, true );
        selection.cursor = textCursor();
        extraSelections.prepend( selection );
    }
    setExtraSelections( extraSelections );
}

void CodeEditor::lineNumberAreaPaintEvent( QPaintEvent* event )
{
    QPainter painter( m_lNumArea );
    painter.fillRect( event->rect(), Qt::lightGray );

    QTextBlock block = firstVisibleBlock();

    int top       = (int)blockBoundingGeometry( block ).translated( contentOffset() ).top();
    int fontSize  = fontMetrics().height();

    int numLines = document()->blockCount();
    if( m_numLines != numLines )                 // Line added or removed: move or remove points
    {
        int delta = m_numLines - numLines;
        m_numLines = numLines;
        bool found = false;
        QTextBlock cBlock = textCursor().block().next();

        QList<int> errors    = m_errors;  // Copy lists to make substitutions
        QList<int> warnings  = m_warnings;
        QList<int> brkPoints = m_brkPoints;

        while( block.isValid() )
        {
            int newLine = block.blockNumber() + 1;
            int oldLine = newLine + delta;
            if( block == cBlock )
            {
                if( delta > 0 ) // Line removed, check if point at cursor line and remove it
                {
                    if( m_brkPoints.contains( newLine ) ) brkPoints.removeOne( newLine );
                    if( m_errors.contains( newLine )    ) errors.removeOne( newLine );
                    if( m_warnings.contains( newLine )  ) warnings.removeOne( newLine );
                }
                found = true;
            }
            if( found ) // Replace lines
            {
                if( m_brkPoints.contains( oldLine ) ) brkPoints.replace( m_brkPoints.indexOf( oldLine ), newLine ); // Replace breakpoint line
                if( m_errors.contains( oldLine )    ) errors.replace(    m_errors.indexOf( oldLine )   , newLine ); // Replace error line
                if( m_warnings.contains( oldLine )  ) warnings.replace(  m_warnings.indexOf( oldLine ) , newLine ); // Replace warning line
            }
            block = block.next();
        }
        m_errors    = errors;      // Replace old lists with new ones
        m_warnings  = warnings;
        m_brkPoints = brkPoints;

        block = firstVisibleBlock();
    }
    while( block.isValid()  && top <= event->rect().bottom() )
    {
        int blockSize = (int)blockBoundingRect( block ).height();
        int bottom = top + blockSize;
        int lineNumber = block.blockNumber() + 1;

        if( block.isVisible() && bottom >= event->rect().top() )
        {
            BaseDebugger* bd = EditorWindow::self()->debugger();
            if( bd && bd->isMappedLine( {m_file,lineNumber} ) )
            {
                painter.setBrush( QColor( 180, 180, 170 ) );
                painter.setPen( Qt::NoPen );
                painter.drawRect( 0, top, fontSize, fontSize );
            }
            if( m_compiler )                            // Breakpoints an error indicator
            {
                painter.setRenderHint( QPainter::Antialiasing );
                //UserData* data = (UserData*)block.userData();

                int pos = m_lNumArea->lastPos;
                if( pos > top && pos < bottom ) // Check if there is a new breakpoint request from context menu
                {
                    if     ( m_brkAction == 1 ) addBreakPoint( lineNumber );
                    else if( m_brkAction == 2 ) remBreakPoint( lineNumber );
                    else if( m_brkAction == 3 ){
                        if( m_brkPoints.contains( lineNumber ) ) remBreakPoint( lineNumber );
                        else                                     addBreakPoint( lineNumber );
                    }
                    m_brkAction = 0;
                    m_lNumArea->lastPos = 0;
                }

                if( m_brkPoints.contains( lineNumber ) ) // Draw breakPoint icon //( data && data->breakp ) //
                {
                    painter.setOpacity( 0.5 );
                    painter.fillRect( QRectF( 0, top+1, fontSize, fontSize-2 ), QColor(50, 50, 0) );
                    painter.setOpacity( 1 );
                    painter.fillRect( QRectF( 1, top+2, fontSize-2, fontSize-4 ), Qt::yellow );
                }

                if( lineNumber == m_debugLine ) // Draw debug line icon
                    painter.drawImage( QRectF( 0, top, fontSize, fontSize ), QImage(":/brkpoint.png") );

                if( m_warnings.contains( lineNumber ) ) // Draw warning
                {
                    painter.setBrush( QColor(220, 120, 0) ); // Orange
                    painter.setPen( Qt::NoPen );
                    QPolygon polygon;
                    polygon << QPoint(1,bottom-2) << QPoint(fontSize/2,top+1) << QPoint(fontSize-1,bottom-2);
                    painter.drawPolygon( polygon );
                    painter.setPen( Qt::white );
                    painter.drawText( 3, top+4, fontSize-6, fontSize-6, Qt::AlignCenter, "!");
                }
                if( m_errors.contains( lineNumber ) ) // Draw error
                {
                    painter.setBrush( QColor(Qt::red) );
                    painter.setPen( Qt::NoPen );
                    painter.drawEllipse( 2, top+2, fontSize-4, fontSize-4 );
                    painter.setPen( Qt::white );
                    painter.drawText( 3, top+3, fontSize-6, fontSize-6, Qt::AlignCenter, "!");
                }
            }
            QString number = QString::number( lineNumber ); // Draw line number
            painter.setPen( Qt::black );
            painter.drawText( 0, top, m_lNumArea->width(), fontSize, Qt::AlignRight, number );
        }
        block = block.next();
        top = bottom;
    }
}

void CodeEditor::indentSelection( bool unIndent )
{
    QTextCursor cur = textCursor();
    int a = cur.anchor();
    int p = cur.position();
    
    cur.beginEditBlock();
     
    if( a > p ) std::swap( a, p );
    
    QString str = cur.selection().toPlainText();
    QString str2 = "";
    QStringList list = str.split("\n");
    
    int lines = list.count();
 
    for( int i=0; i<lines; ++i )
    {
        QString line = list[i];

        if( unIndent ) 
        {
            int n = m_tab.size();
            int n1 = n;
            int n2 = 0;
            
            while( n1 > 0 )
            {
                if( line.size() <= n2 ) break;
                QString car = line.at(n2);
                
                if     ( car == " " ) { n1 -= 1; n2 += 1; }
                else if( car == "\t" ) { n1 -= n; if( n1 >= 0 ) n2 += 1; }
                else n1 = 0;
            }
            line.replace( 0, n2, "" );
        }
        else line.insert( 0, m_tab );
        
        if( i < lines-1 ) line += "\n";
        str2 += line;
    }
    cur.removeSelectedText();
    cur.insertText(str2);
    p = cur.position();

    cur.setPosition( a );
    cur.setPosition( p, QTextCursor::KeepAnchor );

    setTextCursor(cur);
    cur.endEditBlock();
}


// ********************* CLASS LineNumberArea **********************************

LineNumberArea::LineNumberArea( CodeEditor *editor )
              : QWidget(editor)
{
    m_codeEditor = editor;
    lastPos = 0;
}
LineNumberArea::~LineNumberArea(){}

void LineNumberArea::contextMenuEvent( QContextMenuEvent *event)
{
    event->accept();
    
    QMenu menu;

    QAction* addBrkAction = menu.addAction( QIcon(":/breakpoint.png"),tr( "Add BreakPoint" ) );
    connect( addBrkAction, &QAction::triggered,
               m_codeEditor, &CodeEditor::slotAddBreak, Qt::UniqueConnection );

    QAction* remBrkAction = menu.addAction( QIcon(":/nobreakpoint.png"),tr( "Remove BreakPoint" ) );
    connect( remBrkAction, &QAction::triggered,
               m_codeEditor, &CodeEditor::slotRemBreak, Qt::UniqueConnection );

    menu.addSeparator();

    QAction* clrBrkAction = menu.addAction( QIcon(":/remove.svg"),tr( "Clear All BreakPoints" ) );
    connect( clrBrkAction, &QAction::triggered,
               m_codeEditor, &CodeEditor::slotClearBreak, Qt::UniqueConnection );

    if( menu.exec(event->globalPos()) != 0 ) lastPos = event->pos().y();
}

void LineNumberArea::mousePressEvent( QMouseEvent* event )
{
    if( event->button() == Qt::LeftButton )
    {
        lastPos = event->pos().y();
        m_codeEditor->toggleBreak();
        this->repaint();
    }
}

#include "moc_codeeditor.cpp"
