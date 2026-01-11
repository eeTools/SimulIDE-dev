/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QDebug>
#include <QPainter>
#include <QFileInfo>

#include "stm32.h"
//#include "ustm32pin.h"
#include "stm32port.h"
#include "stm32usart.h"
#include "stm32twi.h"
//#include "stm32spi.h"

#define tr(str) simulideTr("Stm32",str)

//enum armActions{
//    STM32_GPIO_OUT = 1,
//    STM32_GPIO_CRx,
//    STM32_GPIO_IN,
//    STM32_ALT_OUT,
//    STM32_REMAP
//};

#define IOMEM_BASE 0x40000000
#define IOMEM_SIZE 0x00023400
#define GPIOA_BASE 0x40010800 // Port A
#define GPIO_SIZE  0x00000400


Stm32::Stm32( QString type, QString id, QString device )
     : QemuDevice( type, id )
{
    m_area = QRect(-32,-32, 64, 64 );

    m_device = device;
    QString model = device.right( 3 );

    QStringList pkgs = { "T", "C", "R", "V", "Z" };
    uint32_t pkg = pkgs.indexOf( model.mid(1,1) );
    switch( pkg ){
    case 0: m_packageFile = "stm32_dip36.package";  break; // T
    case 1: m_packageFile = "stm32_dip48.package";  break; // C
    case 2: m_packageFile = "stm32_dip64.package";  break; // R
    case 3: m_packageFile = "stm32_dip100.package"; break; // V
    //case 4: m_packageFile = "stm32_dip144.package"; break; // Z

    default: m_packageFile = "stm32_dip64.package";  break;
    }

    QStringList vars = { "4", "6", "8", "B", "C", "D", "E", "F", "G" };
    uint32_t var = vars.indexOf( model.right(1) );

    switch( var )                                              //   Flash/RAM
    {
        case 0: {                                              // 4   16k/6k
            //m_FLASH_SIZE  = 16 * 1024;
            //m_SRAM_SIZE   =  6 * 1024;
            m_portN = 4; m_usartN = 2; m_i2cN = 1; m_spiN = 1;
        } break;
        case 1: {                                              // 6   32k/10k
            //m_FLASH_SIZE  = 32 * 1024;
            //m_SRAM_SIZE   = 10 * 1024;
            m_portN = 4; m_usartN = 2; m_i2cN = 1; m_spiN = 1;
        } break;
        case 2:{                                               // 8   64k/20k
            //m_FLASH_SIZE  = 64 * 1024;
            //m_SRAM_SIZE   = 20 * 1024;
            m_portN = 5; m_usartN = 3; m_i2cN = 2; m_spiN = 2;
        } break;
        case 3: {                                              // B  128k/20k
            //m_FLASH_SIZE  = 128 * 1024;
            //m_SRAM_SIZE   =  20 * 1024;
            m_portN = 5; m_usartN = 3; m_i2cN = 2; m_spiN = 2;
        } break;
        case 4: {                                              // C  256k/48k
            //m_FLASH_SIZE  = 256 * 1024;
            //m_SRAM_SIZE   =  48 * 1024;
            m_portN = 7; m_usartN = 5; m_i2cN = 2; m_spiN = 3;
            } break;
        case 5: {                                              // D  384k/64k
            //m_FLASH_SIZE  = 384 * 1024;
            //m_SRAM_SIZE   =  64 * 1024;
            m_portN = 7; m_usartN = 5; m_i2cN = 2; m_spiN = 3;
        } break;
        case 6: {                                              // E  512k/64k
            //m_FLASH_SIZE  = 512 * 1024;
            //m_SRAM_SIZE   =  64 * 1024;
            m_portN = 7; m_usartN = 5; m_i2cN = 2; m_spiN = 3;
        } break;
        //else if( var == "F" ) m_usartN = 3; m_timerN = 4; m_i2cN = 2; // F  768k/96k
        //else if( var == "G" ) m_usartN = 3; m_timerN = 4; m_i2cN = 2; // G    1M/96k
        default: break;
    }

    uint32_t fam = model.left(1).toInt();

    m_model = fam << 16 | pkg << 8 | var;
    qDebug() << "Stm32::Stm32 model" << device << m_model;
    m_executable = "./data/STM32/qemu-system-arm";

    m_firmware ="";

    m_ioMem.resize( IOMEM_SIZE, 0 );
    m_ioMemStart = IOMEM_BASE;

    createPorts();

    m_i2cs.resize( m_i2cN );
    if( m_i2cN > 0 ) m_i2cs[0] = new Stm32Twi( this, "I2C1", 0, &m_apb1, 0x00005400, 0x000057FF);
    if( m_i2cN > 1 ) m_i2cs[1] = new Stm32Twi( this, "I2C2", 1, &m_apb1, 0x00005800, 0x00005BFF);

    //m_spis.resize( m_spiN );
    //for( int i=0; i<m_spiN; ++i ) m_spis[i] = new Stm32Spi( this, "I2C"+QString::number(i), i );

    m_usarts.resize( m_usartN );
    if( m_usartN > 0 ) m_usarts[0] = new Stm32Usart( this, "Usart1", 0, &m_apb2, 0x00013800, 0x00013BFF );
    if( m_usartN > 1 ) m_usarts[1] = new Stm32Usart( this, "Usart2", 1, &m_apb1, 0x00004400, 0x000047FF );
    if( m_usartN > 2 ) m_usarts[2] = new Stm32Usart( this, "Usart3", 2, &m_apb1, 0x00004800, 0x00004BFF );
    if( m_usartN > 3 ) m_usarts[3] = new Stm32Usart( this, "Uart4" , 3, &m_apb1, 0x00004C00, 0x00004FFF );
    if( m_usartN > 4 ) m_usarts[4] = new Stm32Usart( this, "Uart5" , 4, &m_apb1, 0x00005000, 0x000053FF );

    //m_timers.resize( m_timerN );
    //for( int i=0; i<m_timerN; ++i ) m_timers[i] = new QemuTimer( this, "Timer"+QString::number(i), i );

    m_dummyModule = new QemuModule( this, "UnMapped", 0, nullptr, 0, IOMEM_SIZE );
}
Stm32::~Stm32(){}

bool Stm32::createArgs()
{
    //QFileInfo fi = QFileInfo( m_firmware );

    /*if( fi.size() != 1048576 )
    {
        qDebug() << "Error firmware file size:" << fi.size() << "must be 1048576";
        return false;
    }*/
    m_arena->regData = m_model;

    m_arguments.clear();

    m_arguments << m_shMemKey;          // Shared Memory key

    m_arguments << "qemu-system-arm";

    QStringList extraArgs = m_extraArgs.split(",");
    for( QString arg : extraArgs )
    {
        if( arg.isEmpty() ) continue;
        m_arguments << arg;
    }

    //m_arguments << "-d";
    //m_arguments << "in_asm";

    //m_arguments << "-machine";
    //m_arguments << "help";

    m_arguments << "-M";
    m_arguments << "stm32-f10xx";

    m_arguments << "-drive";
    m_arguments << "file="+m_firmware+",if=pflash,format=raw";

    //m_arguments << "-kernel";  // Does not work
    //m_arguments <<  m_firmware ;

    //m_arguments << "-accel";
    //m_arguments << "tcg,tb-size=100";

    //m_arguments << "-rtc";
    //m_arguments <<"clock=vm";

    m_arguments << "-icount";
    m_arguments <<"shift=0,align=off,sleep=off";

    return true;
}

void Stm32::stamp()
{
    if( m_i2cN > 0 ) m_i2cs[0]->setPins( m_ports[1]->getPin(6) , m_ports[1]->getPin(7)  );
    if( m_i2cN > 1 ) m_i2cs[1]->setPins( m_ports[1]->getPin(10), m_ports[1]->getPin(11) );

    //if( m_spiN > 0 ) m_spis[0]->setPins( m_ports[0].at(7) , m_ports[0].at(6) , m_ports[0].at(5) , m_ports[0].at(4)  );
    //if( m_spiN > 1 ) m_spis[1]->setPins( m_ports[1].at(15), m_ports[1].at(14), m_ports[1].at(13), m_ports[1].at(12) );
    //if( m_spiN > 2 ) m_spis[2]->setPins( m_ports[1].at(5) , m_ports[1].at(4) , m_ports[1].at(3) , m_ports[0].at(15) );

    if( m_usartN > 0 ) m_usarts[0]->setPins({m_ports[0]->getPin(9) , m_ports[0]->getPin(10)}); // No Remap (TX/PB6, RX/PB7)
    if( m_usartN > 1 ) m_usarts[1]->setPins({m_ports[0]->getPin(2) , m_ports[0]->getPin(3) }); // No remap (CTS/PA0, RTS/PA1, TX/PA2, RX/PA3, CK/PA4), Remap (CTS/PD3, RTS/PD4, TX/PD5, RX/PD6, CK/PD7)
    if( m_usartN > 2 ) m_usarts[2]->setPins({m_ports[1]->getPin(10), m_ports[1]->getPin(11)});
    if( m_usartN > 3 ) m_usarts[3]->setPins({m_ports[2]->getPin(10), m_ports[2]->getPin(11)});
    if( m_usartN > 4 ) m_usarts[4]->setPins({m_ports[2]->getPin(12), m_ports[3]->getPin(2) });

    QemuDevice::stamp();
}

void Stm32::createPorts()
{
    m_ports.resize( m_portN );

    uint64_t start = GPIOA_BASE-IOMEM_BASE;
    uint64_t end   = start + GPIO_SIZE -1;

    for( int p=0; p<m_portN; ++p )
    {
        QString pId;
        pId.append(QChar('A' + p));
        Stm32Port* port = new Stm32Port( this, "Port"+pId, p, &m_apb2, start, end );
        m_ports[p] = port;

        start += GPIO_SIZE;
        end   += GPIO_SIZE;

        //createPort( &m_ports[i], i+1, QString('A'+i), 16 );
        for( int i=0; i<16; ++i )
        {
            //qDebug() << "Stm32::createPort" << m_id+"-P"+pId+QString::number(i);
            Stm32Pin* pin = port->createPin( i, m_id+"-P"+pId+QString::number(i), this );
            pin->setVisible( false );
        }
    }

    setPackageFile("./data/STM32/"+m_packageFile);
    Chip::setName( m_device );
}

void Stm32::updtFrequency()
{
    m_apb1 = m_arena->regAddr;
    m_apb2 = m_arena->regData;

    for( QemuModule* module : m_modules ) module->freqChanged();
}

//void Stm32::createPort( std::vector<Stm32Pin*>* port, uint8_t number, QString pId, uint8_t n )
//{
//    for( int i=0; i<n; ++i )
//    {
//        //qDebug() << "Stm32::createPort" << m_id+"-P"+pId+QString::number(i);
//        Stm32Pin* pin = new Stm32Pin( number, i, m_id+"-P"+pId+QString::number(i), this );
//        port->emplace_back( pin );
//        pin->setVisible( false );
//    }
//}

//void Stm32::doAction()
//{
//
//}

//uint16_t Stm32::readInputs( uint8_t port )
//{
//    Stm32Port ioPort = m_ports[port-1]; //getPort( port );
//
//    uint16_t state = 0;
//    for( uint8_t i=0; i<ioPort.size(); ++i )
//    {
//        Stm32Pin* ioPin = ioPort.at( i );
//        if( ioPin->getInpState() ) state |= 1<<i;
//    }
//    //qDebug() << "Stm32::doAction GPIO_IN"<< port << state;
//    return state;
//}

//void Stm32::setPortState( uint8_t port, uint16_t state )
//{
//    std::vector<Stm32Pin*> ioPort = m_ports[port-1]; //getPort( port );
//
//    for( uint8_t i=0; i<ioPort.size(); ++i )
//    {
//        Stm32Pin* ioPin = ioPort.at( i );
//        ioPin->setPortState( state & (1<<i) );
//    }
//}

//void Stm32::setPinState( uint8_t port, uint8_t pin, bool state )
//{
//    std::vector<Stm32Pin*> ioPort = m_ports[port]; //getPort( port );
//
//    //qDebug() << "Stm32::setPinState" << port << pin << state;
//
//    Stm32Pin* ioPin = ioPort.at( pin );
//    ioPin->setOutState( state );
//}

//void Stm32::cofigPort( uint8_t port,  uint32_t config, uint8_t shift )
//{
//    std::vector<Stm32Pin*> ioPort = m_ports[port-1]; //getPort( port );
//
//    //qDebug() << "Stm32::doAction GPIO_DIR Port:"<< port << "Directions:" << m_direction;
//
//    for( uint8_t i=shift; i<shift+8; ++i )
//    {
//        Stm32Pin*  ioPin = ioPort.at( i );
//        uint8_t cfgShift = i*4;
//        uint32_t cfgMask = 0b1111 << cfgShift;
//        uint32_t cfgBits = (config & cfgMask) >> cfgShift;
//
//        uint8_t isOutput = cfgBits & 0b0011;  // 0 = Input
//
//        if( isOutput ) // Output
//        {
//            uint8_t   open = cfgBits & 0b0100;
//            pinMode_t pinMode = open ? openCo : output;
//            ioPin->setPinMode( pinMode );
//        }
//        else          // Input
//        {
//            ioPin->setPinMode( input );
//            uint8_t pull = cfgBits & 0b1000;
//            ioPin->setPull( pull>0 );
//        }
//
//        uint8_t mode = cfgBits & 0b1100; // Analog if CNF0[1:0] == 0
//        ioPin->setAnalog( mode==0 );
//        /// TODO: if changing to Not Analog // Restore Port State
//
//        uint8_t alternate = cfgBits & 0b1000;
//        if( !ioPin->setAlternate( alternate>0 ) )              // If changing to No Alternate
//            ioPin->setPortState( (m_state[port] & 1<<i) > 0 ); // Restore Port state
//    }
//}
      // CNF
        // Input mode:
        //     00: Analog mode
        //     01: Input Floating (reset state)
        //     10: Input with pull-up / pull-down
        //     11: Reserved
        // Output mode:
        // CNF0 0 push-pull
        //      1 Open-drain
        // CNF1 0 General purpose output
        //      1 Alternate function output

Pin* Stm32::addPin( QString id, QString type, QString label,
                   int n, int x, int y, int angle, int length, int space )
{
    IoPin* pin = nullptr;
    //qDebug() << "Stm32::addPin" << id;
    if( type.contains("rst") )
    {
        pin = new IoPin( angle, QPoint(x, y), m_id+"-"+id, n-1, this, input );
        m_rstPin = pin;
        m_rstPin->setOutHighV( 3.3 );
        m_rstPin->setPullup( 1e5 );
        m_rstPin->setInputHighV( 0.65 );
        m_rstPin->setInputLowV( 0.65 );
    }
    else{
        //qDebug() << "Stm32::addPin"<<id;
        uint n = id.right(2).toInt();
        QString portStr = id.at(1);
        Stm32Port* port = nullptr;
        if     ( portStr == "A") port = m_ports.at(0);
        else if( portStr == "B") port = m_ports.at(1);
        else if( portStr == "C") port = m_ports.at(2);
        else if( portStr == "D") port = m_ports.at(3);
        else if( portStr == "E") port = m_ports.at(4);

        if( !port ) return nullptr;

        if( n >= port->size() ) return nullptr;

        pin = port->getPin( n );
        if( !pin ) return nullptr;

        pin->setPos( x, y );
        pin->setPinAngle( angle );
        pin->setVisible( true );
    }
    QColor color = Qt::black;
    if( !m_isLS ) color = QColor( 250, 250, 200 );

    //if( type.startsWith("inv") ) pin->setInverted( true );

    pin->setLength( length );
    pin->setSpace( space );
    pin->setLabelText( label );
    pin->setLabelColor( color );
    pin->setFlag( QGraphicsItem::ItemStacksBehindParent, true );
    return pin;
}
