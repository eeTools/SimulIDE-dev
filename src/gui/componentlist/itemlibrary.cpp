/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/
 
#include "itemlibrary.h"
#include "circuit.h"

//BEGIN Item includes
#include "ampmeter.h"
#include "adc.h"
#include "aip31068_i2c.h"
#include "audio_out.h"
#include "battery.h"
#include "bcdto7s.h"
#include "bcdtodec.h"
#include "bincounter.h"
#include "bjt.h"
#include "buffer.h"
#include "bus.h"
#include "capacitor.h"
#include "clock.h"
#include "comparator.h"
#include "counter.h"
#include "csource.h"
#include "currsource.h"
#include "dac.h"
#include "dcmotor.h"
#include "dectobcd.h"
#include "demux.h"
#include "dht22.h"
#include "diac.h"
#include "dial.h"
#include "diode.h"
#include "ds1307.h"
#include "ds1621.h"
#include "ds18b20.h"
#include "dynamic_memory.h"
#include "elcapacitor.h"
#include "ellipse.h"
#include "esp01.h"
#include "esp32.h"
#include "fixedvolt.h"
#include "flipflopd.h"
#include "flipflopjk.h"
#include "flipfloprs.h"
#include "flipflopt.h"
#include "freqmeter.h"
#include "fulladder.h"
#include "function.h"
#include "gate_and.h"
#include "gate_or.h"
#include "gate_xor.h"
#include "gc9a01a.h"
#include "ground.h"
#include "header.h"
#include "hd44780.h"
#include "ili9341.h"
#include "image.h"
#include "i2cram.h"
#include "i2ctoparallel.h"
#include "inductor.h"
#include "jfet.h"
#include "keypad.h"
#include "ky023.h"
#include "ky040.h"
#include "ks0108.h"
#include "lamp.h"
#include "latchd.h"
#include "ldr.h"
#include "led.h"
#include "ledbar.h"
#include "ledmatrix.h"
#include "ledrgb.h"
#include "line.h"
#include "lm555.h"
#include "logicanalizer.h"
#include "magnitudecomp.h"
#include "max72xx_matrix.h"
#include "mcu.h"
#include "memory.h"
#include "mosfet.h"
#include "mux.h"
#include "mux_analog.h"
#include "op_amp.h"
#include "oscope.h"
#include "pcd8544.h"
#include "pcf8833.h"
#include "plant_model.h"
#include "probe.h"
#include "potentiometer.h"
#include "push.h"
#include "rail.h"
#include "rectangle.h"
#include "relay.h"
#include "resistor.h"
#include "resistordip.h"
#include "rtd.h"
#include "scr.h"
#include "sdcard.h"
#include "serialport.h"
#include "serialterm.h"
#include "servo.h"
#include "sevensegment.h"
#include "sevensegment_bcd.h"
#include "sh1107.h"
#include "shiftreg.h"
#include "socket.h"
#include "sr04.h"
#include "ssd1306.h"
#include "st7735.h"
#include "st7789.h"
#include "stepper.h"
#include "strain.h"
#include "subcircuit.h"
#include "subpackage.h"
#include "switch.h"
#include "switchdip.h"
#include "thermistor.h"
#include "testunit.h"
#include "textcomponent.h"
#include "touchpad.h"
#include "transformer.h"
#include "triac.h"
#include "tunnel.h"
#include "varresistor.h"
#include "voltmeter.h"
#include "volt_reg.h"
#include "voltsource.h"
#include "wavegen.h"
#include "ws2812.h"
#include "zener.h"

//END Item includes

ItemLibrary* ItemLibrary::m_pSelf = nullptr;

ItemLibrary::ItemLibrary()
{
    m_pSelf = this;
    loadItems();
}
ItemLibrary::~ItemLibrary()
{
    for( LibraryItem* item : m_items ) delete item;
}

void ItemLibrary::loadItems()
{
    m_items.clear();
    // Meters
    addItem( new LibraryItem( QObject::tr("Meters"), "", "","Meters", nullptr ) );
    addItem( Probe::libraryItem() );
    addItem( Voltimeter::libraryItem() );
    addItem( Amperimeter::libraryItem() );
    addItem( FreqMeter::libraryItem() );
    addItem( Oscope::libraryItem() );
    addItem( LAnalizer::libraryItem() );
    // Sources
    addItem( new LibraryItem( QObject::tr("Sources"), "", "","Sources", nullptr ) );
    addItem( FixedVolt::libraryItem() );
    addItem( Clock::libraryItem() );
    addItem( WaveGen::libraryItem() );
    addItem( VoltSource::libraryItem() );
    addItem( CurrSource::libraryItem() );
    addItem( Csource::libraryItem() );
    addItem( Battery::libraryItem() );
    addItem( Rail::libraryItem() );
    addItem( Ground::libraryItem() );
    // Switches
    addItem( new LibraryItem( QObject::tr("Switches"), "", "","Switches", nullptr ) );
    addItem( Push::libraryItem() );
    addItem( Switch::libraryItem() );
    addItem( SwitchDip::libraryItem() );
    addItem( Relay::libraryItem() );
    addItem( KeyPad::libraryItem() );
    // Passive
    addItem( new LibraryItem( QObject::tr("Passive"), "", "","Passive", nullptr ) );
    addItem( new LibraryItem( QObject::tr("Resistors"), "Passive", "resistors.png","Resistors", nullptr ) );
    addItem( Resistor::libraryItem() );
    addItem( ResistorDip::libraryItem() );
    addItem( Potentiometer::libraryItem() );
    addItem( VarResistor::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Resistive Sensors"), "Passive", "resistorsensors.png","Resistive Sensors", nullptr ) );
    addItem( Ldr::libraryItem() );
    addItem( Thermistor::libraryItem() );
    addItem( RTD::libraryItem() );
    addItem( Strain::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Reactive"), "Passive", "reactive.png","Reactive", nullptr ) );
    addItem( Capacitor::libraryItem() );
    addItem( elCapacitor::libraryItem() );
    addItem( Inductor::libraryItem() );
    addItem( Transformer::libraryItem() );
    // Active
    addItem( new LibraryItem( QObject::tr("Active"), "", "","Active", nullptr ) );
    addItem( new LibraryItem( QObject::tr("Rectifiers"), "Active", "rectifiers.png","Rectifiers", nullptr ) );
    addItem( Diode::libraryItem() );
    addItem( Zener::libraryItem() );
    addItem( SCR::libraryItem() );
    addItem( Diac::libraryItem() );
    addItem( Triac::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Transistors"), "Active", "transistors.png","Transistors", nullptr ) );
    addItem( BJT::libraryItem() );
    addItem( Mosfet::libraryItem() );
    addItem( Jfet::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Other Active"), "Active", "2to2.png","Other Active", nullptr ) );
    addItem( OpAmp::libraryItem() );
    addItem( Comparator::libraryItem() );
    addItem( VoltReg::libraryItem() );
    addItem( MuxAnalog::libraryItem() );
    // Outputs
    addItem( new LibraryItem( QObject::tr("Outputs"), "", "","Outputs", nullptr ) );
    addItem( new LibraryItem( QObject::tr("Leds"), "Outputs", "leds.png","Leds", nullptr ) );
    addItem( Led::libraryItem() );
    addItem( LedRgb::libraryItem() );
    addItem( LedBar::libraryItem() );
    addItem( SevenSegment::libraryItem() );
    addItem( LedMatrix::libraryItem() );
    addItem( Max72xx_matrix::libraryItem() );
    addItem( WS2812::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Displays"), "Outputs", "displays.png","Displays", nullptr ) );
    addItem( Aip31068_i2c::libraryItem() );
    addItem( GC9A01A::libraryItem() );
    addItem( Hd44780::libraryItem() );
    addItem( Ili9341::libraryItem() );
    addItem( Ks0108::libraryItem() );
    addItem( Pcd8544::libraryItem() );
    addItem( PCF8833::libraryItem() );
    addItem( Sh1107::libraryItem() );
    addItem( Ssd1306::libraryItem() );
    addItem( St7735::libraryItem() );
    addItem( St7789::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Motors"), "Outputs", "motors.png","Motors", nullptr ) );
    addItem( DcMotor::libraryItem() );
    addItem( Stepper::libraryItem() );
    addItem( Servo::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Other Outputs"), "Outputs", "1to1.png","Other Outputs", nullptr ) );
    addItem( AudioOut::libraryItem() );
    addItem( Lamp::libraryItem() );
    // Micro
    addItem( new LibraryItem( QObject::tr("Micro"), "", "","Micro", nullptr ) );
    addItem( QemuDevice::libraryItem() );
    addItem( new LibraryItem("Arduino", "Micro", "board.png", "Arduino", nullptr) );
    addItem( new LibraryItem("AVR"    , "Micro", "ic2.png", "AVR"  , nullptr) );
    addItem( new LibraryItem("PIC"    , "Micro", "ic2.png", "PIC"  , nullptr) );
    addItem( new LibraryItem("I51"    , "Micro", "ic2.png", "I51"  , nullptr) );
    addItem( new LibraryItem("MCS65"  , "Micro", "ic2.png", "MCS65", nullptr) );
    addItem( new LibraryItem("Z80"    , "Micro", "ic2.png", "Z80"  , nullptr) );
    addItem( new LibraryItem("STM32"  , "Micro", "ic2.png", "STM32", nullptr) );
    addItem( new LibraryItem("Espressif", "Micro", "ic2.png", "Espressif", nullptr) );
    addItem( new LibraryItem("Shields", "Micro", "ic2.png", "Shields"  , nullptr) );

    addItem( new LibraryItem( QObject::tr("Sensors"), "Micro", "1to2.png","Sensors", nullptr ) );
    addItem( Mcu::libraryItem() );
    addItem( SR04::libraryItem() );
    addItem( Dht22::libraryItem() );
    addItem( DS1621::libraryItem() );
    addItem( Ds18b20::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Peripherals"), "Micro", "perif.png","Peripherals", nullptr ) );
    addItem( SdCard::libraryItem() );
    addItem( SerialPort::libraryItem() );
    addItem( SerialTerm::libraryItem() );
    addItem( TouchPad::libraryItem() );
    addItem( KY023::libraryItem() );
    addItem( KY040::libraryItem() );
    addItem( DS1307::libraryItem() );
    addItem( Esp01::libraryItem() );
    // Logic
    addItem( new LibraryItem( QObject::tr("Logic"), "", "","Logic", nullptr ) );
    addItem( new LibraryItem( QObject::tr("Gates"), "Logic", "gates.png","Gates", nullptr ) );
    addItem( Buffer::libraryItem() );
    addItem( AndGate::libraryItem() );
    addItem( OrGate::libraryItem() );
    addItem( XorGate::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Arithmetic"), "Logic", "2to2.png","Arithmetic", nullptr ) );
    addItem( Counter::libraryItem() );
    addItem( BinCounter::libraryItem() );
    addItem( FullAdder::libraryItem() );
    addItem( MagnitudeComp::libraryItem() );
    addItem( ShiftReg::libraryItem() );
    addItem( Function::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Memory"), "Logic", "subc.png","Memory", nullptr ) );
    addItem( FlipFlopD::libraryItem() );
    addItem( FlipFlopT::libraryItem() );
    addItem( FlipFlopRS::libraryItem() );
    addItem( FlipFlopJK::libraryItem() );
    addItem( LatchD::libraryItem() );
    addItem( Memory::libraryItem() );
    addItem( DynamicMemory::libraryItem() );
    addItem( I2CRam::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Converters"), "Logic", "1to2.png","Converters", nullptr ) );
    addItem( Mux::libraryItem() );
    addItem( Demux::libraryItem() );
    addItem( BcdToDec::libraryItem() );
    addItem( DecToBcd::libraryItem() );
    addItem( BcdTo7S::libraryItem() );
    addItem( I2CToParallel::libraryItem() );

    addItem( new LibraryItem( QObject::tr("Other Logic"), "Logic", "2to3.png","Other Logic", nullptr ) );
    addItem( ADC::libraryItem() );
    addItem( DAC::libraryItem() );
    addItem( SevenSegmentBCD::libraryItem() );
    addItem( Lm555::libraryItem() );
    // Subcircuits
    addItem( SubCircuit::libraryItem() );
    // Connectors
    addItem( new LibraryItem( QObject::tr("Connectors"), "", "","Connectors", nullptr ) );
    addItem( Bus::libraryItem() );
    addItem( Tunnel::libraryItem() );
    addItem( Socket::libraryItem() );
    addItem( Header::libraryItem() );
    // Graphical
    addItem( new LibraryItem( QObject::tr("Graphical"), "", "","Graphical", nullptr ) );
    addItem( Image::libraryItem() );
    addItem( TextComponent::libraryItem() );
    addItem( Rectangle::libraryItem() );
    addItem( Ellipse::libraryItem() );
    addItem( Line::libraryItem() );
    // Other
    addItem( new LibraryItem( QObject::tr("Other"), "", "","Other", nullptr ) );
    addItem( SubPackage::libraryItem() );
    addItem( TestUnit::libraryItem() );
    addItem( Dial::libraryItem() );
    addItem( PlantModel::libraryItem() );
}

void ItemLibrary::addItem( LibraryItem* item )
{
    if( item ) m_items.append( item );
}

Component* ItemLibrary::createItem( QString type, QString id )
{
    for( LibraryItem* item : m_items )
        if( item->isType( type ) && item->createItemFnPtr() )
            return item->createItemFnPtr()( item->type(), id );

    return nullptr;
}

// CLASS LIBRARYITEM *********************************************************

LibraryItem::LibraryItem( QString name, QString category, QString iconName,
                          QString type, createItemPtr createItem )
{
    m_name       = name;
    m_category   = category;
    m_iconfile   = iconName;
    m_createItem = createItem;

    m_typeList = type.split(",");
    m_type     = m_typeList.first();
}
LibraryItem::~LibraryItem() { }

bool LibraryItem::isType( QString type )
{
    return m_typeList.contains( type );
}
