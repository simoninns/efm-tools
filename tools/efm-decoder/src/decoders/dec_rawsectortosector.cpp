/************************************************************************

    dec_rawsectortosector.cpp

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

#include "dec_rawsectortosector.h"

RawSectorToSector::RawSectorToSector()
{}

void RawSectorToSector::pushSector(const RawSector &rawSector)
{
    // Add the data to the input buffer
    m_inputBuffer.enqueue(rawSector);

    // Process the queue
    processQueue();
}

Sector RawSectorToSector::popSector()
{
    // Return the first item in the output buffer
    return m_outputBuffer.dequeue();
}

bool RawSectorToSector::isReady() const
{
    // Return true if the output buffer is not empty
    return !m_outputBuffer.isEmpty();
}

void RawSectorToSector::processQueue()
{
    while (!m_inputBuffer.isEmpty()) {
        // Get the first item in the input buffer
        RawSector rawSector = m_inputBuffer.dequeue();

        // Can we trust the address data?
        bool addressDataError = false;
        if (static_cast<quint8>(rawSector.errorData()[12]) != 0 ||
            static_cast<quint8>(rawSector.errorData()[13]) != 0 ||
            static_cast<quint8>(rawSector.errorData()[14]) != 0) {
            addressDataError = true;
        }

        // Can we trust the mode data?
        bool modeDataError = false;
        if (static_cast<quint8>(rawSector.errorData()[15]) != 0) {
            modeDataError = true;
        }

        // Extract the header data
        qint32 min = bcdToInt(rawSector.data()[12]);
        qint32 sec = bcdToInt(rawSector.data()[13]);
        qint32 frame = bcdToInt(rawSector.data()[14]);

        SectorAddress address(min, sec, frame);

        qint32 mode = 0;
        if (static_cast<quint8>(rawSector.data()[15]) == 0) mode = 0;
        else if (static_cast<quint8>(rawSector.data()[15]) == 1) mode = 1;
        else if (static_cast<quint8>(rawSector.data()[15]) == 2) mode = 2;
        else mode = -1;

        // Create a new sector
        Sector sector;
        if (addressDataError || modeDataError) sector.metadataValid(false);
        else sector.metadataValid(true);
        sector.setAddress(address);
        sector.setMode(mode);

        if (m_showDebug && (addressDataError || modeDataError)) qDebug().noquote() << "RawSectorToSector::processQueue(): Metadata error... Address:" << sector.address().toString() << "Mode:" << sector.mode();

        // // Add the sector to the output buffer
        // m_outputBuffer.enqueue(sector);
    }
}

quint8 RawSectorToSector::bcdToInt(quint8 bcd)
{
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

void RawSectorToSector::showStatistics()
{
    qInfo() << "Raw sector to sector statistics:";
}