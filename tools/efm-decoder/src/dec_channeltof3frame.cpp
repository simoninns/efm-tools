/************************************************************************

    dec_channeltof3frame.cpp

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

#include "dec_channeltof3frame.h"

ChannelToF3Frame::ChannelToF3Frame()
{
    // Statistics
    goodFrames = 0;
    undershootFrames = 0;
    overshootFrames = 0;

    validEfmSymbols = 0;
    invalidEfmSymbols = 0;

    validSubcodeSymbols = 0;
    invalidSubcodeSymbols = 0;
}

void ChannelToF3Frame::pushFrame(const QByteArray &data)
{
    // Add the data to the input buffer
    inputBuffer.enqueue(data);

    // Process queue
    processQueue();
}

F3Frame ChannelToF3Frame::popFrame()
{
    // Return the first item in the output buffer
    return outputBuffer.dequeue();
}

bool ChannelToF3Frame::isReady() const
{
    // Return true if the output buffer is not empty
    return !outputBuffer.isEmpty();
}

void ChannelToF3Frame::processQueue()
{
    while (!inputBuffer.isEmpty()) {
        // Extract the first item in the input buffer
        QByteArray frameData = inputBuffer.dequeue();

        // Count the number of bits in the frame
        int bitCount = 0;
        for (int i = 0; i < frameData.size(); ++i) {
            bitCount += frameData.at(i);
        }

        // Generate statistics
        if (bitCount != 588 && m_showDebug)
            qDebug() << "ChannelToF3Frame::processQueue() - Frame data is" << bitCount
                     << "bits (should be 588)";
        if (bitCount == 588)
            goodFrames++;
        if (bitCount < 588)
            undershootFrames++;
        if (bitCount > 588)
            overshootFrames++;

        // Create an F3 frame
        F3Frame f3Frame = createF3Frame(frameData);

        // Place the frame into the output buffer
        outputBuffer.enqueue(f3Frame);
    }
}

F3Frame ChannelToF3Frame::createF3Frame(const QByteArray &tValues)
{
    F3Frame f3Frame;

    // The channel frame data is:
    //   Sync Header: 24 bits (bits 0-23)
    //   Merging bits: 3 bits (bits 24-26)
    //   Subcode: 14 bits (bits 27-40)
    //   Merging bits: 3 bits (bits 41-43)
    //   Then 32x 17-bit data values (bits 44-587)
    //     Data: 14 bits
    //     Merging bits: 3 bits
    //
    // Giving a total of 588 bits

    // Convert the T-values to data
    QByteArray frameData = tvaluesToData(tValues);

    // Extract the subcode in bits 27-40
    quint16 subcode = efm.fourteenToEight(getBits(frameData, 27, 40));
    if (subcode == 300) {
        subcode = 0;
        invalidSubcodeSymbols++;
    } else {
        validSubcodeSymbols++;
    }

    // Extract the data values in bits 44-587 ignoring the merging bits
    QVector<quint8> dataValues;
    QVector<quint8> errorValues;
    for (int i = 44; i < (frameData.size() * 8) - 13; i += 17) {
        quint16 dataValue = efm.fourteenToEight(getBits(frameData, i, i + 13));

        if (dataValue < 256) {
            dataValues.append(dataValue);
            errorValues.append(0);
            validEfmSymbols++;
        } else {
            dataValues.append(0);
            errorValues.append(1);
            invalidEfmSymbols++;
        }
    }

    // If the data values are not a multiple of 32 (due to undershoot), pad with zeros
    while (dataValues.size() < 32) {
        dataValues.append(0);
        errorValues.append(1);
    }

    // Create an F3 frame...

    // Determine the frame type
    if (subcode == 256)
        f3Frame.setFrameTypeAsSync0();
    else if (subcode == 257)
        f3Frame.setFrameTypeAsSync1();
    else
        f3Frame.setFrameTypeAsSubcode(subcode);

    // Set the frame data
    f3Frame.setData(dataValues);
    f3Frame.setErrorData(errorValues);

    return f3Frame;
}

QByteArray ChannelToF3Frame::tvaluesToData(const QByteArray &tValues)
{
    QByteArray outputData;
    quint8 currentByte = 0; // Will accumulate bits until we have 8.
    quint32 bitsFilled = 0; // How many bits are currently in currentByte.

    // Iterate through each T-value in the input.
    for (quint8 c : tValues) {
        // Convert char to int (assuming the T-value is in the range 3..11)
        int tValue = static_cast<quint8>(c);
        if (tValue < 3 || tValue > 11) {
            qFatal("ChannelToF3Frame::tvaluesToData(): T-value must be in the range 3 to 11.");
        }

        // First, append the leading 1 bit.
        currentByte = (currentByte << 1) | 1;
        ++bitsFilled;
        if (bitsFilled == 8) {
            outputData.append(currentByte);
            currentByte = 0;
            bitsFilled = 0;
        }

        // Then, append (t_value - 1) zeros.
        for (int i = 1; i < tValue; ++i) {
            currentByte = (currentByte << 1); // Append a 0 bit.
            ++bitsFilled;
            if (bitsFilled == 8) {
                outputData.append(currentByte);
                currentByte = 0;
                bitsFilled = 0;
            }
        }
    }

    // If there are remaining bits, pad the final byte with zeros on the right.
    if (bitsFilled > 0) {
        currentByte <<= (8 - bitsFilled);
        outputData.append(currentByte);
    }

    return outputData;
}

quint16 ChannelToF3Frame::getBits(const QByteArray &data, int startBit, int endBit)
{
    quint16 value = 0;

    // Make sure start and end bits are within a 588 bit range
    if (startBit < 0 || startBit > 587) {
        qFatal("ChannelToF3Frame::getBits(): Start bit must be in the range 0 to 587 - start bit was %d.", startBit);
    }

    if (endBit < 0 || endBit > 587) {
        qFatal("ChannelToF3Frame::getBits(): End bit must be in the range 0 to 587 - end bit was %d.", endBit);
    }

    if (startBit > endBit) {
        qFatal("ChannelToF3Frame::getBits(): Start bit must be less than or equal to the end bit.");
    }

    // Extract the bits from the data
    for (int bit = startBit; bit <= endBit; ++bit) {
        int byteIndex = bit / 8;
        if (byteIndex >= data.size()) {
            qFatal("ChannelToF3Frame::getBits(): Byte index of %d exceeds data size of %d.", byteIndex, data.size());
        }
        int bitIndex = bit % 8;
        if (data[byteIndex] & (1 << (7 - bitIndex))) {
            value |= (1 << (endBit - bit));
        }
    }

    return value;
}

void ChannelToF3Frame::showStatistics()
{
    qInfo() << "Channel to F3 Frame statistics:";
    qInfo() << "  Channel Frames:";
    qInfo() << "    Total:" << goodFrames + undershootFrames + overshootFrames;
    qInfo() << "    Good:" << goodFrames;
    qInfo() << "    Undershoot:" << undershootFrames;
    qInfo() << "    Overshoot:" << overshootFrames;
    qInfo() << "  EFM symbols:";
    qInfo() << "    Valid:" << validEfmSymbols;
    qInfo() << "    Invalid:" << invalidEfmSymbols;
    qInfo() << "  Subcode symbols:";
    qInfo() << "    Valid:" << validSubcodeSymbols;
    qInfo() << "    Invalid:" << invalidSubcodeSymbols;
}