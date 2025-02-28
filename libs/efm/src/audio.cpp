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

// Channel class
// Set the data for the channel, ensuring it matches the frame size
void Channel::setData(const QVector<qint16> &data)
{
    if (data.size() != frameSize()) {
        qFatal("Channel::setData(): Data size of %d does not match frame size of %d", data.size(), frameSize());
    }
    m_audioData = data;
}

// Get the data for the channel, returning a zero-filled vector if empty
QVector<qint16> Channel::data() const
{
    if (m_audioData.isEmpty()) {
        qDebug() << "Channel::data(): Frame is empty, returning zero-filled vector";
        return QVector<qint16>(frameSize(), 0);
    }
    return m_audioData;
}

// Set the error data for the channel, ensuring it matches the frame size
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
void Channel::setErrorData(const QVector<qint16> &errorData)
{
    if (errorData.size() != frameSize()) {
        qFatal("Channel::setErrorData(): Error data size of %d does not match frame size of %d", errorData.size(), frameSize());
    }
    m_audioErrorData = errorData;
}

// Get the error_data for the channel, returning a zero-filled vector if empty
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
QVector<qint16> Channel::errorData() const
{
    if (m_audioErrorData.isEmpty()) {
        qDebug() << "Channel::errorData(): Error frame is empty, returning zero-filled vector";
        return QVector<qint16>(frameSize(), 0);
    }
    return m_audioErrorData;
}

// Count the number of errors in the channel
quint32 Channel::countErrors() const
{
    quint32 errorCount = 0;
    for (int i = 0; i < frameSize(); ++i) {
        if (m_audioErrorData[i] == 1) {
            errorCount++;
        }
    }
    return errorCount;
}

// Check if the channel is full (i.e., has data)
bool Channel::isFull() const
{
    return !isEmpty();
}

// Check if the channel is empty (i.e., has no data)
bool Channel::isEmpty() const
{
    return m_audioData.isEmpty();
}

// Show the frame data and errors in debug
void Channel::showData(QString channelName)
{
    QString dataString;
    bool hasError = false;
    for (int i = 0; i < m_audioData.size(); ++i) {
        if (m_audioErrorData[i] == 0) {
            dataString.append(QString("%1%2 ")
                .arg(m_audioData[i] < 0 ? "-" : "+")
                .arg(qAbs(m_audioData[i]), 4, 16, QChar('0')));
        } else {
            dataString.append(QString("XXXXX "));
            hasError = true;
        }
    }

    qDebug().noquote() << channelName << dataString.trimmed().toUpper();
}

int Channel::frameSize() const
{
    return 6;
}

// Audio class
// Set the data for the audio frame, ensuring it matches the frame size
void Audio::setData(const QVector<qint16> &data)
{
    if (data.size() != frameSize()) {
        qFatal("Audio::setData(): Data size of %d does not match frame size of %d", data.size(), frameSize());
    }
    
    QVector<qint16> leftChannelData;
    QVector<qint16> rightChannelData;
    for (int i = 0; i < frameSize(); i+=2) {
        leftChannelData.append(data[i]);
        rightChannelData.append(data[i+1]);
    }

    leftChannel.setData(leftChannelData);
    rightChannel.setData(rightChannelData);
}

// Get the data for the audio frame, returning a zero-filled vector if empty
QVector<qint16> Audio::data() const
{
    if (leftChannel.isEmpty() || rightChannel.isEmpty()) {
        qDebug() << "Audio::data(): Channels are empty, returning zero-filled vector";
        return QVector<qint16>(frameSize(), 0);
    }
    
    QVector<qint16> audioData;
    for (int i = 0; i < leftChannel.frameSize(); ++i) {
        audioData.append(leftChannel.data()[i]);
        audioData.append(rightChannel.data()[i]);
    }

    return audioData;
}

// Set the error data for the audio frame, ensuring it matches the frame size
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
void Audio::setErrorData(const QVector<qint16> &errorData)
{
    if (errorData.size() != frameSize()) {
        qFatal("Audio::setErrorData(): Error data size of %d does not match frame size of %d", errorData.size(), 12);
    }
    
    QVector<qint16> leftChannelErrorData;
    QVector<qint16> rightChannelErrorData;
    for (int i = 0; i < frameSize(); i+=2) {
        leftChannelErrorData.append(errorData[i]);
        rightChannelErrorData.append(errorData[i+1]);
    }

    leftChannel.setErrorData(leftChannelErrorData);
    rightChannel.setErrorData(rightChannelErrorData);
}

// Get the error_data for the audio frame, returning a zero-filled vector if empty
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
QVector<qint16> Audio::errorData() const
{
    if (leftChannel.isEmpty() || rightChannel.isEmpty()) {
        qDebug() << "Audio::errorData(): Channels are empty, returning zero-filled vector";
        return QVector<qint16>(frameSize(), 0);
    }
    
    QVector<qint16> audioErrorData;
    for (int i = 0; i < leftChannel.frameSize(); ++i) {
        audioErrorData.append(leftChannel.errorData()[i]);
        audioErrorData.append(rightChannel.errorData()[i]);
    }

    return audioErrorData;
}

// Count the number of errors in the audio frame
quint32 Audio::countErrors() const
{
    quint32 errorCount = 0;
    for (int i = 0; i < leftChannel.frameSize(); ++i) {
        if (leftChannel.errorData()[i] == 1) errorCount++;
        if (rightChannel.errorData()[i] == 1) errorCount++;
    }
    return errorCount;
}

// Check if the audio frame is full (i.e., has data)
bool Audio::isFull() const
{
    return !isEmpty();
}

// Check if the audio frame is empty (i.e., has no data)
bool Audio::isEmpty() const
{
    return (leftChannel.isEmpty() || rightChannel.isEmpty());
}

// Show the frame data and errors in debug
void Audio::showData()
{
    leftChannel.showData(" Left channel:");
    rightChannel.showData("Right channel:");
}

int Audio::frameSize() const
{
    return 12;
}