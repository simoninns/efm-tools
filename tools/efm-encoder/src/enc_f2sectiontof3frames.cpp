/************************************************************************

    enc_f2sectiontof3frames.cpp

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

#include "enc_f2sectiontof3frames.h"

// F2SectionToF3Frames class implementation
F2SectionToF3Frames::F2SectionToF3Frames() :
    m_validF3FramesCount(0)
{
}

void F2SectionToF3Frames::pushSection(F2Section f2Section)
{
    m_inputBuffer.enqueue(f2Section);
    processQueue();
}

QVector<F3Frame> F2SectionToF3Frames::popFrames()
{
    if (!isReady()) {
        qFatal("F2SectionToF3Frames::popFrames(): No F3 frames are available.");
    }
    return m_outputBuffer.dequeue();
}

void F2SectionToF3Frames::processQueue()
{
    while (m_inputBuffer.size() >= 1) {
        F2Section f2Section = m_inputBuffer.dequeue();
        QVector<F3Frame> f3Frames;

        // Take the metadata information from the first F2 frame in the section
        Subcode subcode;
        QByteArray subcodeData = subcode.toData(f2Section.metadata);

        for (quint32 symbolNumber = 0; symbolNumber < 98; ++symbolNumber) {
            F2Frame f2Frame = f2Section.frame(symbolNumber);
            F3Frame f3Frame;

            if (symbolNumber == 0) {
                f3Frame.setFrameTypeAsSync0();
            } else if (symbolNumber == 1) {
                f3Frame.setFrameTypeAsSync1();
            } else {
                // Generate the subcode byte
                f3Frame.setFrameTypeAsSubcode(subcodeData[symbolNumber]);
            }

            f3Frame.setData(f2Frame.data());

            m_validF3FramesCount++;
            f3Frames.append(f3Frame);
        }

        m_outputBuffer.enqueue(f3Frames);
    }
}

bool F2SectionToF3Frames::isReady() const
{
    return !m_outputBuffer.isEmpty();
}
