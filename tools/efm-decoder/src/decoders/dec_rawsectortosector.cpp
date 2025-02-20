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
    : m_have_last_known_good(false),
    m_last_known_good_address(0, 0, 0),
    m_last_known_good_mode(0),
    m_validSectorAddresses(0),
    m_invalidSectorAddresses(0),
    m_validSectorModes(0),
    m_invalidSectorModes(0),
    m_validSectors(0),
    m_invalidSectors(0),
    m_correctedSectors(0)
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

        // Verify the sector data size
        if (rawSector.data().size() != 2352) {
            if (m_showDebug) {
                qDebug() << "RawSectorToSector::processQueue(): Sector data size is incorrect. Expected 2352 bytes, got" << rawSector.data().size() << "bytes";
                qFatal("RawSectorToSector::processQueue(): Sector data size is incorrect");
            }
        }

        // Note: The EDC is a CRC applied to bytes 0 to 2063 (sync to user data)
        // The intermediate field is 8 bytes of zero
        // The P and Q parity is applied to bytes 12 to 2075 (not sync and not parity bytes)

        // So we check the EDC and if it's not correct we error correct the sector

        // Perform CRC - since ECC is expensive on processing, we only
        // error correct sector data if the CRC fails

        // Get the 32-bit EDC word from the F1 data
        quint32 originalEdcWord =
            ((static_cast<quint32>(static_cast<uchar>(rawSector.data()[2064]))) <<  0) |
            ((static_cast<quint32>(static_cast<uchar>(rawSector.data()[2065]))) <<  8) |
            ((static_cast<quint32>(static_cast<uchar>(rawSector.data()[2066]))) << 16) |
            ((static_cast<quint32>(static_cast<uchar>(rawSector.data()[2067]))) << 24);

        quint32 edcWord = crc32(rawSector.data(), 2064);

        // Perform a CRC32 on bytes 0 to 2063 of the F1 frame
        if (originalEdcWord != edcWord) {
            if (m_showDebug) {
                qDebug() << "RawSectorToSector::processQueue(): CRC32 error - sector data is corrupt. EDC:" << originalEdcWord << "Calculated:" << edcWord << "attempting to correct";
            }

            // Attempt Q and P parity error correction on the sector data
            Rspc rspc;

            // Make a local copy of the sector data
            QByteArray correctedData = rawSector.data();
            QByteArray correctedErrorData = rawSector.errorData();

            rspc.qParityEcc(correctedData, correctedErrorData, m_showDebug);
            rspc.pParityEcc(correctedData, correctedErrorData, m_showDebug);

            // Copy the corrected data back to the raw sector
            rawSector.pushData(correctedData);
            rawSector.pushErrorData(correctedErrorData);

            // Check CRC again for the corrected data
            quint32 correctedEdcWord =
                ((static_cast<quint32>(static_cast<uchar>(rawSector.data()[2064]))) <<  0) |
                ((static_cast<quint32>(static_cast<uchar>(rawSector.data()[2065]))) <<  8) |
                ((static_cast<quint32>(static_cast<uchar>(rawSector.data()[2066]))) << 16) |
                ((static_cast<quint32>(static_cast<uchar>(rawSector.data()[2067]))) << 24);

            edcWord = crc32(rawSector.data(), 2064);

            // Perform a CRC32 on bytes 0 to 2063 of the F1 frame
            if (correctedEdcWord != edcWord) {
                if (m_showDebug) qDebug() << "RawSectorToSector::processQueue(): CRC32 error - sector data cannot be recovered. EDC:" << correctedEdcWord << "Calculated:" << edcWord << "post correction";
                m_invalidSectors++;
            } else {
                if (m_showDebug) qDebug() << "RawSectorToSector::processQueue(): Sector data corrected. EDC:" << correctedEdcWord << "Calculated:" << edcWord << "";
                m_correctedSectors++;
            }
        } else {
            //if (m_showDebug) qDebug() << "RawSectorToSector::processQueue(): Sector data is correct.";
             m_validSectors++;
        }

        // Note: This is very simplistic metadata correction but we are
        // relying on the previous decoding stages to give us the sectors
        // in the correct order.

        // Extract the sector address data
        qint32 min = bcdToInt(rawSector.data()[12]);
        qint32 sec = bcdToInt(rawSector.data()[13]);
        qint32 frame = bcdToInt(rawSector.data()[14]);
        SectorAddress address(min, sec, frame);

        // Extract the sector mode data
        qint32 mode = 0;
        if (static_cast<quint8>(rawSector.data()[15]) == 0) mode = 0;
        else if (static_cast<quint8>(rawSector.data()[15]) == 1) mode = 1;
        else if (static_cast<quint8>(rawSector.data()[15]) == 2) mode = 2;
        else mode = -1;

        // Can we trust the address data?
        if (static_cast<quint8>(rawSector.errorData()[12]) != 0 ||
            static_cast<quint8>(rawSector.errorData()[13]) != 0 ||
            static_cast<quint8>(rawSector.errorData()[14]) != 0) {
            // Address data cannot be trusted
            if (m_have_last_known_good) {
                // Use the last known good address + 1
                SectorAddress expectedAddress = m_last_known_good_address + 1;
                if (m_showDebug) {
                    qDebug() << "RawSectorToSector::processQueue(): Address error. Got:" << address.toString() << "replacing with expected:" << expectedAddress.toString();
                }
                address = expectedAddress;
            } else {
                // Use a default address
                SectorAddress defaultAddress(0, 0, 0);
                if (m_showDebug) {
                    qDebug() << "RawSectorToSector::processQueue(): Address error. Got:" << address.toString() << "replacing with default:" << defaultAddress.toString();
                }
                address = defaultAddress;
            }
            m_invalidSectorAddresses++;
        } else {
            // Address data is good
            m_have_last_known_good = true;
            m_last_known_good_address = address;
            m_last_known_good_mode = 1; // Just in case...

            m_validSectorAddresses++;
        }

        // Can we trust the mode data?
        if (static_cast<quint8>(rawSector.errorData()[15]) != 0) {
            // Mode cannot be trusted
            if (m_have_last_known_good) {
                // Use the last known good mode
                mode = m_last_known_good_mode;
                if (m_showDebug) {
                    qDebug() << "RawSectorToSector::processQueue(): Mode error. Got:" << mode << "replacing with last known good:" << m_last_known_good_mode;
                }
            } else {
                // Use a default mode
                mode = 1;
                if (m_showDebug) {
                    qDebug() << "RawSectorToSector::processQueue(): Mode error. Got:" << mode << "replacing with default:" << 1;
                }
            }
            m_invalidSectorModes++;
        } else {
            // Mode data is good
            m_have_last_known_good = true;
            m_last_known_good_mode = mode;

            m_validSectorModes++;
        }

        // Create a new sector
        Sector sector;
        sector.metadataValid(true);
        sector.setAddress(address);
        sector.setMode(mode);

        //if (m_showDebug) qDebug().noquote() << "RawSectorToSector::processQueue(): Metadata Address:" << sector.address().toString() << "Mode:" << sector.mode();

        // // Add the sector to the output buffer
        // m_outputBuffer.enqueue(sector);
    }
}

// Convert 1 byte BCD to integer
quint8 RawSectorToSector::bcdToInt(quint8 bcd)
{
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

// CRC code adapted and used under GPLv3 from:
// https://github.com/claunia/edccchk/blob/master/edccchk.c
quint32 RawSectorToSector::crc32(const QByteArray &src, qint32 size)
{
    quint32 crc = 0;
    const uchar *data = reinterpret_cast<const uchar*>(src.constData());

    while(size--) {
        crc = (crc >> 8) ^ m_crc32Lut[(crc ^ (*data++)) & 0xFF];
    }

    return crc;
}

void RawSectorToSector::showStatistics()
{
    qInfo() << "Raw sector to sector statistics:";
    qInfo() << "  Valid sector addresses:" << m_validSectorAddresses;
    qInfo() << "  Invalid sector addresses:" << m_invalidSectorAddresses;
    qInfo() << "  Valid sector modes:" << m_validSectorModes;
    qInfo() << "  Invalid sector modes:" << m_invalidSectorModes;
    qInfo() << "Error correction statistics:";
    qInfo() << "  Valid sectors:" << m_validSectors;
    qInfo() << "  Corrected sectors:" << m_correctedSectors;
    qInfo() << "  Invalid sectors:" << m_invalidSectors;

}