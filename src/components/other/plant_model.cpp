/***************************************************************************
 *   Copyright (C) 2026 by Stefan Persson                                  *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include <QObject>
#include <QIcon>

#include "circuit.h"
#include "circuitwidget.h"
#include "intprop.h"
#include "iopin.h"
#include "itemlibrary.h"
#include "label.h"
#include "mainwindow.h"
#include "pathval.h"
#include "propdialog.h"
#include "plant_model/pythonprocessor.h"
#include "resistor.h"
#include "simulator.h"
#include "stringprop.h"
#include "stringprop.h"

#include "plant_model.h"

#define tr(str) simulideTr("PlantModel",str)

Component* PlantModel::construct( QString type, QString id )
{ return new PlantModel( type, id ); }

LibraryItem* PlantModel::libraryItem()
{
    return new LibraryItem(
        tr("Python Plant Model"),
        "Other",
        "plantmodel.png",
        "PlantModel",
        PlantModel::construct);
}

PlantModel::PlantModel( QString type, QString id )
    : Component( type, id )
    , eElement( id )
{
    m_changed   = false;
    setSizeInput(1);
    setSizeOutput(1);
    setPyFileName("");
    setPyClassName("Processor");
    setLabelPos(-24,-40, 0);
    Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"), { new IntProp <PlantModel>("Size_Input", tr("Size Input"), ""
                                                       , this, &PlantModel::getSizeInput, &PlantModel::setSizeInput, propNoCopy,"uint" ),
                               new IntProp <PlantModel>("Size_Output", tr("Size Output"), ""
                                                       , this, &PlantModel::getSizeOutput, &PlantModel::setSizeOutput, propNoCopy,"uint" ),
                               new StrProp <PlantModel>("Processor_class_name", tr("Processor class name"), ""
                                                       , this, &PlantModel::getPyClassName, &PlantModel::setPyClassName, propNoCopy,"string" ),
                               new StrProp <PlantModel>("Py_file", tr("py file"), ""
                                                       , this, &PlantModel::getPyFileName, &PlantModel::setPyFileName, propNoCopy,"file" ),
                               },0 } );
    setToolTip(tr("PlantModel: Python-scripted dynamic system\n"
                  "For control loops, physics models, etc.\n"
                  "Select .py file with Processor class\n"
                  "See examples.py for templates\n"
                  "Note: select class name before loading file."));
    qDebug() << "PlantModel" << getUid() << ": created";
}
PlantModel::~PlantModel()
{
    qDebug() << "PlantModel" << getUid() << ": A Plant Model is being destroyed/deleted.";
}

void PlantModel::stamp()    // Called at Simulation Start
{
    qDebug() << "PlantModel" << getUid() << ": Simulation started";
    setPyFileName( PlantModel::getPyFileName() );
    if( !Simulator::self()->isPaused() ) {
        m_changed = true;
    }
}

void PlantModel::updateStep()
{
    //if( !m_changed ) return;
    PythonGIL gil;   // ← one line — now safe & exception-safe
    try
    {
        for (unsigned long i = 0; i < python_input_v.size(); i++) {
            python_input_v[i] = pin_input[i]->getVoltage();
        }
        // Simulation time Simulator::self()->circTime() is in ps
        std::vector<double> values_out =python_processor.process(Simulator::self()->circTime(), python_input_v);
        try
        {
            if (values_out.size() != python_output_v.size()){
                throw std::runtime_error("Diffent length of output vector from pyton script (" + std::to_string(values_out.size()) + ") and expected length in SimulIDE (" + std::to_string(python_output_v.size()) + ")");
            }
        }
        catch (const std::runtime_error& error)
        {
            if( Simulator::self()->isRunning() ) Simulator::self()->stopSim();
            showCriticalErrorLater("Error size output py script: " ,error.what());
        }

        for (unsigned long i = 0; i < python_output_v.size(); i++) {
            pin_output[i]->setVoltage( values_out[i] );
        }
        update();
    }
    catch (const std::runtime_error& error)
    {
        if( Simulator::self()->isRunning() ) Simulator::self()->stopSim();
        showErrorLater("Error running py script: " ,error.what());
    }
}

void PlantModel::createInputs( unsigned int c )
{
    for( int i=python_input_v.size(); i<python_input_v.size()+c; i++ )
    {
        QString reid = m_id;
        reid.append(QString("-resistor"+QString::number(i)));
        pin_input.insert(pin_input.begin() + i, new IoPin( 180, QPoint(comp_x_pos_min-comp_x_left_pin_offset,-4+i*comp_pin_space ), reid+"-ePinInput"+QString::number(i), 0, this, pinMode_t::input));
    }
    python_input_v.resize(python_input_v.size()+c);
}

void PlantModel::deleteInputs( int d )
{
    if (d <= 0 || d > getSizeInput()) return;

    if (Simulator::self()->isRunning())
        CircuitWidget::self()->powerCircOff();

    int old_size = getSizeInput();
    for (int i = old_size - 1; i >= old_size - d; --i)
    {
        deletePin(pin_input[i]);
        pin_input.erase(pin_input.begin() + i);
    }
    python_input_v.resize(old_size - d);

    // Always update geometry after pin count change
    updateGeometry();
}

void PlantModel::createOutputs( int c )
{
    for( int i=python_output_v.size(); i<python_output_v.size()+c; i++ )
    {
        QString reid = m_id;
        reid.append(QString("-resistor"+QString::number(i)));
        pin_output.insert(pin_output.begin() + i, new IoPin( 0, QPoint( comp_x_pos_max-comp_x_right_pin_offset,-4+i*comp_pin_space), reid+"-ePinOutput"+QString::number(i), 0, this, pinMode_t::source));
    }
    python_output_v.resize(python_output_v.size()+c);
}

void PlantModel::deleteOutputs( int d )
{
    if (d <= 0 || d > getSizeOutput()) return;

    if (Simulator::self()->isRunning())
        CircuitWidget::self()->powerCircOff();

    int old_size = getSizeOutput();
    for (int i = old_size - 1; i >= old_size - d; --i)
    {
        deletePin(pin_output[i]);
        pin_output.erase(pin_output.begin() + i);
    }
    python_output_v.resize(old_size - d);

    // Always update geometry after pin count change
    updateGeometry();
}

void PlantModel::setSizeInput( int size )
{
    if( size == getSizeInput() || size < 1 ) return;
    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    if     ( size < getSizeInput() ) deleteInputs( getSizeInput()-size );
    else if( size > getSizeInput() ) createInputs( size-getSizeInput() );
    updateGeometry();
}

void PlantModel::setSizeOutput( int size )
{
    if( size == getSizeOutput() || size < 1 ) return;
    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    if     ( size < getSizeOutput() ) deleteOutputs( getSizeOutput()-size );
    else if( size > getSizeOutput() ) createOutputs( size-getSizeOutput() );
    updateGeometry();
}

void PlantModel::setPyFileName( QString pyFileName )
{
    qDebug() << "PlantModel" << getUid() << ": Selected file" << pyFileName;
    if(pyFileName!=""){
        PythonGIL gil;   // acquire here — released on scope exit (even on throw)
        QFileInfo info(pyFileName);

        QString folder    = info.absolutePath();     // folder name
        QString fileName  = info.baseName();         // name without extension
        QString extension = info.suffix();

        m_folder_name = folder;
        m_file_name = fileName;
        m_file_extension = extension;
        try
        {
            python_processor.PythonProcessorLoadFile(folder.toUtf8().constData(),
                                                     fileName.toUtf8().constData(),
                                                     py_class_name.toUtf8().constData());
            m_complete_file_name = pyFileName;
            hasLoadedPyFile = true;
        }
        catch (const std::runtime_error& error)
        {
            showErrorLater("Error loading py script" ,error.what());
        }
    }
}

void PlantModel::updtProperties()
{
    if( !m_propDialog ) return;
    m_propDialog->adjustWidgets();
}

void PlantModel::slotProperties()
{
    Component::slotProperties();
    updtProperties();
    if (!m_propDialog) return;
}

void PlantModel::remove()
{
    deleteInputs( getSizeInput());
    deleteOutputs( getSizeOutput());
    Component::remove();
}

void PlantModel::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );
    QFont font = p->font() ;
    font.setPointSize(static_cast<int>(std::min(10.0, boundingRect().height() / 10)));
    p->setFont(font);
    p->drawRoundedRect( QRect(comp_x_pos_min, comp_y_pos_min, comp_x_pos_max, 2+maxWithOffset(getSizeInput(), getSizeOutput())*comp_pin_space ), 2, 2 );
    p->drawText(QRectF(comp_x_pos_min, comp_y_pos_min, comp_x_pos_max, 2+maxWithOffset(getSizeInput(), getSizeOutput())*comp_pin_space ), Qt::AlignCenter, "Plant model\n  py\n"+ m_file_name.left(10)+ "\n"+py_class_name.left(10));

    Component::paintSelected( p );
}

void PlantModel::showErrorLater(const QString &title, const QString &msg) {
    QMetaObject::invokeMethod(MainWindow::self(),
                              [title, msg]() {
                                  QMessageBox::warning(nullptr, title, msg);
                              },
                              Qt::QueuedConnection);
}

void PlantModel::showCriticalErrorLater(const QString &title, const QString &msg) {
    QMetaObject::invokeMethod(MainWindow::self(),
                              [title, msg]() {
                                  QMessageBox::critical(nullptr, title, msg);
                              },
                              Qt::QueuedConnection);
}

void PlantModel::updateGeometry()
{
    m_area = QRect(comp_x_pos_min, comp_y_pos_min,
                   comp_x_pos_max,
                   2 + comp_pin_space * maxWithOffset(getSizeInput(), getSizeOutput()));
    Circuit::self()->update();
}
