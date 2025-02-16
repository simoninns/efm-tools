/************************************************************************

    enc_f3frametochannel.cpp

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-encoder is free software: you can redistribute it and/or
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

#include "enc_f3frametochannel.h"

// F3FrameToChannel class implementation
F3FrameToChannel::F3FrameToChannel()
    : dsv(0)
    , dsvDirection(true)
    , totalTValues(0)
    , totalSections(0)
    , validChannelFramesCount(0)
    , corruptF3Sync(false)
    , corruptF3SyncFrequency(0)
    , corruptSubcodeSync(false)
    , corruptSubcodeSyncFrequency(0)
    , subcodeCorruptionType(0)
{
    previousChannelFrame.clear();
}

void F3FrameToChannel::pushFrame(F3Frame f3Frame)
{
    inputBuffer.enqueue(f3Frame);
    processQueue();
}

QVector<quint8> F3FrameToChannel::popFrame()
{
    if (!isReady()) {
        qFatal("F3FrameToChannel::popFrame(): No bytes are available.");
    }
    return outputBuffer.dequeue();
}

void F3FrameToChannel::processQueue()
{
    // Seed a random number generator
    QRandomGenerator randomGenerator(
            static_cast<quint32>(QTime::currentTime().msecsSinceStartOfDay()));

    while (!inputBuffer.isEmpty()) {
        // Pop the F3 frame data from the processing queue
        F3Frame f3Frame = inputBuffer.dequeue();
        QVector<quint8> f3FrameData = f3Frame.data();

        // Ensure the F3 frame data is 32 bytes long
        if (f3FrameData.size() != 32) {
            qFatal("F3FrameToChannel::processQueue(): F3 frame data must be 32 bytes long.");
        }

        QString channelFrame;
        QString mergingBits = "xxx";

        // F3 Sync header
        if (corruptF3Sync) {
            if (validChannelFramesCount % corruptF3SyncFrequency == 0) {
                // Generate a random sync header
                // Must be 24 bits long and have a minimum of 2 and a maximum of 10 zeros between
                // each 1
                channelFrame += generateRandomSyncValue();
                qDebug() << "F3FrameToChannel::processQueue(): Corrupting F3 sync header:"
                         << channelFrame;
            } else {
                channelFrame += syncHeader;
            }
        } else {
            channelFrame += syncHeader;
        }
        channelFrame += mergingBits;

        QString subcodeValue;

        // Subcode sync0 and sync1 headers (based on the F3 frame type)
        if (f3Frame.f3FrameType() == F3Frame::F3FrameType::Subcode) {
            subcodeValue = efm.eightToFourteen(f3Frame.subcodeByte());
        } else if (f3Frame.f3FrameType() == F3Frame::F3FrameType::Sync0) {
            subcodeValue += efm.eightToFourteen(256);
            totalSections++;

            // Note: This is 0-8 because we want it to be unlikely that both sync0 and sync1 are
            // corrupted at the same time.  So 0 = corrupt both, 1-4 = corrupt sync0 and 5-8 =
            // corrupt sync1
            subcodeCorruptionType = randomGenerator.bounded(9); // 0-8
        } else {
            // SYNC1
            subcodeValue += efm.eightToFourteen(257);
        }

        // Corrupt the subcode sync0 and sync1 patterns?
        if (corruptSubcodeSync) {
            if (totalSections % corruptSubcodeSyncFrequency == 0) {
                if (f3Frame.f3FrameType() == F3Frame::F3FrameType::Sync0) {
                    if (subcodeCorruptionType == 0
                        || (subcodeCorruptionType >= 1 && subcodeCorruptionType <= 4)) {
                        // Corrupt the sync0 pattern
                        subcodeValue = efm.eightToFourteen(randomGenerator.bounded(256));
                        qDebug() << "F3FrameToChannel::processQueue(): Corrupting subcode sync0 "
                                    "value:"
                                 << subcodeValue;
                    }
                }

                if (f3Frame.f3FrameType() == F3Frame::F3FrameType::Sync1) {
                    if (subcodeCorruptionType == 0
                        || (subcodeCorruptionType >= 5 && subcodeCorruptionType <= 8)) {
                        // Corrupt the sync1 pattern
                        subcodeValue = efm.eightToFourteen(randomGenerator.bounded(256));
                        qDebug() << "F3FrameToChannel::processQueue(): Corrupting subcode sync1 "
                                    "value:"
                                 << subcodeValue;
                    }
                }
            }
        }

        channelFrame += subcodeValue;
        channelFrame += mergingBits;

        // Now we output the actual F3 frame data
        for (quint32 index = 0; index < f3FrameData.size(); index++) {
            channelFrame += efm.eightToFourteen(f3FrameData[index]);
            channelFrame += mergingBits;
        }

        // Now we add the merging bits to the frame
        channelFrame = addMergingBits(channelFrame);

        // Sanity check - Frame size should be 588 bits
        if (channelFrame.size() != 588) {
            qDebug() << "F3FrameToChannel::processQueue(): Channel frame:" << channelFrame;
            qFatal("F3FrameToChannel::processQueue(): BUG - Merged channel frame size is not 588 "
                   "bits. It is %d bits",
                   channelFrame.size());
        }

        // Sanity check - The frame should only contain one sync header (unless we are corrupting
        // it)
        if (channelFrame.count(syncHeader) != 1 && !corruptF3Sync) {
            qDebug() << "F3FrameToChannel::processQueue(): Channel frame:" << channelFrame;
            qFatal("F3FrameToChannel::processQueue(): BUG - Channel frame contains %d sync "
                   "headers.",
                   channelFrame.count(syncHeader));
        }

        // Sanity check - This and the previous frame combined should contain only two sync headers
        // (edge case detection)
        if (!previousChannelFrame.isEmpty()) {
            QString combinedFrames = previousChannelFrame + channelFrame;
            if (combinedFrames.count(syncHeader) != 2) {
                qDebug() << "F3FrameToChannel::processQueue(): Previous frame:"
                         << previousChannelFrame;
                qDebug() << "F3FrameToChannel::processQueue():  Current frame:" << channelFrame;
                qFatal("F3FrameToChannel::processQueue(): BUG - Previous and current channel "
                       "frames combined contains more than two sync headers.");
            }
        }

        // Flush the output data to the output buffer
        validChannelFramesCount++;
        writeFrame(channelFrame);
    }
}

void F3FrameToChannel::writeFrame(QString channelFrame)
{
    // Since the base class uses QVector<quint8> for the output buffer we have to as well
    QVector<quint8> outputBytes;

    // Check the input data size
    if (channelFrame.size() != 588) {
        qFatal("F3FrameToChannel::writeFrame(): Channel frame size is not 588 bits.");
    }

    for (int i = 0; i < channelFrame.size(); ++i) {
        // Is the first bit a 1?
        if (channelFrame[i] == '1') {
            // Yes, so count the number of 0s until the next 1
            quint32 zeroCount = 0;
            for (quint32 j = 1; i < channelFrame.size(); j++) {
                if ((j + i) < channelFrame.size() && channelFrame[j + i] == '0') {
                    zeroCount++;
                } else {
                    break;
                }
            }

            // The number of zeros is not between 2 and 10 - something is wrong in the input data
            if (zeroCount < 2 || zeroCount > 10) {
                qInfo() << "F3FrameToChannel::writeFrame(): Channel frame:" << channelFrame;
                qInfo() << "F3FrameToChannel::writeFrame(): Zero count:" << zeroCount;
                qFatal("F3FrameToChannel::writeFrame(): Number of zeros between ones is not "
                       "between 2 and 10.");
            }

            i += zeroCount;

            // Append the T-value to the output bytes (the number of zeros plus 1)
            outputBytes.append(zeroCount + 1);
            totalTValues++;
        } else {
            // First bit is zero... input data is invalid!
            qFatal("F3FrameToChannel::writeFrame(): First bit should not be zero!");
        }
    }

    outputBuffer.enqueue(outputBytes);
}

bool F3FrameToChannel::isReady() const
{
    return !outputBuffer.isEmpty();
}

QString F3FrameToChannel::addMergingBits(QString channelFrame)
{
    // Ensure the channel frame is 588 bits long
    if (channelFrame.size() != 588) {
        qFatal("F3FrameToChannel::addMergingBits(): Channel frame is not 588 bits long.");
    }

    // We need to add another sync header to the channel frame
    // to make sure the last merging bit is correct
    QString mergedFrame = channelFrame + syncHeader;

    // The merging bits are represented by "xxx".  Each merging bit is preceded by and
    // followed by an EFM symbol.  Firstly we have to extract the preceeding and following
    // EFM symbols.

    // There are 34 sets of merging bits in the 588 bit frame
    for (int i = 0; i < 34; ++i) {
        QString currentEfm;
        QString nextEfm;

        // Set start to the next set of merging bits, the first merging bits are 24 bits in and then
        // every 14 bits
        quint32 start = 0;
        if (i == 0) {
            // The first EFM symbol is a sync header
            currentEfm = syncHeader;
            start = 24;
        } else {
            start = 24 + i * 17;

            // Extract the preceding EFM symbol
            currentEfm = mergedFrame.mid(start - 14, 14);
        }

        // Extract the following EFM symbol
        if (i == 33) {
            // If it's the last set of merging bits, the next EFM symbol is the sync header of the
            // next frame
            nextEfm = syncHeader;
        } else {
            nextEfm = mergedFrame.mid(start + 3, 14);
        }

        if (nextEfm.isEmpty())
            qFatal("F3FrameToChannel::addMergingBits(): Next EFM symbol is empty.");

        // Get a list of legal merging bit patterns
        QStringList mergingPatterns = getLegalMergingBitPatterns(currentEfm, nextEfm);

        // Order the patterns by the resulting DSV delta
        mergingPatterns = orderPatternsByDsvDelta(mergingPatterns, currentEfm, nextEfm);

        // Pick the first pattern from the list that doesn't form a spurious syncHeader pattern
        QString mergingBits;
        for (const QString &pattern : mergingPatterns) {
            QString tempFrame = mergedFrame;

            // Count the current number of sync headers (might be 0 due to corruption settings)
            int syncHeaderCount = tempFrame.count(syncHeader);

            // Add our possible pattern to the frame
            tempFrame.replace(start, 3, pattern);

            // Any spurious sync headers generated?
            if (tempFrame.count(syncHeader) == syncHeaderCount) {
                mergingBits = pattern;
                break;
            }
        }

        // This shouldn't happen, but it it does, let's get debug information :)
        if (mergingBits.isEmpty()) {
            qDebug() << "F3FrameToChannel::addMergingBits(): No legal merging bit pattern found.";
            qDebug() << "F3FrameToChannel::addMergingBits(): Possible patterns:"
                     << mergingPatterns;
            qDebug() << "F3FrameToChannel::addMergingBits(): Merging bits start at:" << start;
            qDebug() << "F3FrameToChannel::addMergingBits():" << mergedFrame.mid(start - 24, 24)
                     << "xxx" << mergedFrame.mid(start + 3, 24);

            qFatal("F3FrameToChannel::addMergingBits(): No legal merging bit pattern found - "
                   "encode failed");
        }

        // Replace the "xxx" with the chosen merging bits
        mergedFrame.replace(start, 3, mergingBits);
    }

    // Remove the last sync header
    mergedFrame.chop(syncHeader.size());

    // Verify the merged frame is 588 bits long
    if (mergedFrame.size() != 588) {
        qFatal("F3FrameToChannel::addMergingBits(): Merged frame is not 588 bits long.");
    }

    return mergedFrame;
}

// Returns a list of merging bit patterns that don't violate the ECMA-130 rules
// of at least 2 zeros and no more than 10 zeros between ones
QStringList F3FrameToChannel::getLegalMergingBitPatterns(const QString &currentEfm,
                                                         const QString &nextEfm)
{
    // Check that currentEfm is at least 14 bits long
    if (currentEfm.size() < 14) {
        qFatal("F3FrameToChannel::getLegalMergingBitPatterns(): Current EFM symbol is too "
               "short.");
    }

    // Check that nextEfm is at least 14 bits long
    if (nextEfm.size() < 14) {
        qFatal("F3FrameToChannel::getLegalMergingBitPatterns(): Next EFM symbol is too short.");
    }

    QStringList patterns;
    QStringList possiblePatterns = { "000", "001", "010", "100" };
    for (const QString &pattern : possiblePatterns) {
        QString combined = currentEfm + pattern + nextEfm;

        int maxZeros = 0;
        int minZeros = std::numeric_limits<int>::max();
        int currentZeros = 0;
        bool insideZeros = false;

        // Remove leading zeros from the combined string
        while (combined[0] == '0') {
            combined.remove(0, 1);
        }

        for (QChar ch : combined) {
            if (ch == '1') {
                if (insideZeros) {
                    if (currentZeros > 0) {
                        maxZeros = qMax(maxZeros, currentZeros);
                        minZeros = qMin(minZeros, currentZeros);
                    }
                    currentZeros = 0;
                    insideZeros = false;
                }
            } else if (ch == '0') {
                insideZeros = true;
                currentZeros++;
            }
        }

        // If no zeros were found between ones
        if (minZeros == std::numeric_limits<int>::max()) {
            minZeros = 0;
        }

        // Check for illegal 11 pattern
        if (combined.contains("11")) {
            minZeros = 0;
        }

        // The pattern is only considered if the combined string conforms to the "at least 2 zeros
        // and no more than 10 zeros between ones" rule
        if (minZeros >= 2 && maxZeros <= 10) {
            patterns.append(pattern);
        }
    }

    return patterns;
}

// Accepts a list of patterns and orders them by DSV delta (lowest to highest)
QStringList F3FrameToChannel::orderPatternsByDsvDelta(const QStringList &mergingPatterns,
                                                      const QString &currentEfm,
                                                      const QString &nextEfm)
{
    QStringList orderedPatterns;

    QMap<qint32, QString> dsvMap;
    for (int i = 0; i < mergingPatterns.size(); ++i) {
        QString combined = currentEfm + mergingPatterns[i] + nextEfm;
        qint32 dsvDelta = calculateDsvDelta(combined);
        dsvMap.insert(dsvDelta, mergingPatterns[i]);
    }

    for (auto it = dsvMap.begin(); it != dsvMap.end(); ++it) {
        orderedPatterns.append(it.value());
    }

    return orderedPatterns;
}

// This function calculates the DSV delta for the input data i.e. the change in DSV
// if the data is used to generate a channel frame.
qint32 F3FrameToChannel::calculateDsvDelta(const QString data)
{
    // The DSV is based on transitions between pits and lands in the EFM data
    // rather than the number of 1s and 0s.

    // If the first value isn't a 1 - count the zeros until the next 1
    // If dsvDirection is true, we increment dsv by the number of zeros
    // If dsvDirection is false, we decrement dsv by the number of zeros
    // Then we flip dsvDirection and repeat until we run out of data

    qint32 dsvDelta = 0;

    for (int i = 0; i < data.size(); ++i) {
        if (data[i] == '1') {
            dsvDirection = !dsvDirection;
        } else {
            int zeroCount = 0;
            while (i < data.size() && data[i] == '0') {
                zeroCount++;
                i++;
            }
            if (dsvDirection) {
                dsvDelta += zeroCount;
            } else {
                dsvDelta -= zeroCount;
            }
            dsvDirection = !dsvDirection;
        }
    }

    return dsvDelta;
}

qint32 F3FrameToChannel::getTotalTValues() const
{
    return totalTValues;
}

void F3FrameToChannel::setCorruption(bool _corruptF3Sync, quint32 _corruptF3SyncFrequency,
                                     bool _corruptSubcodeSync,
                                     quint32 _corruptSubcodeSyncFrequency)
{
    corruptF3Sync = _corruptF3Sync;
    corruptF3SyncFrequency = _corruptF3SyncFrequency;
    corruptSubcodeSync = _corruptSubcodeSync;
    corruptSubcodeSyncFrequency = _corruptSubcodeSyncFrequency;
}

// Function to generate a "random" 24-bit sync
QString F3FrameToChannel::generateRandomSyncValue()
{

    // Seed the random number generator
    QRandomGenerator *generator = QRandomGenerator::global();

    QStringList possibleReplacements{
        "100100000001000000000010", "100000100010000010000010", "100000000001001000000010",
        "100000010000100001000010", "100100000001001000000010", "100000100000010001000010",
        "100100000001000010000010", "100000100010001000100010", "100001000001001000000010",
        "100100010000010001000010", "100100000010001001000010", "100000100001001000100010",
    };

    // Pick a random value between 0 and the length of the possible replacements
    quint32 randomIndex = generator->bounded(possibleReplacements.size());

    return possibleReplacements[randomIndex];
}