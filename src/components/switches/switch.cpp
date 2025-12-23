/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QGraphicsProxyWidget>
#include <QRegularExpression>

#include "switch.h"
#include "itemlibrary.h"
#include "simulator.h"
#include "e-node.h"
#include "pin.h"
#include "custombutton.h"

#include "stringprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("Switch",str)

Component* Switch::construct( QString type, QString id )
{ return new Switch( type, id ); }

LibraryItem* Switch::libraryItem()
{
    return new LibraryItem(
        tr("Switch (all)"),
        "Switches",
        "switch.png",
        "Switch",
        Switch::construct);
}

Switch::Switch( QString type, QString id )
      : SwitchBase( type, id )
{
    m_area = QRectF(-11,-9, 22, 11 );
    m_proxy->setPos(-8, 4 );

    SetupSwitches( 1, 1 );

    QObject::connect( m_button, &CustomButton::clicked, [=](){ onbuttonclicked(); });

    addPropGroup( { tr("Main"), {
        new BoolProp<Switch>("Norm_Close", tr("Normally Closed"), ""
                            , this, &Switch::nClose, &Switch::setNClose ),

        new BoolProp<Switch>("DT", tr("Double Throw"), ""
                            , this, &Switch::dt, &Switch::setDt, propNoCopy ),

        new IntProp <Switch>("Poles", tr("Poles"), ""
                            , this, &Switch::poles, &Switch::setPoles, propNoCopy,"uint" ),

        new StrProp <Switch>("Key", tr("Key"), ""
                            , this, &Switch::key, &Switch::setKey ),

        new StrProp <Switch>("LinkCondition", tr("Link Condition"), ""
                            , this, &Switch::linkCondition, &Switch::setLinkCondition ),

        new BoolProp<Switch>("Checked", "", ""
                            , this, &Switch::checked, &Switch::setChecked, propHidden ),
    },0} );
}
Switch::~Switch(){}

void Switch::stamp()
{
    MechContact::stamp();
    onbuttonclicked();
}

void Switch::setLinkedValue( double v, int )
{
    bool checked = Switch::checked();

    if (m_condition.isNull() or m_condition.isEmpty())
        return;
    bool new_state = Switch::evaluateCompoundCondition(m_condition, (int)v);
    if (checked != new_state) {
        Switch::setChecked(new_state);
    }
}

void Switch::keyEvent( QString key, bool pressed )
{
    if( key == m_key )
    {
        if( !pressed )
        {
            Switch::setChecked(!m_button->isChecked());
}   }   }


bool evaluateSingleCondition(const QString& condition, int value) {
    QRegularExpression regex(R"(([><=!]=?|==)\s*(-?\d+))");
    QRegularExpressionMatch match = regex.match(condition);

    if (!match.hasMatch()) {
        return false;
    }

    QString op = match.captured(1);
    int threshold = match.captured(2).toInt();

    if (op == ">") return value > threshold;
    if (op == ">=") return value >= threshold;
    if (op == "<") return value < threshold;
    if (op == "<=") return value <= threshold;
    if (op == "==") return value == threshold;
    if (op == "=") return value == threshold;
    if (op == "!=") return value != threshold;

    return false;
}

QStringList tokenize(const QString& condition) {
    QString cleanedCondition = condition;
    cleanedCondition.replace(QRegularExpression("\\s+"), "");
    cleanedCondition = cleanedCondition.trimmed();
    QString regexPattern = R"((\(|\)|and|or|[><=!]=?[+-]?\d+))";
    QRegularExpression regex(regexPattern);

    QStringList tokens;
    QRegularExpressionMatchIterator i = regex.globalMatch(cleanedCondition);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        tokens << match.captured(1);
    }

    return tokens;
}

bool evaluateTokens(const QStringList& tokens, int& index, int value) {
    bool result = false;
    QString currentOperator = "or";

    while (index < tokens.size()) {
        QString token = tokens[index++];

        if (token == "(") {
            bool subResult = evaluateTokens(tokens, index, value);
            if (currentOperator == "and") {
                result = result && subResult;
            } else if (currentOperator == "or") {
                result = result || subResult;
            }
        } else if (token == ")") {
            break;
        } else if (token == "and" || token == "or") {
            currentOperator = token;
        } else {
            bool conditionResult = evaluateSingleCondition(token, value);
            if (currentOperator == "and") {
                result = result && conditionResult;
            } else if (currentOperator == "or") {
                result = result || conditionResult;
            }
        }
    }

    return result;
}

bool Switch::evaluateCompoundCondition(const QString& rawCondition, int value) {
    QStringList tokens = tokenize(rawCondition);  // Tokenize the condition into parts
    int index = 0;
    return evaluateTokens(tokens, index, value);
}
bool Switch::checked()
{
    return m_button->isChecked();
}

void Switch::setChecked( bool c )
{
    m_button->setChecked( c );
    SwitchBase::onbuttonclicked();
}

void Switch::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    if( m_hidden ) return;

    Component::paint( p, o, w );
    MechContact::paint( p, o, w );
}
