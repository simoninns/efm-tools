/************************************************************************

    dec_rawsectortosector.h

    ld-efm-decoder - EFM data decoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-decoder is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

************************************************************************/

#ifndef DEC_RAWSECTORTOSECTOR_H
#define DEC_RAWSECTORTOSECTOR_H

#include "decoders.h"
#include "sector.h"

class RawSectorToSector : public Decoder
{
public:
    RawSectorToSector();
    void pushSector(const RawSector &rawSector);
    Sector popSector();
    bool isReady() const;

    void showStatistics();

private:
    void processQueue();
    quint8 bcdToInt(quint8 bcd);

    QQueue<RawSector> m_inputBuffer;
    QQueue<Sector> m_outputBuffer;

    bool m_have_last_known_good;
    SectorAddress m_last_known_good_address;
    quint8 m_last_known_good_mode;

    // Statistics
    quint32 m_validSectorAddresses;
    quint32 m_invalidSectorAddresses;
    quint32 m_validSectorModes;
    quint32 m_invalidSectorModes;
};

#endif // DEC_RAWSECTORTOSECTOR_H