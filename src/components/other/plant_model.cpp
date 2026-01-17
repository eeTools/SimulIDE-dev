/***************************************************************************
 *   Copyright (C) 2010 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "plant_model.h"
#include "circuitwidget.h"
#include "itemlibrary.h"
//#include "connector.h"
#include "simulator.h"
#include "circuit.h"
//#include "pin.h"
#include "resistor.h"
//#include "e-resistor.h"
#include "label.h"
#include "propdialog.h"
#include "mainwindow.h"
#include "iopin.h"
//#include "doubleprop.h"
#include "intprop.h"
//#include "boolprop.h"
#include "pythonprocessor.h"
#include <iostream>
#include <QFileDialog>
#include "pathval.h"

#include <QFileInfo>


#include "stringprop.h"
#include "stringprop.h"

#define tr(str) simulideTr("PlantModel",str)

eNode PlantModel::m_puEnode("");

Component* PlantModel::construct( QString type, QString id )
{ return new PlantModel( type, id ); }

LibraryItem* PlantModel::libraryItem()
{
    return new LibraryItem(
        tr("PlantModel"),
        "Other",
        "PlantModel.png",
        "PlantModel",
        PlantModel::construct);
}

PlantModel::PlantModel( QString type, QString id )
    : Component( type, id )
    , eElement( id )
{
    m_changed   = false;
    m_size_input = 0;
    m_size_output = 0;
    m_size_total = 0;
    setSizeInput(1);
    setSizeOutput(1);
    setPyFileName("");
    setPyClassName("Processor");


    setLabelPos(-24,-40, 0);

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"), { // new StrProp <PlantModel>("File", tr("File"), ""
                                    //                      , this, &PlantModel::setFile, &PlantModel::runsetFile, propNoCopy,"string" ),
                                  new IntProp <PlantModel>("Size Input", tr("Size Input"), ""
                                                           , this, &PlantModel::sizeInput, &PlantModel::setSizeInput, propNoCopy,"uint" ),
                                  new IntProp <PlantModel>("Size Output", tr("Size Output"), ""
                                                          , this, &PlantModel::sizeOutput, &PlantModel::setSizeOutput, propNoCopy,"uint" ),

                                  new StrProp <PlantModel>("Py file", tr("py file"), ""
                                                          , this, &PlantModel::pyFileName, &PlantModel::setPyFileName, propNoCopy,"file" ),
                                  new StrProp <PlantModel>("Py file", tr("py file"), ""
                                                          , this, &PlantModel::pyClassName, &PlantModel::setPyClassName, propNoCopy,"string" ),




                              },0 } );

}
PlantModel::~PlantModel(){}

void PlantModel::stamp()
{
    if( !Simulator::self()->isPaused() ) m_changed = true;
}

void PlantModel::updateStep()
{
    if( !m_changed ) return;
    float in_1  =m_pin[0]->getVoltage();

    std::cout << "Voltage on pin 0: " << in_1 << std::endl;
    //m_pin[1]->setVoltage( 2*in_1 );

    std::cout << Simulator::self()->circTime() << std::endl;  // Simulation time is in ps simulates (confirmed) and it simulates every 5 ms?

    //PyGILState_STATE gstate = PyGILState_Ensure();
    // // call Python
    //PyGILState_Release(gstate);

    std::pair<int, int> value_1 =python_processor.process(in_1);
    m_pin[1]->setVoltage( value_1.first );
    std::cout << "The value is " << value_1.first  << std::endl;


    m_changed = false;
    update();
    stamp();
}

void PlantModel::createInputs( int c )
{
    int start = m_size_input;
    m_size_input=m_size_input+c;
    m_size_total=m_size_total+c;

    for( int i=start; i<m_size_input; i++ )
    {
        int index = i;
        QString reid = m_id;
        reid.append(QString("-resistor"+QString::number(i)));
         m_pin.insert(m_pin.begin() + index, new IoPin( 180, QPoint(-16,-32+8+i*8 ), reid+"-ePin"+QString::number(index), 0, this, pinMode_t::input));
    }
}

void PlantModel::deleteInputs( int d )
{
    if( d > m_size_input ) d = m_size_input;
    int start = m_size_input-d;

    for( int i=start; i<m_size_input; ++i )
    {
        deletePin( m_pin[i] );
        m_pin.erase(m_pin.begin() + i);
    }
    m_size_input = m_size_input-d;
    m_size_total = m_size_total-d;
}

void PlantModel::createOutputs( int c )
{
    int start = m_size_total;
    m_size_output=m_size_output+c;
    m_size_total=m_size_total+c;

    for( int i=start; i<m_size_total; i++ )
    {
        int index = i;
        QString reid = m_id;
        reid.append(QString("-resistor"+QString::number(i)));
        m_pin.insert(m_pin.begin() + index, new IoPin( 0, QPoint( 16,-32+8+(i-m_size_input)*8), reid+"-ePin"+QString::number(index+1), 0, this, pinMode_t::source));
    }


}

void PlantModel::deleteOutputs( int d )
{
    if( d > m_size_output ) d = m_size_output;
    int start = m_size_total-d;

    for( int i=start; i<m_size_total; ++i )
    {
        deletePin( m_pin[i] );
        m_pin.erase(m_pin.begin() + i); 
    }
    m_size_output = m_size_output-d;
    m_size_total = m_size_total-d;
}

void PlantModel::setSizeInput( int size )
{
    if( size == m_size_input || size < 1 ) return;
    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    if     ( size < m_size_input ) deleteInputs( m_size_input-size );
    else if( size > m_size_input ) createInputs( size-m_size_input );

    m_area = QRect( -10, -30, 20, std::max(m_size_input, m_size_output)*8+4 );
    Circuit::self()->update();
}

void PlantModel::setSizeOutput( int size )
{
    std::cout <<" size " << size << " m_size_total " << m_size_total << std::endl;
    if( size == m_size_output || size < 1 ) return;
    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    if     ( size < m_size_output ) deleteOutputs( m_size_output-size );
    else if( size > m_size_output ) createOutputs( size-m_size_output );

    m_area = QRect( -10, -30, 20, std::max(m_size_input, m_size_output)*8+4 );
    Circuit::self()->update();
}

void PlantModel::setPyFileName( QString pyFileName )
{
    if(pyFileName!=""){
    QFileInfo info(pyFileName);

    QString folder    = info.absolutePath();           // or info.path()
    QString fileName  = info.baseName();               // name without extension
    QString extension = info.suffix();

    std::cout << "The string is: " << pyFileName.toStdString() << std::endl;
    m_folder_name = folder;
    m_file_name = fileName;
    //python_processor.setPyFileName(m_folder_name.toStdString(), m_file_name.toStdString());
    python_processor.PythonProcessorLoadFile(folder.toUtf8().constData(),
                                             fileName.toUtf8().constData(),
                                             m_class_name.toUtf8().constData());
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
}

void PlantModel::remove()
{
    //deleteInputs( m_size_input );
    //deleteOutputs( m_size_output );

    Component::remove();
}

void PlantModel::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );

    if( m_ansiSymbol ){
        for( int i=0; i<m_size_total; i++ ) {} //Resistor::drawAnsi( p, 0,-32+8+i*8, 0.7, 0.6 );
    }
    else p->drawRoundedRect( QRect(-9,-28, 18, std::max(m_size_input, m_size_output)*8 ), 2, 2 );
    Component::paintSelected( p );
}
