/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "comproperty.h"
#include "proputils.h"

template <class Comp>
class PointProp : public ComProperty
{
    public:
        PointProp( QString name, QString caption, QString unit, Comp* comp
                 , QPointF (Comp::*getter)(), void (Comp::*setter)(QPointF)
                 , QString type="point", uint8_t flags=0 )
        : ComProperty( name, caption, unit, type, flags )
        {
            m_comp = comp;
            m_getter = getter;
            m_setter = setter;
        }
        ~PointProp(){;}

        void setValStr( QString val ) override
        { (m_comp->*m_setter)( getPointF( val ) ); }

        QString getValStr() override
        { return getStrPointF( (m_comp->*m_getter)() ); }

    private:
        Comp* m_comp;
        QPointF (Comp::*m_getter)();
        void    (Comp::*m_setter)(QPointF);
};
