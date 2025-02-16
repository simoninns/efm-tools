/************************************************************************

    dec_f3frametof2section.h

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

#ifndef DEC_F3FRAMETOF2SECTION_H
#define DEC_F3FRAMETOF2SECTION_H

#include "decoders.h"
#include "section.h"
#include "subcode.h"

class F3FrameToF2Section : public Decoder
{
public:
    F3FrameToF2Section();
    void pushFrame(const F3Frame &data);
    F2Section popSection();
    bool isReady() const;

    void showStatistics();

private:
    void processStateMachine();

    QQueue<F3Frame> m_inputBuffer;
    QQueue<F2Section> m_outputBuffer;

    // State machine states
    enum State { ExpectingSync0, ExpectingSync1, ExpectingSubcode, ProcessSection };

    State m_currentState;
    QVector<F3Frame> m_sectionBuffer;

    // State machine state processing functions
    State expectingSync0();
    State expectingSync1();
    State expectingSubcode();
    State processSection();

    // Statistics
    quint32 m_missedSync0s;
    quint32 m_missedSync1s;
    quint32 m_missedSubcodes;
    quint32 m_validSections;
    quint32 m_invalidSections;
    quint32 m_inputF3Frames;
};

#endif // DEC_F3FRAMETOF2SECTION_H