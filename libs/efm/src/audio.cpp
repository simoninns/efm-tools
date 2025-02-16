/************************************************************************

    audio.h

    EFM-library - Audio frame type class
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

#include "audio.h"

// Set the data for the audio frame, ensuring it matches the frame size
void Audio::setData(const QVector<qint16> &data)
{
    if (data.size() != 12) {
        qFatal("Audio::setData(): Data size of %d does not match frame size of %d", data.size(), 12);
    }
    m_audioData = data;
}

// Get the data for the audio frame, returning a zero-filled vector if empty
QVector<qint16> Audio::data() const
{
    if (m_audioData.isEmpty()) {
        qDebug() << "Audio::data(): Frame is empty, returning zero-filled vector";
        return QVector<qint16>(12, 0);
    }
    return m_audioData;
}

// Set the error data for the audio frame, ensuring it matches the frame size
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
void Audio::setErrorData(const QVector<qint16> &errorData)
{
    if (errorData.size() != 12) {
        qFatal("Audio::setErrorData(): Error data size of %d does not match frame size of %d", errorData.size(), 12);
    }
    m_audioErrorData = errorData;
}

// Get the error_data for the audio frame, returning a zero-filled vector if empty
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
QVector<qint16> Audio::errorData() const
{
    if (m_audioErrorData.isEmpty()) {
        qDebug() << "Audio::errorData(): Error frame is empty, returning zero-filled vector";
        return QVector<qint16>(12, 0);
    }
    return m_audioErrorData;
}

// Count the number of errors in the audio frame
quint32 Audio::countErrors() const
{
    quint32 errorCount = 0;
    for (int i = 0; i < 12; ++i) {
        if (m_audioErrorData[i] == 1) {
            errorCount++;
        }
    }
    return errorCount;
}

// Check if the audio frame is full (i.e., has data)
bool Audio::isFull() const
{
    return !m_audioData.isEmpty();
}

// Check if the audio frame is empty (i.e., has no data)
bool Audio::isEmpty() const
{
    return m_audioData.isEmpty();
}

// Show the frame data and errors in debug
void Audio::showData()
{
    QString dataString;
    bool hasError = false;
    for (int i = 0; i < m_audioData.size(); ++i) {
        if (m_audioErrorData[i] == 0) {
            dataString.append(QString("%1 ").arg(m_audioData[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            hasError = true;
        }
    }
    if (hasError) {
        qInfo().noquote() << "Audio:" << dataString.trimmed() << "ERROR";
    } else {
        qInfo().noquote() << "Audio:" << dataString.trimmed();
    }
}

int Audio::frameSize() const
{
    return 12;
}