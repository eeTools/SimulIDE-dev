/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QAction>

#include "sdcard.h"
#include "simulator.h"
#include "itemlibrary.h"
#include "circuit.h"
#include "pin.h"
#include "mainwindow.h"
#include "circuitwidget.h"
#include "utils.h"

#include "stringprop.h"

#define tr(str) simulideTr("SdCard",str)

Component* SdCard::construct( QString type, QString id )
{
    return new SdCard( type, id );
}

LibraryItem* SdCard::libraryItem()
{
    return new LibraryItem(
        tr("SD Card Reader"),
        "Peripherals",
        "sdcard.png",
        "SdCard",
        SdCard::construct );
}

SdCard::SdCard( QString type, QString id )
      : Component( type, id )
      , SpiModule( id )
      , m_pinCS( 180, QPoint(-32, -8 ), id+"-PinCS", 0, this, input  )
      , m_pinDI( 180, QPoint(-32,  0 ), id+"-PinDI", 0, this, input  )
      , m_pinCK( 180, QPoint(-32,  8 ), id+"-PinCK", 0, this, input  )
      , m_pinDO( 180, QPoint(-32, 16 ), id+"-PinDO", 0, this, output )
{
    m_pinCS.setLabelText("CS");
    m_pinDI.setLabelText("DI");
    m_pinCK.setLabelText("CK");
    m_pinDO.setLabelText("DO");

    m_pinDO.setOutHighV( 3.3 );

    m_pin.resize( 4 );
    m_pin= { &m_pinCS, &m_pinDI, &m_pinCK, &m_pinDO };
    for( int i=0; i < 4; i++) m_pin[i]->setLabelColor(Qt::black);

    // TwiModule:
    m_MOSI   = &m_pinDI;
    m_MISO   = &m_pinDO;
    m_SS     = &m_pinCS;
    m_clkPin = &m_pinCK;

    m_area = QRectF(-8*3, -8*2, 8*8, 8*5 );
    setLabelPos(-10,-30, 0 );

    m_diskImage = nullptr;

    Simulator::self()->addToUpdateList( this );

    m_path = QPainterPath();
    m_path.moveTo(-8*3,-8*2 );
    m_path.lineTo( 8*4,-8*2 );
    m_path.lineTo( 8*4,-12 );
    m_path.arcTo( 22,-12, 32, 32, 112, 136 );
    m_path.lineTo( 8*4, 20 );
    m_path.lineTo( 8*4, 8*3 );
    m_path.lineTo(-8*3, 8*3 );
    m_path.lineTo(-8*3,-8*2 );

    addPropGroup( { tr("Main"), {
        new StrProp <SdCard>("File", tr("File"), ""
                           , this, &SdCard::getFile, &SdCard::setFile ),
    },0} );
}

SdCard::~SdCard()
{
    removeCard();
}

void SdCard::stamp()
{
    m_boot = 10;

    resetCard();

    if( m_diskImage ) SpiModule::setMode( SPI_SLAVE );
    else              SpiModule::setMode( SPI_OFF );

}

void SdCard::voltChanged()
{
    SpiModule::voltChanged();
    //if( !m_enabled ) resetCard();
}

void SdCard::endTransaction()
{
    SpiModule::endTransaction();

    if( m_boot ){   // Booting
        m_boot--;
        if( m_boot == 0 ) m_useSS = true;
        else              return;
    }

    m_rxReg = m_srReg;
    m_srReg = 0xFF;
    m_rxBytes++;

    switch( m_rxBytes ) {
        case 1:{                               // command
            if( m_replyBytes )
            {
                int index = m_repply[0] - m_replyBytes;
                m_srReg = m_repply[index];
                m_replyBytes--;
                m_rxBytes = 0;
            }
            else if( m_bytesToRead )
            {
                readData();
                m_rxBytes = 0;
            }
            else if( m_bytesToWrite )
            {
                writeData();
                m_rxBytes = 0;
            }
            else if( (m_rxReg & 0b11000000) == 0b01000000 ) // Check for Host start bits 01
            {
                m_command = m_rxReg & 0b00111111;
                m_arg = 0;
                m_cmdBuff[0] = m_rxReg;
            }
            else m_rxBytes = 0;                /// TODO: Report Error?
        }break;
        case 2: m_arg |= m_rxReg << 8*3; m_cmdBuff[1] = m_rxReg; break; // 1/4 parameter
        case 3: m_arg |= m_rxReg << 8*2; m_cmdBuff[2] = m_rxReg; break; // 2/4 parameter
        case 4: m_arg |= m_rxReg << 8*1; m_cmdBuff[3] = m_rxReg; break; // 3/4 parameter
        case 5: m_arg |= m_rxReg << 8*0; m_cmdBuff[4] = m_rxReg; break; // 4/4 parameter
        case 6:  // crc
        {
            if( m_doCRC && m_rxReg != (( CRC7(m_cmdBuff, 5) << 1) | 1) )
                m_R1 |= 1<<3;  /// FIXME???     // R1 |0|ParameterE|AddressE|EraseE|CRCE|IllegalC|EraseR|idle|

            //qDebug() << "SdCard::endTransaction Command" << m_command;

            m_repply = nullptr;
            m_bytesToWrite = 0;
            m_bytesToRead  = 0;
            m_replyBytes = 0;
            m_rxBytes = 0;

            // In idle state, the card accepts only CMD0, CMD1, ACMD41 and CMD8 /// CMD58 is error?.
            if( (m_R1 & 1) && m_command != 0 && m_command != 8
             && m_command != 41 && m_command != 55 && m_command != 58 && m_command != 59 )
            {
                m_R1 |= 1<<2;   // Illegal Command
                //qDebug() << "Warning: SdCard::endTransaction Illegal Command in Idle State";
            }
            else if( m_appCom ) appCommand();
            else                command();

            m_srReg = m_R1;    // Response
            m_replyIndex = 0;
            if( m_repply ) m_replyBytes = m_repply[0]-1;

            //qDebug() << "SdCard::endTransaction Reply:" << m_srReg << "reply data:"<<m_replyBytes;
        }break;
        default: readData();
    }
}

void SdCard::command()
{
    switch( m_command ) {
    case  0: resetCard();         break; // GO_IDLE_STATE - init card in spi mode if CS low
    case  1:                      break; // SEND_OP_COND
          /// TODO: Initialization should take hundreds of ms
    case  8: m_repply = d_COND;          // SEND_IF_COND - verify SD Memory Card interface operating condition.
        m_repply[4] = m_arg & 0xFF; // check patern
        break;
    case  9: m_repply = d_CSD;    break; // SEND_CSD - Read CSD register
    case 10: m_repply = d_CID;    break; // SEND_CID - Read CID register
    case 12: m_multi = 0;         break; // STOP_TRANSMISSION - end multiple block sequence
    case 13:                             // SEND_STATUS - read the card status register
        m_repply = d_Packet;
        m_repply[1] = 0x00;  // R2
        break;
    case 16:                      break; // SET_BLOCKLEN - Set block length (in bytes) for all following block commands
        /// TODO: set block size
    case 17: // Fallthrough                 READ_SINGLE_BLOCK
    case 18:                             // READ_MULTIPLE_BLOCK until a STOP_TRANSMISSION
        m_repply = d_Packet;
        m_repply[1] = 0xFE;  // start block
        m_multi = (m_command == 18);
        //qDebug() << "SdCard::command: read at" << m_arg << m_multi;
        if( !m_diskImage->seek( m_arg ) ){ qDebug() << "SdCard::command: File seek failed!"; return; }
        readDataBlock();
        break;
    case 24: // Fallthrough                 WRITE_BLOCK
    case 25:                             // WRITE_MULTIPLE_BLOCK until a STOP_TRANSMISSION
        if( !m_diskImage->seek( m_arg ) ){ qDebug() << "SdCard::command: File seek failed!"; return; }
        m_bytesToWrite = 516; // 1 byte + token + 512 bytes + crc16
        m_multi = (m_command == 25);
        //qDebug() << "SdCard::command: write at" << m_arg << m_multi;
        break;
    case 32: m_start = m_arg/512; break; // ERASE_WR_BLK_START - sets the address of the first block to be erased
    case 33: m_end   = m_arg/512; break; // ERASE_WR_BLK_END - sets the address of the last block of the continuous range to be erased
    case 38: eraseBlocks();       break; // ERASE    - previously selected blocks
    case 55: m_appCom = 1;        break; // APP_CMD  - Set app command
    case 58: m_repply = d_OCR;    break; // READ_OCR - Read OCR register
    case 59: m_doCRC = m_arg;     break; // CRC_OFF  - Set CRC checking
    default:
        //m_reply[0] |= 0x04;  // R1 |0|ParameterE|AddressE|EraseE|CRCE|IllegalC|EraseR|idle|
        qDebug() << "sdcard command CMD%1 not implemented" << m_command;
        break;
    }
}

void SdCard::appCommand()
{
    switch( m_command ) {
    case 13: m_repply = d_STATUS; break; // SD_STATUS - Send the SD Status.
    case 23:                      break; // SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be pre-erased before writing
    case 41: m_R1 = 0;            break; // SD_SEND_OP_COMD - Sends host capacity support information and activates the card's initialization process
         // Initialized, exit idle
        /// TODO: Initialization should take hundreds of ms
    case 42:                             // SET_CLR_CARD_DETECT - Connect[1]/Disconnect[0] the 50KOhm pull-up resistor on CS (pin 1) of the card. The pull-up may be used for card detection.
        /// TODO: m_pinCS.setPullup( 50*1000 ); // qDebug() << "sdcard set_clr_card_detect\n";
        break;
    case 51: m_repply = d_SCR;    break; // SEND_SCR - Reads the SD Configuration Register (SCR).
    default:
    //    m_reply[0] |= 0x04;  // R1 |0|ParameterE|AddressE|EraseE|CRCE|IllegalC|EraseR|idle|
    //    qDebug() << "Warning SdCard::appCommand"<< m_command<<"not implemented" ;
        break;
    }
    m_appCom = 0;
}

void SdCard::readData()
{
    m_srReg = m_buffer[512-m_bytesToRead];
    m_bytesToRead--;

    if( m_bytesToRead == 0 )
    {
        uint16_t crc16 = m_doCRC ? CRC16( m_buffer, 512 ) : 0xFFFFFFFF;

        m_repply = d_CRC;
        m_repply[1] = crc16 >> 8;
        m_repply[2] = crc16 & 0xFF;
        m_replyBytes = 2;

        if( !m_multi ) return;
        m_reply[2] = 0xFF;
        m_reply[3] = 0xFE;  // start block
        m_replyBytes = 5;

        m_arg += 512;
        readDataBlock();
    }
}

void SdCard::writeData()
{
    if( m_bytesToWrite == 515 )  // token
    {
        if( m_rxReg != 0xFE && m_rxReg != 0xFC )
            m_bytesToWrite = 515; // must be 0xFE
    }
    else if( m_bytesToWrite == 1 )          // CRC Low byte
    {
        m_crc16 |= m_rxReg;
        uint8_t resp = 0x05;
        if( m_doCRC && m_crc16 != CRC16( m_buffer, 512 ) )
            resp = 0x0B; // Data rejected due to a CRC error

        m_srReg = resp; // write response
    }
    else if( m_bytesToWrite == 2 )          // CRC High byte
        m_crc16 = (uint16_t)m_rxReg << 8;
    else if( m_bytesToWrite == 3 ){          // Write data to Disk
        int64_t writen = m_diskImage->write( reinterpret_cast<const char*>(m_buffer), 512 );
        if( writen < 0 ) {;} /// TODO: write error
        //qDebug() << "Writen" << writen ;
        //qDebug() << reinterpret_cast<const char*>(m_buffer);
        //qDebug() << m_buffer;
    }
    else {
        m_buffer[514-m_bytesToWrite] = m_rxReg; // Data
    }
    m_bytesToWrite--;
    if( m_bytesToWrite == 0 && m_multi ) m_bytesToWrite = 515;
}

void SdCard::readDataBlock()
{
    m_bytesToRead = 512;
    QByteArray data = m_diskImage->read(512);
    std::copy( data.constData(), data.constData() + qMin(data.size(), 512 ), m_buffer );
    //qDebug() << reinterpret_cast<const char*>(m_buffer);
    //qDebug() << m_buffer;
}

void SdCard::eraseBlocks()
{
    memset( m_buffer, 0, 512 );
    if( !m_diskImage->seek( m_arg ) ){ qDebug() << "SdCard::eraseBlocks: File seek failed"; return; }
    for( uint32_t i = m_start; i<m_end; i++ ) m_diskImage->write( reinterpret_cast<const char*>(m_buffer), 512 );
}

void SdCard::LoadFile()
{
    QDir circuitDir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    QString fil = circuitDir.absolutePath();
    const QString dir = fil;

    QString fileName = QFileDialog::getOpenFileName( 0l, tr("Load Disk Image"), dir,
                       tr("img files (*.img);;All files (*.*)"));

    if( fileName.isEmpty() ) return;
    if( !fileName.endsWith(".img") ) return;

    setFile( fileName );
}

void SdCard::setFile( QString fileName )
{
    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    QDir circuitDir = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    QString fileNameAbs  = circuitDir.absoluteFilePath( fileName );

    if( !QFileInfo::exists( fileNameAbs ) ) {
        qDebug() << "SdCard::setFile Error: file doesn't exist:\n"+fileNameAbs;
        return;
    }
    removeCard();
    m_diskImage = new QFile( fileNameAbs );

    if( !m_diskImage->open( QIODevice::ReadWrite ) ){
        qDebug() << "SdCard::setFile Error: Could not open file:\n"+fileNameAbs;
        removeCard();
        return;
    }
    m_fileName = fileNameAbs;

    m_diskKb = m_diskImage->size() >> 10;  // Size in KB

    int size = m_diskKb/512 - 1;
    d_CSD[9]  = size >> 16;                      // reserved3:2 c_size_high:6
    d_CSD[10] = size >>  8;                      // c_size_mid:8
    d_CSD[11] = size >>  0;                      // c_size_low:8
    d_CSD[17] = (CRC7(&d_CSD[2], 15) << 1) | 1;  // crc:7 always1:1

    uint16_t crc16 = (CRC16(&d_CSD[2], 16));
    d_CSD[18] = crc16 >> 8;                      // CRC16 High
    d_CSD[19] = crc16;                           // CRC16 Low

    if( m_diskKb >= 4194304L) d_SCR[1] = 0xC1;  // Busy:1 CCS:1 resrved:5  S18A:1
    else                      d_SCR[1] = 0x81;  // Busy:1 CCS:1 resrved:5  S18A:1

    qDebug() << "sdcard size =" << m_diskKb << "KB  ->  " << (m_diskImage->size() / 512) << "blocks";
    update();
}

void SdCard::resetCard()
{
    m_repply = nullptr;
    m_appCom = 0;
    m_replyBytes = 0;
    m_replyBytes = 0;
    m_bytesToRead = 0;
    m_bytesToWrite = 0;
    m_rxBytes = 0;
    m_multi = 0;
    m_doCRC = 0;
    m_R1 = 1;           // Idle
}

void SdCard::removeCard()
{
    if( !m_diskImage ) return;
    if( m_diskImage->isOpen() ) m_diskImage->close();
    delete m_diskImage;
    m_diskImage = nullptr;
    update();
}

void SdCard::contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu )
{
    QAction* loadAction = menu->addAction( QIcon(":/load.svg"),tr("Load .img File") );
    QObject::connect( loadAction, &QAction::triggered, [=](){ LoadFile(); } );

    if( m_diskImage ){
        QAction* loadAction = menu->addAction( QIcon(":/detach.png"),tr("Eject SD card") );
        QObject::connect( loadAction, &QAction::triggered, [=](){ removeCard(); } );
    }
    menu->addSeparator();
    Component::contextMenu( event, menu );
}

void SdCard::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );

    if( m_diskImage ){
        p->setBrush( QColor( 50,50,50 ) );
        p->drawRoundedRect( 8,-12, 32, 32, 2, 2 );
        p->setBrush( QColor( 200,255,200 ) );
    }
    else p->setBrush( QColor( Qt::white ) );

    p->drawPath( m_path );
    p->drawText( QRectF(-8*3, -8*2, 54, 8*5 ), Qt::AlignCenter, "SD");

    Component::paintSelected( p );
}
