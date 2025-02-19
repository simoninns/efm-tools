/************************************************************************

    dec_data24torawsector.cpp

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

#include "dec_data24torawsector.h"

Data24ToRawSector::Data24ToRawSector()
    : m_validSectorCount(0),
      m_discardedBytes(0),
      m_missedSyncPatternCount(0),
      m_goodSyncPatternCount(0),
      m_syncLostCount(0),
      m_badSyncPatternCount(0),
      m_currentState(WaitingForSync)
{}

void Data24ToRawSector::pushSection(const Data24Section &data24Section)
{
    // Add the data to the input buffer
    m_inputBuffer.enqueue(data24Section);

    // Process the state machine
    processStateMachine();
}

RawSector Data24ToRawSector::popSector()
{
    // Return the first item in the output buffer
    return m_outputBuffer.dequeue();
}

bool Data24ToRawSector::isReady() const
{
    // Return true if the output buffer is not empty
    return !m_outputBuffer.isEmpty();
}

void Data24ToRawSector::processStateMachine()
{
    while (!m_inputBuffer.isEmpty()) {
        Data24Section data24Section = m_inputBuffer.dequeue();

        // Add the data24 section's data to the sector data buffer
        m_sectorData.reserve(m_sectorData.size() + 2352);
        for (int i = 0; i < 98; i++) {
            const QVector<quint8>& frameData = data24Section.frame(i).data();
            QByteArray frameBytes = QByteArray(reinterpret_cast<const char*>(frameData.constData()), frameData.size());
            m_sectorData.append(frameBytes);
        }

        switch (m_currentState) {
            case WaitingForSync:
                m_currentState = waitingForSync();
                break;
            case InSync:
                m_currentState = inSync();
                break;
            case LostSync:
                m_currentState = lostSync();
                break;
        }
    }
}

Data24ToRawSector::State Data24ToRawSector::waitingForSync()
{
    State nextState = WaitingForSync;

    // Does the sector data contain the sync pattern?
    quint32 syncPatternPosition = m_sectorData.indexOf(m_syncPattern);
    if (syncPatternPosition == -1) {
        // No sync pattern found
        qDebug() << "Data24ToRawSector::waitingForSync(): No sync pattern found in sectorData, discarding" << m_sectorData.size() - 11 << "bytes";

        // Clear the sector data buffer (except the last 11 bytes)
        m_discardedBytes += m_sectorData.size() - 11;
        m_sectorData = m_sectorData.right(11);

        // Get more data and try again
        nextState = WaitingForSync;
    } else {
        // Sync pattern found

        // Discard any data before the sync pattern
        m_discardedBytes += syncPatternPosition;
        m_sectorData = m_sectorData.right(m_sectorData.size() - syncPatternPosition);
        qDebug() << "Data24ToRawSector::waitingForSync(): Sync pattern found in sectorData at position:" << syncPatternPosition << "discarding" << syncPatternPosition << "bytes";

        nextState = InSync;
    }

    return nextState;
}

Data24ToRawSector::State Data24ToRawSector::inSync()
{
    State nextState = InSync;

    // Is there enough data in the buffer to form a sector?
    if (m_sectorData.size() < 2352) {
        // Not enough data
        qDebug() << "Data24ToRawSector::inSync(): Not enough data in sectorData to form a sector, waiting for more data";

        // Get more data and try again
        nextState = InSync;
        return nextState;
    } else {
        // Is there a valid sync pattern at the beginning of the sector data?
        if (m_sectorData.left(12) != m_syncPattern) {
            // No sync pattern found
            m_missedSyncPatternCount++;
            m_badSyncPatternCount++;

            if (m_missedSyncPatternCount > 4) {
                // Too many missed sync patterns, lost sync
                qDebug() << "Data24ToRawSector::inSync(): Too many missed sync patterns (4 missed), lost sync. Valid sector count:" << m_validSectorCount;
                nextState = LostSync;
                return nextState;
            } else {
                qDebug() << "Data24ToRawSector::inSync(): In sync, but the next sync pattern is missing - assuming the sync is corrupted.  Valid sector count:" << m_validSectorCount;
            }
        } else {
            // Sync pattern found
            m_goodSyncPatternCount++;
            m_missedSyncPatternCount = 0;
        }

        // Create a new sector
        RawSector rawSector;
        rawSector.pushData(m_sectorData.left(2352));
        m_outputBuffer.enqueue(rawSector);
        m_validSectorCount++;

        // Clear the sector data buffer
        m_sectorData = m_sectorData.right(m_sectorData.size() - 2352);
    }

    return nextState;
}

Data24ToRawSector::State Data24ToRawSector::lostSync()
{
    State nextState = WaitingForSync;
    m_missedSyncPatternCount = 0;
    qDebug() << "Data24ToRawSector::lostSync(): Lost sync";
    m_syncLostCount++;
    return nextState;
}

void Data24ToRawSector::showStatistics()
{
    qInfo() << "Data24ToRawSector statistics:";
    qInfo() << "  Valid sectors:" << m_validSectorCount;
    qInfo() << "  Discarded bytes:" << m_discardedBytes;
    
    qInfo() << "  Good sync patterns:" << m_goodSyncPatternCount;
    qInfo() << "  Bad sync patterns:" << m_badSyncPatternCount;

    qInfo() << "  Missed sync patterns:" << m_missedSyncPatternCount;
    qInfo() << "  Sync lost count:" << m_syncLostCount;
}