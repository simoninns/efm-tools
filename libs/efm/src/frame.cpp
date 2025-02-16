/************************************************************************

    frame.cpp

    EFM-library - EFM Frame type classes
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

#include "frame.h"

// Frame class
// --------------------------------------------------------------------------------------------------------

// Set the data for the frame, ensuring it matches the frame size
void Frame::setData(const QVector<quint8> &data)
{
    if (data.size() != getFrameSize()) {
        qFatal("Frame::setData(): Data size of %d does not match frame size of %d", data.size(),
               getFrameSize());
    }
    frameData = data;
}

// Get the data for the frame, returning a zero-filled vector if empty
QVector<quint8> Frame::getData() const
{
    if (frameData.isEmpty()) {
        qDebug() << "Frame::getData(): Frame is empty, returning zero-filled vector";
        return QVector<quint8>(getFrameSize(), 0);
    }
    return frameData;
}

// Set the error data for the frame, ensuring it matches the frame size
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
void Frame::setErrorData(const QVector<quint8> &errorData)
{
    if (errorData.size() != getFrameSize()) {
        qFatal("Frame::setErrorData(): Error data size of %d does not match frame size of %d",
               errorData.size(), getFrameSize());
    }
    frameErrorData = errorData;
}

// Get the error_data for the frame, returning a zero-filled vector if empty
// Note: This is a vector of 0s and 1s, where 0 is no error and 1 is an error
QVector<quint8> Frame::getErrorData() const
{
    if (frameErrorData.isEmpty()) {
        qDebug() << "Frame::getErrorData(): Error frame is empty, returning zero-filled vector";
        return QVector<quint8>(getFrameSize(), 0);
    }
    return frameErrorData;
}

// Count the number of errors in the frame
quint32 Frame::countErrors() const
{
    quint32 errorCount = 0;
    for (int i = 0; i < frameErrorData.size(); ++i) {
        if (frameErrorData[i] == 1) {
            errorCount++;
        }
    }
    return errorCount;
}

// Check if the frame is full (i.e., has data)
bool Frame::isFull() const
{
    return !frameData.isEmpty();
}

// Check if the frame is empty (i.e., has no data)
bool Frame::isEmpty() const
{
    return frameData.isEmpty();
}

// Constructor for Data24, initializes data to the frame size
Data24::Data24()
{
    frameData.resize(getFrameSize());
    frameErrorData.resize(getFrameSize());
    frameErrorData.fill(0);
}

// We override the set_data function to ensure the data is 24 bytes
// since it's possible to have less than 24 bytes of data
void Data24::setData(const QVector<quint8> &data)
{
    frameData = data;

    // If there are less than 24 bytes, pad data with zeros to 24 bytes
    if (frameData.size() < 24) {
        frameData.resize(24);
        for (int i = frameData.size(); i < 24; ++i) {
            frameData[i] = 0;
        }
    }
}

void Data24::setErrorData(const QVector<quint8> &errorData)
{
    frameErrorData = errorData;

    // If there are less than 24 bytes, pad data with zeros to 24 bytes
    if (frameErrorData.size() < 24) {
        frameErrorData.resize(24);
        for (int i = frameErrorData.size(); i < 24; ++i) {
            frameErrorData[i] = 0;
        }
    }
}

// Get the frame size for Data24
int Data24::getFrameSize() const
{
    return 24;
}

void Data24::showData()
{
    QString dataString;
    bool hasError = false;
    for (int i = 0; i < frameData.size(); ++i) {
        if (frameErrorData[i] == 0) {
            dataString.append(QString("%1 ").arg(frameData[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            hasError = true;
        }
    }
    if (hasError) {
        qInfo().noquote() << "Data24:" << dataString.trimmed() << "ERROR";
    } else {
        qInfo().noquote() << "Data24:" << dataString.trimmed();
    }
}

// Constructor for F1Frame, initializes data to the frame size
F1Frame::F1Frame()
{
    frameData.resize(getFrameSize());
    frameErrorData.resize(getFrameSize());
    frameErrorData.fill(0);
}

// Get the frame size for F1Frame
int F1Frame::getFrameSize() const
{
    return 24;
}

void F1Frame::showData()
{
    QString dataString;
    bool hasError = false;
    for (int i = 0; i < frameData.size(); ++i) {
        if (frameErrorData[i] == 0) {
            dataString.append(QString("%1 ").arg(frameData[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            hasError = true;
        }
    }
    if (hasError) {
        qInfo().noquote() << "F1Frame:" << dataString.trimmed() << "ERROR";
    } else {
        qInfo().noquote() << "F1Frame:" << dataString.trimmed();
    }
}

// Constructor for F2Frame, initializes data to the frame size
F2Frame::F2Frame()
{
    frameData.resize(getFrameSize());
    frameErrorData.resize(getFrameSize());
    frameErrorData.fill(0);
}

// Get the frame size for F2Frame
int F2Frame::getFrameSize() const
{
    return 32;
}

void F2Frame::showData()
{
    QString dataString;
    bool hasError = false;
    for (int i = 0; i < frameData.size(); ++i) {
        if (frameErrorData[i] == 0) {
            dataString.append(QString("%1 ").arg(frameData[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            hasError = true;
        }
    }
    if (hasError) {
        qInfo().noquote() << "F2Frame:" << dataString.trimmed() << "ERROR";
    } else {
        qInfo().noquote() << "F2Frame:" << dataString.trimmed();
    }
}

// Constructor for F3Frame, initializes data to the frame size
F3Frame::F3Frame()
{
    frameData.resize(getFrameSize());
    subcodeByte = 0;
    f3FrameType = Subcode;
}

// Get the frame size for F3Frame
int F3Frame::getFrameSize() const
{
    return 32;
}

// Set the frame type as subcode and set the subcode value
void F3Frame::setFrameTypeAsSubcode(quint8 subcodeValue)
{
    f3FrameType = Subcode;
    subcodeByte = subcodeValue;
}

// Set the frame type as sync0 and set the subcode value to 0
void F3Frame::setFrameTypeAsSync0()
{
    f3FrameType = Sync0;
    subcodeByte = 0;
}

// Set the frame type as sync1 and set the subcode value to 0
void F3Frame::setFrameTypeAsSync1()
{
    f3FrameType = Sync1;
    subcodeByte = 0;
}

// Get the F3 frame type
F3Frame::F3FrameType F3Frame::getF3FrameType() const
{
    return f3FrameType;
}

// Get the F3 frame type as a QString
QString F3Frame::getF3FrameTypeAsString() const
{
    switch (f3FrameType) {
    case Subcode:
        return "Subcode";
    case Sync0:
        return "Sync0";
    case Sync1:
        return "Sync1";
    default:
        return "UNKNOWN";
    }
}

// Get the subcode value
quint8 F3Frame::getSubcodeByte() const
{
    return subcodeByte;
}

void F3Frame::showData()
{
    QString dataString;
    bool hasError = false;
    for (int i = 0; i < frameData.size(); ++i) {
        if (frameErrorData[i] == 0) {
            dataString.append(QString("%1 ").arg(frameData[i], 2, 16, QChar('0')));
        } else {
            dataString.append(QString("XX "));
            hasError = true;
        }
    }

    QString errorString;
    if (hasError)
        errorString = "ERROR";
    else
        errorString = "";

    if (f3FrameType == Subcode) {
        qInfo().noquote() << "F3Frame:" << dataString.trimmed()
                          << " subcode:" << QString("0x%1").arg(subcodeByte, 2, 16, QChar('0'))
                          << errorString;
    } else if (f3FrameType == Sync0) {
        qInfo().noquote() << "F3Frame:" << dataString.trimmed() << " Sync0" << errorString;
    } else if (f3FrameType == Sync1) {
        qInfo().noquote() << "F3Frame:" << dataString.trimmed() << " Sync1" << errorString;
    } else {
        qInfo().noquote() << "F3Frame:" << dataString.trimmed() << " UNKNOWN" << errorString;
    }
}
