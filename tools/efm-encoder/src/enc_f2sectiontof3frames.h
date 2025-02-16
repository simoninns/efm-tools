/************************************************************************

    enc_f2sectiontof3frames.h

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

#ifndef ENC_F2SECTIONTOF3FRAMES_H
#define ENC_F2SECTIONTOF3FRAMES_H

#include "encoders.h"
#include "section.h"
#include "subcode.h"

class F2SectionToF3Frames : Encoder
{
public:
    F2SectionToF3Frames();
    void pushSection(F2Section f2Section);
    QVector<F3Frame> popFrames();
    bool isReady() const;
    quint32 validOutputSectionsCount() const override { return validF3FramesCount; };

private:
    void processQueue();

    QQueue<F2Section> inputBuffer;
    QQueue<QVector<F3Frame>> outputBuffer;

    quint32 validF3FramesCount;
};

#endif // ENC_F2SECTIONTOF3FRAMES_H
