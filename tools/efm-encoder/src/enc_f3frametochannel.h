/************************************************************************

    enc_f3frametochannel.h

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

#ifndef ENC_F3FRAMETOCHANNEL_H
#define ENC_F3FRAMETOCHANNEL_H

#include <QTime>
#include <QRandomGenerator>
#include "encoders.h"
#include "efm.h"

class F3FrameToChannel : Encoder
{
public:
    F3FrameToChannel();
    void pushFrame(F3Frame f3Frame);
    QVector<quint8> popFrame();
    bool isReady() const;
    qint32 getTotalTValues() const;
    quint32 validOutputSectionsCount() const override
    {
        return validChannelFramesCount;
    };

    void setCorruption(bool corruptF3Sync, quint32 corruptF3SyncFrequency,
                      bool corruptSubcodeSync, quint32 corruptSubcodeSyncFrequency);

private:
    void processQueue();

    QQueue<F3Frame> inputBuffer;
    QQueue<QVector<quint8>> outputBuffer;

    void writeFrame(QString channelFrame);

    QString addMergingBits(QString channelFrame);
    QStringList getLegalMergingBitPatterns(const QString &currentEfm, const QString &nextEfm);
    QStringList orderPatternsByDsvDelta(const QStringList &mergingPatterns,
                                       const QString &currentEfm, const QString &nextEfm);
    qint32 calculateDsvDelta(const QString data);

    qint32 dsv;
    bool dsvDirection;
    qint32 totalTValues;
    qint32 totalSections;
    QString previousChannelFrame;
    quint32 validChannelFramesCount;
    Efm efm;

    // Define the 24-bit F3 sync header for the F3 frame
    const QString syncHeader = "100000000001000000000010";

    // Corruption flags
    bool corruptF3Sync;
    quint32 corruptF3SyncFrequency;
    bool corruptSubcodeSync;
    quint32 corruptSubcodeSyncFrequency;
    quint32 subcodeCorruptionType;

    QString generateRandomSyncValue();
};

#endif // ENC_F3FRAMETOCHANNEL_H
