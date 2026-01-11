/***************************************************************************
 *   Copyright (C) 2024 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <math.h>
//#include <QDebug>

#include "truthtable.h"
#include "testunit.h"
#include "mainwindow.h"

TruthTable::TruthTable( TestUnit* tu, QWidget* parent )
          : QDialog( parent )
{
    setupUi( this );

    m_testUnit = tu;
    m_numInputs = 0;
    m_numOutputs = 0;
}

void TruthTable::setup( QString inputs, QString outputs, std::vector<uint> truthT )
{
    m_inputsStr = inputs;
    m_outputsStr = outputs;
    m_truthVector = truthT;

    QStringList inputList = inputs.split(",");
    inputList.removeAll(" ");
    m_numInputs = inputList.size();
    int rows = pow( 2, m_numInputs );

    QStringList outputList = outputs.split(",");
    outputList.removeAll(" ");
    m_numOutputs = outputList.size();

    int columns = m_numInputs+m_numOutputs+1;

    QStringList header;
    for( QString output : outputList ) header.prepend( output );
    header.prepend("");
    for( QString input : inputList ) header.prepend( input );

    table->clear();
    table->setRowCount( rows );
    table->setColumnCount( columns );
    table->setHorizontalHeaderLabels( header );
    //table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //table->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch ); // QHeaderView::ResizeToContents

    float scale = MainWindow::self()->fontScale();
    int fontSize = round(13*scale);
    QFont font;
    font.setFamily("Ubuntu Mono");
    font.setBold( true );
    font.setPixelSize( fontSize );

    int columnWidth = round( fontSize*1.7 );

    for( int row=0; row<rows; ++row )
    {
        uint truRow = 0;
        if( row < (int)truthT.size() ) truRow = truthT.at( row );

        int bit = m_numOutputs-1;
        for( int col=0; col<columns; ++col )
        {
            QTableWidgetItem* it = new QTableWidgetItem();
            it->setFont( font );

            if( col == m_numInputs )    // Separator Column
            {
                it->setFlags( Qt::NoItemFlags );
                it->setBackground( QColor( 150, 150, 150 ) );
            }
            else                        // Value Column
            {
                it->setFlags( Qt::ItemIsEnabled );
                int value = 0;

                if( col < m_numInputs ) // Input Column
                {
                    value = ( row & 1<<(m_numInputs-col-1));
                    if( value ) it->setBackground( QColor( 150, 255, 200 ) );
                    else        it->setBackground( QColor( 240, 240, 255 ) );
                }
                else                    // Output Column
                {
                    value = (truRow & 1<<bit);
                    bit--;
                    if( value ) it->setBackground( QColor( 100, 255, 100 ) );
                    else        it->setBackground( QColor( 220, 220, 255 ) );
                }
                QString valStr = value ? "H" : "L";
                it->setText( valStr );
                it->setTextAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
            }
            table->setItem( row, col, it );
            table->setColumnWidth( col, columnWidth );
        }
    }
    table->setColumnWidth( m_numInputs, 8 );
}

void TruthTable::setItemColor( QTableWidgetItem* it, bool b )
{
    if( b ) it->setBackground( QColor( 100, 255, 100 ) );
    else    it->setBackground( QColor( 220, 220, 255 ) );
}

bool TruthTable::checkThruth( std::vector<uint>* samples )
{
    bool ok = true;

    for( uint row=0; row<m_truthVector.size(); ++row ) // Get output values from Table
    {
        uint truthRow  = m_truthVector.at( row );
        uint sampleRow = samples->at( row );
        uint size = m_numInputs+m_numOutputs;

        for( int bit=0; bit<m_numOutputs; ++bit )
        {
            QTableWidgetItem* it = table->item( row, size-bit );

            int truthBit  = truthRow  & 1<<bit;
            int sampleBit = sampleRow & 1<<bit;

            if( truthBit == sampleBit ) setItemColor( it, truthBit );
            else{
                ok = false;                                     // Test failed
                QString valStr = sampleBit ? "H" : "L";
                it->setText( valStr );
                it->setBackground( QColor( 255, 150,  10 ) );   // Red color
            }
        }
    }
    return ok;
}

std::vector<uint> TruthTable::getTruthVector()
{
    std::vector<uint> outValues;

    for( int row=0; row<table->rowCount(); ++row ) // Get output values from Table
    {
        uint truRow = 0;
        int bit = m_numOutputs-1;
        int columns = table->columnCount();
        for( int col=columns-m_numOutputs; col<columns; ++col )
        {
            QTableWidgetItem* it = table->item( row, col );
            if( it->text() == "H") truRow |= 1<<bit;
            bit--;
        }
        outValues.emplace_back( truRow );
    }
    return outValues;
}

void TruthTable::on_saveButton_pressed()
{
    setup( m_inputsStr, m_outputsStr, getTruthVector() );
    m_testUnit->save( m_truthVector );
}

void TruthTable::on_runButton_pressed()
{
    setup( m_inputsStr, m_outputsStr, m_truthVector );
    m_testUnit->runTest();
}

void TruthTable::on_clearButton_pressed()
{
    setup( m_inputsStr, m_outputsStr, m_truthVector );
}

void TruthTable::on_table_itemDoubleClicked( QTableWidgetItem* item )
{
    if( item->column() <= m_numInputs ) return;

    QString text = item->text();
    if( text == "H")
    {
        item->setText("L");
        item->setBackground( QColor( 220, 220, 255 ) );
    }else{
        item->setText("H");
        item->setBackground( QColor( 100, 255, 100 ) );
    }
    m_truthVector = getTruthVector();
}

