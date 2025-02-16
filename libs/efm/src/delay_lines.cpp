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

#include <QVector>
#include <QQueue>
#include <QtGlobal>
#include <QDebug>
#include <iostream>

#include "delay_lines.h"

DelayLines::DelayLines(QVector<quint32> delayLengths)
{
    for (qint32 i = 0; i < delayLengths.size(); ++i) {
        m_delayLines.append(DelayLine(delayLengths[i]));
    }
}

QVector<quint8> DelayLines::push(QVector<quint8> inputData)
{
    if (inputData.size() != m_delayLines.size()) {
        qFatal("Input data size does not match the number of delay lines.");
    }

    QVector<quint8> outputData;
    for (qint32 i = 0; i < m_delayLines.size(); ++i) {
        outputData.append(m_delayLines[i].push(inputData[i]));
    }

    // Check if the delay lines are ready and, if not, return an empty vector
    if (!isReady()) {
        return QVector<quint8>{};
    }

    return outputData;
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
DelayLine::DelayLine(qint32 delayLength)
{
    m_buffer = new quint8[delayLength];

    m_delayLength = delayLength;
    flush();
}

quint8 DelayLine::push(quint8 inputDatum)
{
    if (m_delayLength == 0) {
        return inputDatum;
    }

    // Make sure the buffer is valid before accessing
    quint8 outputDatum = m_buffer[0];

    std::copy(m_buffer + 1, m_buffer + m_delayLength, m_buffer);
    m_buffer[m_delayLength - 1] = inputDatum;

    // Check if the delay line is ready
    if (m_pushCount >= m_delayLength) {
        m_ready = true;
    } else {
        ++m_pushCount;
    }

    return outputDatum;
}

bool DelayLine::isReady()
{
    return m_ready;
}

void DelayLine::flush()
{
    if (m_delayLength > 0) {
        std::fill(m_buffer, m_buffer + m_delayLength, 0);
        m_ready = false;
    } else {
        m_ready = true;
    }
    m_pushCount = 0;
}