/************************************************************************

    dec_f2sectiontof1section.h

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

#ifndef DEC_F2SECTIONTOF1SECTION_H
#define DEC_F2SECTIONTOF1SECTION_H

#include "decoders.h"
#include "reedsolomon.h"
#include "delay_lines.h"
#include "interleave.h"
#include "inverter.h"

class F2SectionToF1Section : public Decoder
{
public:
    F2SectionToF1Section();
    void pushSection(const F2Section &f2Section);
    F1Section popSection();
    bool isReady() const;

    void showStatistics();

private:
    void processQueue();
    void showData(const QString &description, qint32 index, const QString &timeString, QVector<quint8> &data,
                  QVector<quint8> &dataError);

    QQueue<F2Section> inputBuffer;
    QQueue<F1Section> outputBuffer;

    ReedSolomon circ;

    DelayLines delayLine1;
    DelayLines delayLine2;
    DelayLines delayLineM;

    DelayLines delayLine1Err;
    DelayLines delayLine2Err;
    DelayLines delayLineMErr;

    Interleave interleave;
    Inverter inverter;

    Interleave interleaveErr;

    // Statistics
    quint32 invalidInputF2FramesCount;
    quint32 validInputF2FramesCount;
    quint32 invalidOutputF1FramesCount;
    quint32 validOutputF1FramesCount;
    quint32 dlLostFramesCount;
    quint32 continuityErrorCount;

    quint32 inputByteErrors;
    quint32 outputByteErrors;

    // Continuity check
    qint32 lastFrameNumber;
};

#endif // DEC_F2SECTIONTOF1SECTION_H