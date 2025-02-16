/************************************************************************

    enc_f1sectiontof2section.cpp

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

#include "enc_f1sectiontof2section.h"

// F1SectionToF2Section class implementation
F1SectionToF2Section::F1SectionToF2Section()
    : m_delayLine1({1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
                  1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0})
    , m_delayLine2({2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0,
                  2, 2, 2, 2, 0, 0, 0, 0})
    , m_delayLineM({0,  4,  8,  12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52,
                  56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108})
    , m_validF2SectionsCount(0)
{
}

void F1SectionToF2Section::pushSection(F1Section f1Section)
{
    m_inputBuffer.enqueue(f1Section);
    processQueue();
}

F2Section F1SectionToF2Section::popSection()
{
    if (!isReady()) {
        qFatal("F1SectionToF2Section::popSection(): No F2 sections are available.");
    }
    return m_outputBuffer.dequeue();
}

// Note: The F2 frames will not be correct until the delay lines are full
// So lead-in is required to prevent loss of the input date.  For now we will
// just discard the data until the delay lines are full.
//
// This will drop 108+2+1 = 111 F2 frames of data - The decoder will also have
// the same issue and will loose another 111 frames of data (so you need at least
// 222 frames of lead-in data to ensure the decoder has enough data to start decoding)
void F1SectionToF2Section::processQueue()
{
    while (!m_inputBuffer.isEmpty()) {
        F1Section f1Section = m_inputBuffer.dequeue();
        F2Section f2Section;
        f2Section.metadata = f1Section.metadata;

        for (int index = 0; index < 98; ++index) {
            // Pop the F1 frame and copy the data
            F1Frame f1Frame = f1Section.frame(index);
            QVector<uint8_t> data = f1Frame.data();

            // Process the data
            data = m_delayLine2.push(data);
            if (data.isEmpty()) {
                // Generate a blank F2 frame (to keep the section in sync)
                F2Frame f2Frame;
                QVector<uint8_t> blankData(32, 0);
                f2Frame.setData(blankData);
                f2Section.pushFrame(f2Frame);
                continue;
            }

            data = m_interleave.interleave(data); // 24
            m_circ.c2Encode(data); // 24 + 4 = 28

            data = m_delayLineM.push(data); // 28
            if (data.isEmpty()) {
                // Generate a blank F2 frame (to keep the section in sync)
                F2Frame f2Frame;
                QVector<uint8_t> blankData(32, 0);
                f2Frame.setData(blankData);
                f2Section.pushFrame(f2Frame);
                continue;
            }

            m_circ.c1Encode(data); // 28 + 4 = 32

            data = m_delayLine1.push(data); // 32
            if (data.isEmpty()) {
                // Generate a blank F2 frame (to keep the section in sync)
                F2Frame f2Frame;
                QVector<uint8_t> blankData(32, 0);
                f2Frame.setData(blankData);
                f2Section.pushFrame(f2Frame);
                continue;
            }

            m_inverter.invertParity(data); // 32

            // Put the resulting data into an F2 frame and push it to the output buffer
            F2Frame f2Frame;
            f2Frame.setData(data);

            f2Section.pushFrame(f2Frame);
        }

        m_validF2SectionsCount++;
        m_outputBuffer.enqueue(f2Section);
    }
}

bool F1SectionToF2Section::isReady() const
{
    return !m_outputBuffer.isEmpty();
}
