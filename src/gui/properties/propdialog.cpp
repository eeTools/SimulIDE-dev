/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "propdialog.h"
#include "component.h"
#include "circuit.h"

#include "labelval.h"
#include "numval.h"
#include "pathval.h"
#include "strval.h"
#include "textval.h"
#include "enumval.h"
#include "boolval.h"
#include "colorval.h"
#include "mainwindow.h"

#include "comproperty.h"

PropDialog::PropDialog( QWidget* parent, QString help )
          : QDialog( parent )
{
    setupUi( this );

    setWindowFlags( Qt::Window | Qt::WindowTitleHint | Qt::Tool
                  | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint );

    m_component = nullptr;

    m_helpExpanded = false;
    mainLayout->removeWidget( helpText );
    helpText->document()->setDocumentMargin( 10 );
    helpText->setMarkdown( help.replace("<br>", "<br/>") );
    helpText->adjustSize();
    helpText->setVisible( false );
}

void PropDialog::setComponent( CompBase* comp, bool isComp )
{
    QFontMetrics fm( labelLabel->font() );
    m_scale = fm.horizontalAdvance(" ")/2.0;
    m_minW  = 265*m_scale;
    m_minH  = 100*m_scale;

    QString title = isComp ? "Uid: " : "";
    this->setWindowTitle( title+comp->getUid() );
    type->setText( "Type: "+comp->itemType() );
    if( isComp ) labelBox->setText( comp->getPropStr("label") );
    else{
        labelLabel->setVisible( false );
        labelBox->setVisible( false );
        showButton->setVisible( false );
        labelShow->setVisible( false );
    }
    tabList->clear();
    m_component = comp;
    showButton->setChecked( comp->getPropStr("Show_id") == "true" );

    int index=0;
    QList<propGroup>* groups = comp->properties();

    for( propGroup group : *groups )
    {
        if( group.flags & groupHidden ) continue;

        bool groupEnabled = true;
        bool isMaincomp = comp->isHidden() && (comp->getPropStr("mainComp") == "true"); // main Compoenent & is in subcircuit
        if( isMaincomp && (group.flags & groupNoCopy) ) groupEnabled = false;

        QList<ComProperty*> propList = group.propList;
        if( !propList.isEmpty() )
        {
            index++;
            QWidget* groupWidget = new QWidget( tabList );
            groupWidget->setLayout( new QVBoxLayout( groupWidget ));
            groupWidget->layout()->setSpacing( 9 );
            groupWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
            groupWidget->setObjectName( group.name );

            for( ComProperty* prop : propList )
            {
                if( prop->flags() & propHidden ) continue; // Property hidden

                if( prop->name() == "" ) // Just a label
                {
                    QString text = prop->capt();
                    if( text == "separator")
                    {
                        QFrame* line = new QFrame;
                        line->setFrameShape( QFrame::HLine );
                        line->setFrameShadow( QFrame::Sunken );
                        groupWidget->layout()->addWidget( line );
                    }else{
                        LabelVal* mp = new LabelVal( this );
                        mp->setLabelVal( text );
                        groupWidget->layout()->addWidget( mp );
                    }
                    continue;
                }
                QString type = prop->type();
                PropVal* mp = nullptr;

                if     ( type == "double"  ) mp = new NumVal( this, comp, prop );
                else if( type == "color"   ) mp = new ColorVal( this, comp, prop );
                else if( type == "uint"    ) mp = new NumVal( this, comp, prop );
                else if( type == "int"     ) mp = new NumVal( this, comp, prop );
                else if( type == "string"  ) mp = new StrVal( this, comp, prop );
                else if( type == "path"    ) mp = new PathVal( this, comp, prop, false );
                else if( type == "file"    ) mp = new PathVal( this, comp, prop, true );
                else if( type == "textEdit") mp = new TextVal( this, comp, prop );
                else if( type == "enum"    ) mp = new EnumVal( this, comp, prop );
                else if( type == "bool"    ) mp = new BoolVal( this, comp, prop );

                if( !mp ) continue;

                mp->setup( isComp );
                m_propList.append( mp );
                groupWidget->layout()->addWidget( mp );

                bool propEnabled = true;
                if( isMaincomp && (prop->flags() & propNoCopy) ) propEnabled = false;

                mp->setEnabled( groupEnabled && propEnabled );
            }
            tabList->addTab( groupWidget, group.name );
    }   }
    if( tabList->count() == 0 ) tabList->setVisible( false ); // Hide tab widget if empty
}

PropVal* PropDialog::getPropWidget( QString propName )
{
    for( PropVal* widget : m_propList )
        if( widget->propName() == propName ) return widget;

    return nullptr;
}

void PropDialog::showProp( QString name, bool show )
{
    for( PropVal* prop : m_propList )
    {
        if( prop->propName() != name ) continue;
        prop->setHidden( !show );
        if( show ) this->adjustSize();
        break;
    }
}

void PropDialog::enableProp( QString name, bool en )
{
    for( PropVal* prop : m_propList )
    {
        if( prop->propName() != name ) continue;
        prop->setEnabled( en );
        break;
    }
}

void PropDialog::on_labelBox_editingFinished()
{
    m_component->setPropStr("label", labelBox->text() );
}

void PropDialog::on_showButton_toggled( bool checked )
{
    QString show = checked ? "true" : "false";
    m_component->setPropStr("Show_id", show );
}

void PropDialog::on_tabList_currentChanged( int )
{
    if( !m_component ) return;
    updtValues();
    adjustWidgets();
}

void PropDialog::on_helpButton_clicked()
{
    m_helpExpanded = !m_helpExpanded;
    if( m_helpExpanded ) mainLayout->addWidget( helpText );
    else                 mainLayout->removeWidget( helpText );
    helpText->setVisible( m_helpExpanded );

    adjustWidgets();
}

void PropDialog::adjustWidgets()
{
    int h = 0;
    int w = m_minW;
    if( helpText->isVisible() ) w += helpText->width()+6;

    QWidget* widget = tabList->currentWidget();
    if( widget )
    {
        int count = 0;
        for( int i=0; i<widget->layout()->count(); ++i )
            if( !widget->layout()->itemAt(i)->widget()->isHidden() )
                count++;

        h = count*24*m_scale+90*m_scale;
    }
    if( h < m_minH ) h = m_minH;

    this->setFixedHeight( h );
    this->setMaximumWidth( w+100*m_scale );
    this->adjustSize();
    this->setMaximumHeight( h+70*m_scale );
}

void PropDialog::updtValues()
{
    for( PropVal* prop : m_propList ) prop->updtValues();
    m_component->setup();
}

void PropDialog::changed()
{
    Circuit::self()->setChanged();
}
