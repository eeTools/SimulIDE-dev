/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "enumval.h"
#include "component.h"
#include "propdialog.h"
#include "comproperty.h"
#include "mainwindow.h"

EnumVal::EnumVal( PropDialog* parent, CompBase* comp, ComProperty* prop )
       : PropVal( parent, comp, prop )
{
    setupUi(this);
}
EnumVal::~EnumVal() {}

void EnumVal::setup( bool isComp )
{
    valLabel->setText( m_property->capt() );
    m_blocked = true;

    setEnums( m_property->unit() );

    QString valStr = m_property->getValStr();
    valueBox->setCurrentIndex( m_enums.indexOf( valStr) );

    QFontMetrics fm( valueBox->font() );
    float scale = fm.horizontalAdvance(" ")/2;
    valueBox->setFixedWidth( 170.0*scale );

    if( !isComp ) showVal->setVisible( false );

    m_blocked = false;
    updtValues();
    this->adjustSize();
}

void EnumVal::setEnums( QString e )
{
    QStringList list = e.split(";");
    m_enums = list.takeFirst().split(",");
    m_enums.removeAll("");

    QStringList enumNames;
    if( list.size() ) enumNames = list.first().split(",");
    else              enumNames = m_enums;
    enumNames.removeAll("");

    bool isBlocked = m_blocked;
    m_blocked = true;
    valueBox->clear();
    for( QString val : enumNames ) valueBox->addItem( val );
    m_blocked = isBlocked;
}

void EnumVal::on_showVal_toggled( bool checked )
{
    if( m_blocked ) return;
    m_blocked = true;

    if( checked ){
        m_component->setPropStr("ShowProp", m_propName );  //setShowProp( m_propName );
        m_component->setPropStr("ValLabelText", valueBox->currentText() ); //setValLabelText( valueBox->currentText() );
    }
    else m_component->setPropStr("ShowProp", "" );  //setShowProp( "" );
    m_propDialog->updtValues();
    m_propDialog->changed();
    m_blocked = false;
}

void EnumVal::on_valueBox_currentIndexChanged( QString val )
{
    if( m_blocked ) return;

    prepareChange();
    int index = valueBox->currentIndex();
    m_property->setValStr( m_enums.at( index ) );

    if( showVal->isChecked() ) m_component->setPropStr("ValLabelText", val ); //setValLabelText( val );
    saveChanges();
}

void EnumVal::updtValues()
{
    if( m_blocked ) return;
    m_blocked = true;
    showVal->setChecked( m_component->getPropStr("ShowProp") == m_propName );
    m_blocked = false;
}

