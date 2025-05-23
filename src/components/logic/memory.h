/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "iocomponent.h"
#include "e-element.h"
#include "memdata.h"

class LibraryItem;

class Memory : public IoComponent, public eElement, public MemData
{
    public:
        Memory( QString type, QString id );
        ~Memory();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        void stamp() override;
        void updateStep() override;
        void voltChanged() override;
        void runEvent() override { IoComponent::runOutputs(); }

        void setMem( QString m );
        QString getMem();

        int addrBits() { return m_addrBits; }
        void setAddrBits( int bits );
        void deleteAddrBits( int bits );
        void createAddrBits( int bits );

        int dataBits() { return m_dataBits; }
        void setDataBits( int bits );
        void deleteDataBits( int bits );
        void createDataBits( int bits );

        bool persistent() { return m_persistent; }
        void setPersistent( bool p ) { m_persistent = p; }

        bool asynchro() { return m_asynchro; }
        void setAsynchro( bool a );

        void updatePins();

    public slots:
        void loadData();
        void saveData();
        void slotShowTable();

    protected:
        virtual void contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu ) override;
        
    private:
        int m_addrBits;
        int m_dataBits;
        int m_dataBytes;
        int m_address;

        QVector<int> m_ram;

        bool m_oe;
        bool m_we;
        bool m_cs;
        bool m_persistent;
        bool m_asynchro;

        IoPin* m_CsPin;
        IoPin* m_WePin;
        IoPin* m_OePin;
};
