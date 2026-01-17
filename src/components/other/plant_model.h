/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once
#include "pythonprocessor.h"
#include "component.h"
#include "e-element.h"

class LibraryItem;
class eResistor;
class IoPin;

class PlantModel : public Component, public eElement
{
public:
    PlantModel( QString type, QString id );
    ~PlantModel();

    static Component* construct( QString type, QString id );
    static LibraryItem *libraryItem();


    virtual void stamp() override;
    virtual void updateStep() override;

    int sizeInput() { return m_size_input; }
    int sizeOutput() { return m_size_output; }

    QString pyFileName() { return m_file_name; }
    QString pyClassName() { return m_class_name; };


    QString runsetFile(){QString a; return a;};

    void setSizeInput( int sizeInput );
    void setSizeOutput( int sizeOutput );

    void setPyFileName( QString pyFileName );

    void setPyClassName( QString pyClassName ) {m_class_name = pyClassName;};

    void createInputs( int c );
    void deleteInputs( int d );
    void createOutputs( int c );
    void deleteOutputs( int d );

    virtual void remove() override;

    virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

protected slots:
    virtual void slotProperties() override;

private:
    void updtProperties();


    int m_size_output, m_size_input, m_size_total;
    QString m_folder_name = "";
    QString m_file_name = "";

    QString m_class_name = "Processor";

    PythonProcessor python_processor;

    std::vector<IoPin*> m_pin;
    std::vector<eResistor*> m_resistor;

    static eNode m_puEnode;
};
