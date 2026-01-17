/***************************************************************************
 *   Copyright (C) 2010 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "plant_model.h"
#include "itemlibrary.h"

#include "intprop.h"
#include "boolprop.h"
#include <iostream>
#include "iopin.h"

#define tr(str) simulideTr("PlantModel",str)

Component* PlantModel::construct( QString type, QString id )
{ return new PlantModel( type, id ); }

LibraryItem* PlantModel::libraryItem()
{
    return new LibraryItem(
        tr("Plant Model" ),
        "Other",
        "plantmodel.png",
        "Plant Model",
        PlantModel::construct );
}

PlantModel::PlantModel( QString type, QString id )
       : Gate( type, id, 2 )
        //Component( type, id )
{
    PlantModel::updatePath();
    PlantModel::setNumInputs(1);
    setNumOutputs(1);
    m_minInputs = 1;

    addPropGroup( { tr("Main"), IoComponent::familyProps(), 0 } );

    addPropGroup( { tr("Inputs"),
        //IoComponent::inputProps()
        QList<ComProperty*>({
            new IntProp <PlantModel>("Num_Inputs", tr("Input Size"),""
                                 , this, &PlantModel::numInps, &PlantModel::setNumInputs, propNoCopy,"uint" ),

        })
    ,0 } ); // setNumOutputs

    addPropGroup( { tr("Outputs"),
        // Gate::outputProps()
       //+ IoComponent::outputType()
       QList<ComProperty*>({
           //new ComProperty("", tr("Invert Output by Right-Click on Pin"),"","",0),
           new IntProp <PlantModel>("Num_Outputs", tr("Output Size"),""
                                                  , this, &PlantModel::numOuts, &PlantModel::setNumOutputs, propNoCopy,"uint" ),
       })
    ,0 } );

}

void PlantModel::setNumOutputs( int outs )
{
    if( outs < 1 ) return;
    IoComponent::setNumOuts( outs, "");
    updtInPins();
    updtOutPins();
    updatePath();
    std::cout << "Num inputs: " << m_inpPin.size() << " Num outputs: " << m_outPin.size() << std::endl;

}

void PlantModel::setNumInputs( int inputs )
{
    if( inputs < m_minInputs ) return;
    IoComponent::setNumInps( inputs, "");
    updtInPins();
    updtOutPins();
    //m_outPin  [0]->setY( 0 );
    numPinsInOutMax = std::max(m_inpPin.size(), m_outPin.size());
    updatePath();
    std::cout << "Num inputs: " << m_inpPin.size() << " Num outputs: " << m_outPin.size() << std::endl;
}


PlantModel::~PlantModel(){}

void PlantModel::updatePath()
{
    int endY = m_area.height()/2;
    int endX = m_area.width()/2;

    m_path = QPainterPath();
    m_path.moveTo(-endX,-endY);
    m_path.lineTo(-endX,endY+8);
    m_path.lineTo(endX,endY+8);
    m_path.lineTo(endX,-endY);
    m_path.lineTo(-endX,-endY);
    std::cout << endX << "  " << endY << std::endl;

    double volt = m_outPin[0]->getVoltage();
    double volt2 = m_inpPin[0]->getVoltage();




}
