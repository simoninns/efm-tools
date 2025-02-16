/************************************************************************

    enc_f1sectiontof2section.h

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

#ifndef ENC_F1SECTIONTOF2SECTION_H
#define ENC_F1SECTIONTOF2SECTION_H

#include "encoders.h"
#include "delay_lines.h"
#include "interleave.h"
#include "inverter.h"
#include "reedsolomon.h"
#include "section.h"

class F1SectionToF2Section : Encoder
{
public:
    F1SectionToF2Section();
    void pushSection(F1Section f1Section);
    F2Section popSection();
    bool isReady() const;
    quint32 validOutputSectionsCount() const override { return m_validF2SectionsCount; };

private:
    void processQueue();

    QQueue<F1Section> m_inputBuffer;
    QQueue<F2Section> m_outputBuffer;

    ReedSolomon m_circ;

    DelayLines m_delayLine1;
    DelayLines m_delayLine2;
    DelayLines m_delayLineM;

    Interleave m_interleave;
    Inverter m_inverter;

    quint32 m_validF2SectionsCount;
};

#endif // ENC_F1SECTIONTOF2SECTION_H
