/************************************************************************

    audio.cpp

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

// Audio class
// Set the data for the audio, ensuring it matches the frame size
void Audio::setData(const QVector<qint16> &data)
{
    if (data.size() != frameSize()) {
        qFatal("Audio::setData(): Data size of %d does not match frame size of %d", data.size(), frameSize());
    }
    m_audioData = data;
}

// Get the data for the audio, returning a zero-filled vector if empty
QVector<qint16> Audio::data() const
{
    if (m_audioData.isEmpty()) {
        qDebug() << "Audio::data(): Frame is empty, returning zero-filled vector";
        return QVector<qint16>(frameSize(), 0);
    }
    return m_audioData;
}

// Set the error data for the audio, ensuring it matches the frame size
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
void Audio::setErrorData(const QVector<bool> &errorData)
{
    if (errorData.size() != frameSize()) {
        qFatal("Audio::setErrorData(): Error data size of %d does not match frame size of %d", errorData.size(), frameSize());
    }
    m_audioErrorData = errorData;
}

// Get the error_data for the audio, returning a zero-filled vector if empty
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
QVector<bool> Audio::errorData() const
{
    if (m_audioErrorData.isEmpty()) {
        qDebug() << "Audio::errorData(): Error frame is empty, returning zero-filled vector";
        return QVector<bool>(frameSize(), false);
    }
    return m_audioErrorData;
}

// Count the number of errors in the audio
quint32 Audio::countErrors() const
{
    quint32 errorCount = 0;
    for (int i = 0; i < frameSize(); ++i) {
        if (m_audioErrorData[i] == true) {
            errorCount++;
        }
    }
    return errorCount;
}

// Check if the audio is full (i.e., has data)
bool Audio::isFull() const
{
    return !isEmpty();
}

// Check if the audio is empty (i.e., has no data)
bool Audio::isEmpty() const
{
    return m_audioData.isEmpty();
}

// Show the audio data and errors in debug
void Audio::showData()
{
    QString dataString;
    bool hasError = false;
    for (int i = 0; i < m_audioData.size(); ++i) {
        if (m_audioErrorData[i] == false) {
            dataString.append(QString("%1%2 ")
                .arg(m_audioData[i] < 0 ? "-" : "+")
                .arg(qAbs(m_audioData[i]), 4, 16, QChar('0')));
        } else {
            dataString.append(QString("XXXXX "));
            hasError = true;
        }
    }

    qDebug().noquote() << dataString.trimmed().toUpper();
}

int Audio::frameSize() const
{
    return 12;
}