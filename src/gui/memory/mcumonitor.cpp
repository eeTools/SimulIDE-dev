/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "mcumonitor.h"
#include "simulator.h"
#include "e_mcu.h"
#include "cpu8bits.h"
#include "memtable.h"
#include "mainwindow.h"
#include "utils.h"

#include "watcher.h"

MCUMonitor::MCUMonitor( QWidget* parent, eMcu* mcu )
          : QDialog( parent )
          , m_status( 1, 8 )
          , m_pc( 1, 2 )
{
    setupUi(this);
    hide();

    setWindowFlags( Qt::Window | Qt::WindowTitleHint | Qt::Tool
                   | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint );

    m_processor = mcu;

    m_jumpToAddress = false;

    m_statusReg    = nullptr;
    m_ramTable     = nullptr;
    m_ramMonitor   = nullptr;
    m_flashMonitor = nullptr;
    m_romMonitor   = nullptr;

    createStatusPC();

    horizontalLayout->setStretchFactor( byteButton, 20 );
    QSplitter* spl = nullptr;

    m_watcher = m_processor->cpu()->getWatcher();
    if( m_watcher )
    {
        if( mcu->ramSize() )
        {
            spl = new QSplitter( Qt::Horizontal, this );
            spl->addWidget( m_watcher );
            tabWidget->addTab( spl, tr("Watch") );
        }
        else tabWidget->addTab( m_watcher, tr("Watch") );
    }
    if( mcu->ramSize() )
    {
        m_ramTable = m_processor->getRamTable();
        if( spl ){
            spl->addWidget( m_ramTable );
            this->resize( this->width()*2,this->height() );
            spl->setSizes({350,460});
        }else{
            tabWidget->addTab( m_ramTable, tr("Watch") );
            this->resize( 480,this->height() );
        }

        m_ramMonitor = new MemTable( tabWidget, m_processor->ramSize() );
        connect(m_ramMonitor,
                &MemTable::dataChanged,
                this,
                &MCUMonitor::ramDataChanged);
        tabWidget->addTab( m_ramMonitor, "RAM" );
        jumpButton->setVisible( true );
    }

    if( mcu->flashSize() )
    {
        m_flashMonitor = new MemTable( tabWidget, m_processor->flashSize(), m_processor->wordSize() );
        tabWidget->addTab( m_flashMonitor, "Flash");
        connect(m_flashMonitor,
                &MemTable::dataChanged,
                this,
                &MCUMonitor::flashDataChanged);

    }
    if( mcu->romSize() )
    {
        m_romMonitor   = new MemTable( tabWidget, m_processor->romSize() );
        tabWidget->addTab( m_romMonitor, "EEPROM");
        connect(m_romMonitor,
                &MemTable::dataChanged,
                this,
                &MCUMonitor::eepromDataChanged);
    }
    connect(tabWidget,
            &QTabWidget::currentChanged,
            this,
            &MCUMonitor::tabChanged);

}

void MCUMonitor::ramDataChanged( int address, int val )
{ m_processor->setRamValue( address, val ); }

void MCUMonitor::flashDataChanged( int address, int val )
{ m_processor->setFlashValue( address, val ); }

void MCUMonitor::eepromDataChanged( int address, int val )
{ m_processor->setRomValue( address, val ); }

void MCUMonitor::tabChanged( int )
{
    if( Simulator::self()->isRunning() ) return;
    updateStep();
}

void MCUMonitor::on_byteButton_toggled( bool byte )
{
    if( m_flashMonitor )
    {
        int bytes = byte ? 1 : m_processor->wordSize();
        m_flashMonitor->setCellBytes( bytes );
    }
    updateStep();
}

void  MCUMonitor::on_jumpButton_toggled( bool jump )
{
    m_jumpToAddress = jump;
}

void MCUMonitor::updateStep()
{
    int pc = m_processor->cpu()->getPC();

    if( m_statusReg )
    {
        int status = *m_statusReg; //m_processor->cpu->getStatus();
        for( int i=0; i<8; i++ )
        {
            int bit = status & 1;
            if( bit ) m_status.item( 0, 7-i )->setBackground( QColor( 255, 150, 00 ) );
            else      m_status.item( 0, 7-i )->setBackground( QColor( 120, 230, 255 ) );
            status >>= 1;
        }

        if( byteButton->isChecked() ) pc *= m_processor->wordSize();
        m_pc.item( 0, 0 )->setData( 0, pc );
        m_pc.item( 0, 1 )->setText("0x"+val2hex(pc) );
    }

    if( m_watcher && m_watcher->isVisible() )
    {
        m_watcher->updateValues();
    }
    if( m_ramTable && m_ramTable->isVisible() )
    {
        //if( m_ramTable->getSplitter()->sizes().at(1) > 150 ) // RAM Watcher visible
            m_ramTable->updateValues();
    }
    if( m_ramMonitor && m_ramMonitor->isVisible() ) // RAM MemTable visible
    {
        for( uint32_t i=0; i<m_processor->ramSize(); ++i )
            m_ramMonitor->setValue( i, m_processor->getRamValue(i) );

        if( Simulator::self()->simState() == SIM_RUNNING )
            m_ramMonitor->setAddrSelected( m_ramTable->getCurrentAddr(), m_jumpToAddress );
    }
    if( m_romMonitor && m_romMonitor->isVisible() )
    {
        m_romMonitor->setData( m_processor->eeprom() );
    }
    if( m_flashMonitor && m_flashMonitor->isVisible() )
    {
        for( uint32_t i=0; i<m_processor->flashSize(); ++i )
            m_flashMonitor->setValue( i, m_processor->getFlashValue(i));

        if( Simulator::self()->simState() == SIM_RUNNING
         || Simulator::self()->simState() == SIM_PAUSED )
            m_flashMonitor->setAddrSelected( pc, m_jumpToAddress );
    }
}

void MCUMonitor::updateRamTable()
{
    if( m_ramTable ) m_ramTable->updateItems();
}

void MCUMonitor::createStatusPC()
{
    m_statusReg = m_processor->cpu()->getStatus();
    if( !m_statusReg )
    {
        byteButton->setVisible( false );
        jumpButton->setVisible( false );
        return;
    }

    float scale = MainWindow::self()->fontScale()*0.8;
    int row_heigh = round( 22*scale );
    int font_size = round(14*scale);
    int numberColor = 0x202090;

    QTableWidgetItem* it;
    QFont font;
    font.setFamily("Ubuntu Mono");
    font.setBold( true );
    font.setPixelSize( font_size );
    m_status.setFont( font );
    m_pc.setFont( font );

    m_status.setVerticalHeaderLabels( QStringList()<<" STATUS " );
    m_status.verticalHeader()->setFixedWidth( round(60*scale) );
    m_status.horizontalHeader()->setMaximumSectionSize(row_heigh);
    m_status.horizontalHeader()->hide();
    m_status.setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_status.setRowHeight( 0, row_heigh );
    m_status.setFixedHeight( row_heigh );
    font.setPixelSize( round(12*scale) );
    for( int i=0; i<8; i++ )
    {
        it = new QTableWidgetItem(0);
        it->setFlags( Qt::ItemIsEnabled );
        it->setTextAlignment( Qt::AlignCenter );
        it->setFont( font );
        m_status.setItem( 0, i, it );
        m_status.setColumnWidth( i, row_heigh );
    }
    int wi = row_heigh*8 + round(60*scale);
    m_status.setMinimumWidth( wi );
    m_status.setMaximumWidth( wi );
    QStringList statusBits = m_processor->getStatusBits();
    if( statusBits.size() )
        for( int i=7; i>=0; --i ) m_status.item( 0, i )->setText( statusBits.takeFirst() );

    m_pc.setVerticalHeaderLabels( QStringList()<<" PC ");
    m_pc.verticalHeader()->setFixedWidth( round(31*scale) );
    m_pc.horizontalHeader()->hide();
    m_pc.setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_pc.setRowHeight( 0, row_heigh );
    m_pc.setFixedHeight( row_heigh  );

    font.setPixelSize( font_size );
    it = new QTableWidgetItem(0);
    it->setFlags( Qt::ItemIsEnabled );
    it->setFont( font );
    it->setForeground( QColor( numberColor ) );
    m_pc.setItem( 0, 0, it );
    m_pc.setColumnWidth( 0, round(45*scale) );

    it = new QTableWidgetItem(0);
    it->setFlags( Qt::ItemIsEnabled );
    it->setFont( font );
    it->setForeground( QColor( 0x3030B8 ) );
    m_pc.setItem( 0, 1, it );
    m_pc.setColumnWidth( 1, round(60*scale) );
    wi = round(135*scale);
    m_pc.setMinimumWidth( wi );
    m_pc.setMaximumWidth( wi );

    horizontalLayout->insertWidget( 0, &m_pc);
    horizontalLayout->setStretchFactor( &m_pc, 30 );
    verticalLayout->insertWidget( 0, &m_status);
}
