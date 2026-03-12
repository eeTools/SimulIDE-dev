
/***************************************************************************
 *   Copyright (C) 2026 by Stefan Persson                                  *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once
#include "plant_model/pythonprocessor.h"
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

    int getSizeInput() { return python_input_v.size(); }
    int getSizeOutput() { return python_output_v.size();  }

    QString getPyFileName() { return m_complete_file_name; }
    QString getPyClassName() { return py_class_name; };

    void setSizeInput( int sizeInput );
    void setSizeOutput( int sizeOutput );
    void setPyFileName( QString pyFileName );
    void setPyClassName( QString pyClassName ) {py_class_name = pyClassName;};

    void createInputs( unsigned int c );
    void deleteInputs( int d );
    void createOutputs( int c );
    void deleteOutputs( int d );

    virtual void remove() override;
    virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

    void showErrorLater(const QString &title, const QString &msg);
    void showCriticalErrorLater(const QString &title, const QString &msg);

protected slots:
    virtual void slotProperties() override;


private:
    void updtProperties();

    QString m_complete_file_name; // = "";
    QString m_folder_name; // = "";
    QString m_file_name; // = "";
    QString m_file_extension; // = "";
    QString py_class_name; // = "Processor";

    PythonProcessor python_processor;

    std::vector<IoPin*> pin_input, pin_output;
    std::vector<double> python_input_v, python_output_v;

    const int comp_x_pos_min = -20;
    const int comp_x_pos_max = 60;
    const int comp_x_left_pin_offset = 8;
    const int comp_x_right_pin_offset = 12;
    const int comp_pin_space = 8;
    const int comp_y_pos_min = -10;
    int maxWithOffset(int a, int b){
        return std::max(6, std::max(a, b));
    }
    void updateGeometry();

    bool hasLoadedPyFile = false;
};
