/***************************************************************************
 *   Copyright (C) 2021 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "mcuicunit.h"

class PicIcUnit : public McuIcUnit
{
    friend class PicCcpUnit;

    public:
        PicIcUnit( eMcu* mcu, QString name );
        virtual ~PicIcUnit();

        virtual void initialize() override;
        virtual void voltChanged() override;

        virtual void configure( uint8_t CCPxM ) override;

    private:
        uint64_t m_prescaler;
        uint64_t m_counter;
};
