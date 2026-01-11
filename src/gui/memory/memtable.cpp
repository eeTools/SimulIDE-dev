/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <math.h>
#include <QToolTip>
#include <QMenu>

#include "memtable.h"
#include "mainwindow.h"
#include "simulator.h"
#include "utils.h"
#include "memdata.h"

MemTable::MemTable( QWidget* parent, int dataSize, int wordBytes )
        : QWidget( parent )
{
    setupUi(this);

    m_addrBytes = ceil( ceil(log2(dataSize))/8 );
    m_wordBytes = wordBytes;
    m_cellBytes = wordBytes;
    m_byteRatio = 1;
    m_updtCount = 0;
    m_data = nullptr;
    m_hoverItem = nullptr;

    m_canSaveLoad = true;

    resizeTable( dataSize );

    table->setMouseTracking( true );

    setContextMenuPolicy( Qt::CustomContextMenu );

    connect( this, &MemTable::customContextMenuRequested, this, &MemTable::on_context_menu_requested );
    connect( actionSave_Memory_Table, &QAction::triggered, this, &MemTable::saveTable );
    connect( actionLoad_Memory_Table, &QAction::triggered, this, &MemTable::loadTable );
}

void MemTable::updateTable( QVector<int>* data )
{
    if( ++m_updtCount >= 10 ) m_updtCount = 0;
    else                      return;
    setData( data, m_wordBytes );
}

void MemTable::setValue( int address, int val )
{
    m_blocked = true;

    if( m_byteRatio > 1 )
    {
        address *= m_byteRatio;
        int mask = pow(2,8*m_cellBytes)-1;
        for( int i=0; i<m_byteRatio; ++i ) setCellValue( address+i, (val>>(i*8)) & mask );
    }
    else setCellValue( address, val );

    m_blocked = false;
}

void MemTable::setCellValue( int address, int val )
{
    int row = address/16;
    int colRam = address%16;
    int colAscii = colRam +17;

    QTableWidgetItem* item = table->item( row, colRam );
    if( underMouse() && item == m_hoverItem )
    {
        QString values = "Addr: 0x"+valToHex( address, m_addrBytes )
                       +"\nDec: "+decToBase( val, 10, 2*m_cellBytes+1 )
                       +"\nOct: "+decToBase( val,  8, 3*m_cellBytes )
                       +"\nBin: "+decToBase( val,  2, 8*m_cellBytes );

        if( QToolTip::text() != values )
            QToolTip::showText( QCursor::pos(), values, 0, {}, 100000 );
    }

    item->setData( 0, valToHex( val, m_cellBytes ) );
    QString valS = QChar( val&0x00FF );
    for( int i=1; i<m_cellBytes; ++i )
    {
        val >>= 8;
        valS.prepend( QString( QChar( val&0x00FF )) );
    }
    table->item( row, colAscii )->setData( 0, valS );
}

void MemTable::setData( QVector<int>* data, int wordBytes )
{
    m_data = data;

    if( data->size() != m_dataSize
      || wordBytes   != m_wordBytes )
    {
        m_wordBytes = wordBytes;
        m_cellBytes = wordBytes;
        m_byteRatio = 1;
        resizeTable( data->size() );
    }
    for( int i=0; i<data->size(); ++i ) setValue( i, data->at(i) );
}

void MemTable::setCellBytes( int bytes )
{
    if( m_cellBytes != bytes )
    {
        m_cellBytes = bytes;
        m_byteRatio = m_wordBytes/m_cellBytes;
        resizeTable( m_dataSize );
    }
}

void MemTable::resizeTable( int dataSize )
{
    m_blocked = true;

    m_dataSize = dataSize;
    dataSize *= m_byteRatio;

    m_addrBytes = ceil( ceil(log2(dataSize))/8 );

    int rows = dataSize/16;
    if( dataSize%16) rows++;

    table->clear();

    //table->horizontalHeader()->setSectionResizeMode( QHeaderView::Fixed );
    table->setHorizontalHeaderLabels( QStringList()
      <<"00"<<"01"<<"02"<<"03"<<"04"<<"05"<<"06"<<"07"
      <<"08"<<"09"<<"0A"<<"0B"<<"0C"<<"0D"<<"0E"<<"0F"
      << " "
      <<"0"<<"1"<<"2"<<"3"<<"4"<<"5"<<"6"<<"7"
      <<"8"<<"9"<<"A"<<"B"<<"C"<<"D"<<"E"<<"F");

    table->setRowCount( rows );

    float scale = MainWindow::self()->fontScale();
    QFont font;
    font.setFamily("Ubuntu Mono");
    font.setPixelSize( round(13*scale) );

    table->horizontalHeader()->setFont( font );
    table->verticalHeader()->setFont( font );

    QTableWidgetItem* it;

    for( int row=0; row<rows; ++row )
    {
        it = new QTableWidgetItem(0);
        it->setFlags( Qt::ItemIsEnabled );
        it->setFont( font );
        it->setText( " 0x"+valToHex( row*16, m_addrBytes )+" ");
        table->setVerticalHeaderItem( row, it );

        for( int col=0; col<33; ++col )
        {
            it = new QTableWidgetItem(0);
            if( col == 16 ) it->setFlags( Qt::NoItemFlags );
            else{
                if( col < 16 )
                {
                    font.setWeight( QFont::DemiBold );
                    it->setForeground( QColor( 0x202090 ) );
                }
                else font.setWeight( QFont::Medium );
                it->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
                it->setFont( font );
                it->setTextAlignment( Qt::AlignHCenter );
            }
            table->setItem( row, col, it );
        }
        table->setRowHeight( row, 20*scale );
    }
    for( int col=0; col<16; ++col )
    {
        table->setColumnWidth( col, (18*m_cellBytes+5)*scale ); /// (2+m_cellBytes)*15*scale+2 );
        table->setColumnWidth( col+17, (8*m_cellBytes+4+4)*scale );
    }
    table->setColumnWidth( 16, 5 );

    m_blocked = false;
}

void MemTable::setAddrSelected( int addr, bool jump )
{
    if( addr >= m_dataSize ) return;
    int row = addr/16;
    int col = addr%16;
    cellClicked( row, col );
    if( jump ) table->scrollToItem( table->item( row, col ) );
}

void MemTable::on_table_itemChanged( QTableWidgetItem* item )
{
    if( m_blocked ) return;
    m_blocked = true;
    bool running = Simulator::self()->simState() > SIM_PAUSED;
    if( running ) Simulator::self()->pauseSim();

    int col = item->column();
    int row = item->row();
    int val=0;
    bool ok;

    if( col > 16 )
    {
        ok = item->text().size() == m_cellBytes;
        if( ok ){
            for( int i=0; i<m_cellBytes; ++i )
            {
                QChar cv = item->text().at(m_cellBytes-i-1);
                val += cv.cell() << (8*i);
            }
            col -= 17;
        }
    }
    else val = item->text().toInt( &ok, 0 );

    int tableAddr = row*16+col;
    int dataAddr = tableAddr/m_byteRatio;

    if( ok ) setCellValue( tableAddr, val );

    if( ok && (m_byteRatio > 1) )
    {
        int start = dataAddr*m_byteRatio;
        for( int i=0; i<m_byteRatio; ++i )
        {
            int addr = start+i;
            int row = addr/16;
            int colRam = addr%16;
            int cellVal = table->item( row, colRam )->text().toInt( &ok, 0 );
            val |= cellVal<<(i*8);
        }
    }

    if( ok )
    {
        if( m_data) m_data->replace( dataAddr, val );
        emit dataChanged( dataAddr, val );
    }
    else if( m_data) setValue( dataAddr, m_data->at( dataAddr ) );

    if( running ) Simulator::self()->resumeSim();

    m_blocked = false;
}

void MemTable::on_table_itemEntered( QTableWidgetItem* item )
{
    m_hoverItem = item;
}

void MemTable::on_context_menu_requested( const QPoint &pos )
{
    if (!m_canSaveLoad)
        return;
    QMenu menu( this );

    menu.addAction( actionSave_Memory_Table );
    menu.addAction( actionLoad_Memory_Table );
    menu.exec( this->mapToGlobal( pos ) );
}

void MemTable::saveTable()
{
    if (m_data)
        MemData::saveData( m_data );
    else {
        QVector<int> data { toIntVector() };
        MemData::saveData( &data );
    }

}

void MemTable::loadTable()
{
    QVector<int> oldData { toIntVector() };
    QVector<int> data(m_dataSize);
    if ( MemData::loadData( &data,false ) ) {
        for( int i=0; i<m_dataSize; ++i ) {
            if ( oldData[i] != data[i] ) {
                setValue(i, data[i]);
                if ( m_data )
                    m_data->replace(i, data[i]);
                emit dataChanged(i, data[i]);
            }
        }
    }
}

void MemTable::cellClicked( int row, int col )
{
    if( col == 16 ) return;

    if( col > 16 ) col -= 17;
    table->clearSelection();
    table->item( row, col )->setSelected( true );
    table->item( row, col+17 )->setSelected( true );
}

QString MemTable::valToHex( int val, int bytes )
{
    QString sval = QString::number( val, 16 ).toUpper();
    sval = sval.right( bytes*2 );
    while( sval.length() < bytes*2) sval.prepend( "0" );

    return sval;
}

QVector<int> MemTable::toIntVector()
{
    QVector<int> data( m_dataSize );
    int rows = m_dataSize/16;
    if ( m_dataSize%16 ) rows++;
    int i = 0;
    bool ok;
    for ( int row = 0; row < rows; row++ ) {
        for ( int col = 0; col < 16; col++ ) {
            data[i] = table->item( row, col )->data(0).toString().toInt( &ok, 16 );
            i++;
            if ( i >= m_dataSize ) break;
        }
    }
    return data;
}
