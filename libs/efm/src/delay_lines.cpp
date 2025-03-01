/************************************************************************

    delay_lines.cpp

    EFM-library - Delay line functions
    Copyright (C) 2025 Simon Inns

    This file is part of EFM-Tools.

    This is free software: you can redistribute it and/or
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

#include "delay_lines.h"

DelayLines::DelayLines(QVector<qint32> delayLengths)
{
    m_delayLines.reserve(delayLengths.size());
    for (qint32 i = 0; i < delayLengths.size(); ++i) {
        m_delayLines.append(DelayLine(delayLengths[i]));
    }
}

void DelayLines::push(QVector<quint8>& data, QVector<bool>& errorData, QVector<bool>& paddedData)
{
    if (data.size() != m_delayLines.size()) {
        qFatal("Input data size does not match the number of delay lines.");
    }

    // Process each input value through its corresponding delay line
    for (qint32 i = 0; i < m_delayLines.size(); ++i) {
        m_delayLines[i].push(data[i], errorData[i], paddedData[i]);
    }

    // Clear the vector if delay lines aren't ready (in order to
    // return empty data vectors)
    if (!isReady()) {
        data.clear();
        errorData.clear();
        paddedData.clear();
    }
}

bool DelayLines::isReady()
{
    for (qint32 i = 0; i < m_delayLines.size(); ++i) {
        if (!m_delayLines[i].isReady()) {
            return false;
        }
    }
    return true;
}

void DelayLines::flush()
{
    for (qint32 i = 0; i < m_delayLines.size(); ++i) {
        m_delayLines[i].flush();
    }
}

// DelayLine class implementation
DelayLine::DelayLine(qint32 delayLength) :
    m_pushCount(0),
    m_ready(false)
{
    m_buffer.resize(delayLength);
    m_delayLength = delayLength;

    flush();
}

void DelayLine::push(quint8& datum, bool& datumError, bool& datumPadded)
{
    if (m_delayLength == 0) {
        return;
    }

    // Store the input value temporarily
    quint8 tempInput = datum;
    bool tempInputError = datumError;
    bool tempInputPadded = datumPadded;
    
    // Return output through the reference parameter
    datum = m_buffer[0].datum;
    datumError = m_buffer[0].error;
    datumPadded = m_buffer[0].padded;

    // Shift values in the buffers
    for (qint32 i = 0; i < m_delayLength - 1; ++i) {
        m_buffer[i] = m_buffer[i + 1];
    }

    // Store new values at the end
    m_buffer[m_delayLength - 1].datum = tempInput;
    m_buffer[m_delayLength - 1].error = tempInputError;
    m_buffer[m_delayLength - 1].padded = tempInputPadded;

    // Check if the delay line is ready
    if (m_pushCount >= m_delayLength) {
        m_ready = true;
    } else {
        ++m_pushCount;
    }
}

bool DelayLine::isReady()
{
    return m_ready;
}

void DelayLine::flush()
{
    if (m_delayLength > 0) {
        for (int i = 0; i < m_delayLength; ++i) {
            m_buffer[i].datum = 0;
            m_buffer[i].error = false;
            m_buffer[i].padded = false;
        }
        m_ready = false;
    } else {
        m_ready = true;
    }
    m_pushCount = 0;
}