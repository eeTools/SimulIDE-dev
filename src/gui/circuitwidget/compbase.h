/***************************************************************************
 *   Copyright (C) 2021 by santiago González                               *
 *   santigoro@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#ifndef COMPBASE_H
#define COMPBASE_H

#include <QObject>
#include <QString>
#include <QPointF>
#include <QHash>

class ComProperty;

struct propGroup{
        QString name;
        QList<ComProperty*> propList;
};

class PropDialog;
class QDomElement;

class MAINMODULE_EXPORT CompBase : public QObject
{
    public:
        CompBase( QObject* parent, QString type, QString id );
        ~CompBase();

        void addPropGroup( propGroup pg );
        void remPropGroup( QString name );
        QList<propGroup> propeties() { return m_properties; }

        virtual QStringList getEnums( QString name ) { QStringList nul;return nul;}

        virtual bool    setPropStr( QString prop, QString val );
        virtual QString getPropStr( QString prop );

        virtual QString toString();


        QString getUid() { return m_id; }
        void setUid( QString uid ) { m_id = uid; }

        QString itemType()  { return m_type; }
        void setItemType( QString ) {;}

        virtual void setValLabelText( QString t ){;}

        virtual QString showProp(){return "";}
        virtual void setShowProp( QString prop ){;}

        static bool m_saveBoard;

    protected:
        QString m_id;
        QString m_type;

        bool m_isMainComp;

        PropDialog* m_propDialog;
        QList<propGroup> m_properties;
        QHash<QString, ComProperty*> m_propHash;
};

#endif