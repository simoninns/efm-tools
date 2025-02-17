/************************************************************************

    dec_tvaluestochannel.cpp

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

#include "dec_tvaluestochannel.h"

TvaluesToChannel::TvaluesToChannel()
{
    // Statistics
    m_consumedTValues = 0;
    m_discardedTValues = 0;
    m_channelFrameCount = 0;

    m_perfectFrames = 0;
    m_longFrames = 0;
    m_shortFrames = 0;

    m_overshootSyncs = 0;
    m_undershootSyncs = 0;
    m_perfectSyncs = 0;

    // Set the initial state
    m_currentState = ExpectingInitialSync;
}

void TvaluesToChannel::pushFrame(const QByteArray &data)
{
    // Add the data to the input buffer
    m_inputBuffer.enqueue(data);

    // Process the state machine
    processStateMachine();
}

QByteArray TvaluesToChannel::popFrame()
{
    // Return the first item in the output buffer
    return m_outputBuffer.dequeue();
}

bool TvaluesToChannel::isReady() const
{
    // Return true if the output buffer is not empty
    return !m_outputBuffer.isEmpty();
}

void TvaluesToChannel::processStateMachine()
{
    // Add the input data to the internal t-value buffer
    m_internalBuffer.append(m_inputBuffer.dequeue());

    // We need 588 bits to make a frame.  Every frame starts with T11+T11.
    // So the minimum number of t-values we need is 54 and
    // the maximum number of t-values we can have is 191.  This upper limit
    // is where we need to maintain the buffer size (at 382 for 2 frames).

    while (m_internalBuffer.size() > 382) {
        switch (m_currentState) {
        case ExpectingInitialSync:
            //qDebug() << "TvaluesToChannel::processStateMachine() - State: ExpectingInitialSync";
            m_currentState = expectingInitialSync();
            break;
        case ExpectingSync:
            //qDebug() << "TvaluesToChannel::processStateMachine() - State: ExpectingSync";
            m_currentState = expectingSync();
            break;
        case HandleOvershoot:
            //qDebug() << "TvaluesToChannel::processStateMachine() - State: HandleOvershoot";
            m_currentState = handleOvershoot();
            break;
        case HandleUndershoot:
            //qDebug() << "TvaluesToChannel::processStateMachine() - State: HandleUndershoot";
            m_currentState = handleUndershoot();
            break;
        }
    }
}

TvaluesToChannel::State TvaluesToChannel::expectingInitialSync()
{
    State nextState = ExpectingInitialSync;

    // Expected sync header
    QByteArray t11_t11 = QByteArray::fromHex("0B0B");

    // Does the buffer contain a T11+T11 sequence?
    int initialSyncIndex = m_internalBuffer.indexOf(t11_t11);

    if (initialSyncIndex != -1) {
        if (m_showDebug)
            qDebug() << "TvaluesToChannel::expectingInitialSync() - Initial sync header found at index:" << initialSyncIndex;
        nextState = ExpectingSync;
    } else {
        if (m_showDebug)
            qDebug() << "TvaluesToChannel::expectingInitialSync() - Initial sync header not found, dropping" << m_internalBuffer.size() - 1 << "T-values";
        // Drop all but the last T-value in the buffer
        m_discardedTValues += m_internalBuffer.size() - 1;
        m_internalBuffer = m_internalBuffer.right(1);
    }

    return nextState;
}

TvaluesToChannel::State TvaluesToChannel::expectingSync()
{
    State nextState = ExpectingSync;

    // The internal buffer contains a valid sync at the start
    // Find the next sync header after it
    QByteArray t11_t11 = QByteArray::fromHex("0B0B");
    int syncIndex = m_internalBuffer.indexOf(t11_t11, 2);

    // Do we have a valid second sync header?
    if (syncIndex != -1) {
        // Extract the frame data from (and including) the first sync header until
        // (but not including) the second sync header
        QByteArray frameData = m_internalBuffer.left(syncIndex);

        // Do we have exactly 588 bits of data?  Count the T-values
        int bitCount = countBits(frameData);

        // If the frame data is 550 to 600 bits, we have a valid frame
        if (bitCount > 550 && bitCount < 600) {
            if (bitCount != 588 && m_showDebug)
                qDebug() << "TvaluesToChannel::expectingSync() - Got frame with" << bitCount << "bits - Treating as valid";

            // We have a valid frame
            // Place the frame data into the output buffer
            m_outputBuffer.enqueue(frameData);
            if (m_showDebug && countBits(frameData) != 588)
                qDebug() << "TvaluesToChannel::expectingSync() - Queuing frame of" << countBits(frameData) << "bits";
            m_consumedTValues += frameData.size();
            m_channelFrameCount++;
            m_perfectSyncs++;

            if (bitCount == 588)
                m_perfectFrames++;
            if (bitCount < 588)
                m_longFrames++;
            if (bitCount > 588)
                m_shortFrames++;

            // Remove the frame data from the internal buffer
            m_internalBuffer = m_internalBuffer.right(m_internalBuffer.size() - syncIndex);
            nextState = ExpectingSync;
        } else {
            // This is most likely a missing sync header issue rather than
            // one or more T-values being incorrect. So we'll handle that
            // separately.
            if (bitCount > 588) {
                nextState = HandleOvershoot;
            } else {
                nextState = HandleUndershoot;
            }
        }
    } else {
        // The buffer does not contain a valid second sync header, so throw it away
        
        if (m_showDebug)
            qDebug() << "TvaluesToChannel::expectingSync() - No second sync header found, sync lost - dropping" << m_internalBuffer.size() << "T-values";

        m_discardedTValues += m_internalBuffer.size();
        m_internalBuffer.clear();
        nextState = ExpectingInitialSync;
    }

    return nextState;
}

TvaluesToChannel::State TvaluesToChannel::handleUndershoot()
{
    State nextState = ExpectingSync;

    // The frame data is too short
    m_undershootSyncs++;

    // Find the second sync header
    QByteArray t11_t11 = QByteArray::fromHex("0B0B");
    int secondSyncIndex = m_internalBuffer.indexOf(t11_t11, 2);

    // Find the third sync header
    int thirdSyncIndex = m_internalBuffer.indexOf(t11_t11, secondSyncIndex + 2);

    // So, unless the data is completely corrupt we should have 588 bits between
    // the first and third sync headers (i.e. the second was a corrupt sync header) or
    // 588 bits between the second and third sync headers (i.e. the first was a corrupt sync header)
    //
    // If neither of these conditions are met, we have a corrupt frame data and we have to drop it

    if (thirdSyncIndex != -1) {
        // Value of the Ts between the first and third sync header
        int fttBitCount = countBits(m_internalBuffer, 0, thirdSyncIndex);

        // Value of the Ts between the second and third sync header
        int sttBitCount = countBits(m_internalBuffer, secondSyncIndex, thirdSyncIndex);

        if (fttBitCount > 550 && fttBitCount < 600) {
            if (m_showDebug)
                qDebug() << "TvaluesToChannel::handleUndershoot() - Undershoot frame - Value from first to third sync_header =" << fttBitCount << "bits - treating as valid";
            // Valid frame between the first and third sync headers
            QByteArray frameData = m_internalBuffer.left(thirdSyncIndex);
            m_outputBuffer.enqueue(frameData);
            if (m_showDebug && countBits(frameData) != 588)
                qDebug() << "TvaluesToChannel::handleUndershoot1() - Queuing frame of" << countBits(frameData) << "bits";
            m_consumedTValues += frameData.size();
            m_channelFrameCount++;

            if (fttBitCount == 588)
                m_perfectFrames++;
            if (fttBitCount < 588)
                m_longFrames++;
            if (fttBitCount > 588)
                m_shortFrames++;

            // Remove the frame data from the internal buffer
            m_internalBuffer = m_internalBuffer.right(m_internalBuffer.size() - thirdSyncIndex);
            nextState = ExpectingSync;
        } else if (sttBitCount > 550 && sttBitCount < 600) {
            if (m_showDebug)
                qDebug() << "TvaluesToChannel::handleUndershoot() - Undershoot frame - Value from second to third sync_header =" << sttBitCount << "bits - treating as valid";
            // Valid frame between the second and third sync headers
            QByteArray frameData = m_internalBuffer.mid(secondSyncIndex, thirdSyncIndex - secondSyncIndex);
            m_outputBuffer.enqueue(frameData);
            if (m_showDebug && countBits(frameData) != 588)
                qDebug() << "TvaluesToChannel::handleUndershoot2() - Queuing frame of" << countBits(frameData) << "bits";
            m_consumedTValues += frameData.size();
            m_channelFrameCount++;

            if (sttBitCount == 588)
                m_perfectFrames++;
            if (sttBitCount < 588)
                m_longFrames++;
            if (sttBitCount > 588)
                m_shortFrames++;

            // Remove the frame data from the internal buffer
            m_discardedTValues += secondSyncIndex;
            m_internalBuffer = m_internalBuffer.right(m_internalBuffer.size() - thirdSyncIndex);
            nextState = ExpectingSync;
        } else {
            if (m_showDebug)
                qDebug() << "TvaluesToChannel::handleUndershoot() - First to third sync is" << fttBitCount << "bits, second to third sync is" << sttBitCount << ". Dropping (what might be a) frame.";
            nextState = ExpectingSync;

            // Remove the frame data from the internal buffer
            m_discardedTValues += secondSyncIndex;
            m_internalBuffer = m_internalBuffer.right(m_internalBuffer.size() - thirdSyncIndex);
        }
    } else {
        if (m_internalBuffer.size() <= 382) {
            if (m_showDebug)
                qDebug() << "TvaluesToChannel::handleUndershoot() - No third sync header found.  Staying in undershoot state waiting for more data.";
            nextState = HandleUndershoot;
        } else {
            if (m_showDebug)
                qDebug() << "TvaluesToChannel::handleUndershoot() - No third sync header found - Sync lost.  Dropping" << m_internalBuffer.size() - 1 << "T-values";
            
            m_discardedTValues += m_internalBuffer.size() - 1;
            m_internalBuffer = m_internalBuffer.right(1);
            nextState = ExpectingInitialSync;
        }
    }

    return nextState;
}

TvaluesToChannel::State TvaluesToChannel::handleOvershoot()
{
    State nextState = ExpectingSync;

    // The frame data is too long
    m_overshootSyncs++;

    // Is the overshoot due to a missing/corrupt sync header?
    // Count the bits between the first and second sync headers, if they are 588*2, split
    // the frame data into two frames
    QByteArray t11_t11 = QByteArray::fromHex("0B0B");

    // Find the second sync header
    int syncIndex = m_internalBuffer.indexOf(t11_t11, 2);

    // Do we have a valid second sync header?
    if (syncIndex != -1) {
        // Extract the frame data from (and including) the first sync header until
        // (but not including) the second sync header
        QByteArray frameData = m_internalBuffer.left(syncIndex);

        // Remove the frame data from the internal buffer
        m_internalBuffer = m_internalBuffer.right(m_internalBuffer.size() - syncIndex);

        // How many bits of data do we have?  Count the T-values
        int bitCount = countBits(frameData);

        // If the frame data is 588*2 bits, or within 11 bits of 588*2, we have a two frames
        // separated by a corrupt sync header
        if (bitCount > 588 * 2 - 11 && bitCount < 588 * 2 + 11) {
            // Count the number of T-values to 588 bits
            int firstFrameBits = 0;
            int endOfFrameIndex = 0;
            for (int i = 0; i < frameData.size(); i++) {
                firstFrameBits += frameData.at(i);
                if (firstFrameBits >= 588) {
                    endOfFrameIndex = i;
                    break;
                }
            }

            QByteArray firstFrameData = frameData.left(endOfFrameIndex + 1);
            QByteArray secondFrameData = frameData.right(frameData.size() - endOfFrameIndex - 1);

            quint32 firstFrameBitCount = countBits(firstFrameData);
            quint32 secondFrameBitCount = countBits(secondFrameData);

            // Place the frames into the output buffer
            m_outputBuffer.enqueue(firstFrameData);
            m_outputBuffer.enqueue(secondFrameData);

            if (m_showDebug)
                qDebug() << "TvaluesToChannel::handleOvershoot() - Overshoot frame split -" << firstFrameBitCount << "/" << secondFrameBitCount << "bits";

            m_consumedTValues += frameData.size();
            m_channelFrameCount += 2;

            if (firstFrameBitCount == 588)
                m_perfectFrames++;
            if (firstFrameBitCount < 588)
                m_longFrames++;
            if (firstFrameBitCount > 588)
                m_shortFrames++;

            if (secondFrameBitCount == 588)
                m_perfectFrames++;
            if (secondFrameBitCount < 588)
                m_longFrames++;
            if (secondFrameBitCount > 588)
                m_shortFrames++;

            nextState = ExpectingSync;
        } else {
            if (m_showDebug)
                qDebug() << "TvaluesToChannel::handleOvershoot() - Overshoot frame -" << bitCount << "bits, but no sync header found, dropping" << m_internalBuffer.size() - 1 << "T-values";
            m_internalBuffer = m_internalBuffer.right(1);
            nextState = ExpectingInitialSync;
        }
    } else {
        qFatal("TvaluesToChannel::handleOvershoot() - Overshoot frame detected but no second sync header found, even though it should have been there.");
    }

    return nextState;
}

// Count the number of bits in the array of T-values
quint32 TvaluesToChannel::countBits(const QByteArray &data, qint32 startPosition, qint32 endPosition)
{
    if (endPosition == -1)
        endPosition = data.size();

    quint32 bitCount = 0;
    for (int i = startPosition; i < endPosition; i++) {
        bitCount += data.at(i);
    }
    return bitCount;
}

void TvaluesToChannel::showStatistics()
{
    qInfo() << "T-values to Channel Frame statistics:";
    qInfo() << "  T-Values:";
    qInfo() << "    Consumed:" << m_consumedTValues;
    qInfo() << "    Discarded:" << m_discardedTValues;
    qInfo() << "  Channel frames:";
    qInfo() << "    Total:" << m_channelFrameCount;
    qInfo() << "    588 bits:" << m_perfectFrames;
    qInfo() << "    >588 bits:" << m_longFrames;
    qInfo() << "    <588 bits:" << m_shortFrames;
    qInfo() << "  Sync headers:";
    qInfo() << "    Good syncs:" << m_perfectSyncs;
    qInfo() << "    Overshoots:" << m_overshootSyncs;
    qInfo() << "    Undershoots:" << m_undershootSyncs;

    // When we overshoot and split the frame, we are guessing the sync header...
    qInfo() << "    Guessed:" << m_channelFrameCount - m_perfectSyncs - m_overshootSyncs - m_undershootSyncs;
}