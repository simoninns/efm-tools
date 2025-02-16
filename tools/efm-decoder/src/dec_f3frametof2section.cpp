/************************************************************************

    dec_f3frametof2section.cpp

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

#include "dec_f3frametof2section.h"

F3FrameToF2Section::F3FrameToF2Section()
    : m_currentState(ExpectingSync0),
      m_missedSync0s(0),
      m_missedSync1s(0),
      m_missedSubcodes(0),
      m_validSections(0),
      m_invalidSections(0),
      m_inputF3Frames(0)
{
}

void F3FrameToF2Section::pushFrame(const F3Frame &data)
{
    m_inputBuffer.enqueue(data);
    m_inputF3Frames++;
    processStateMachine();
}

F2Section F3FrameToF2Section::popSection()
{
    return m_outputBuffer.dequeue();
}

bool F3FrameToF2Section::isReady() const
{
    return !m_outputBuffer.isEmpty();
}

void F3FrameToF2Section::processStateMachine()
{
    while (!m_inputBuffer.isEmpty()) {
        switch (m_currentState) {
        case ExpectingSync0:
            m_currentState = expectingSync0();
            break;
        case ExpectingSync1:
            m_currentState = expectingSync1();
            break;
        case ExpectingSubcode:
            m_currentState = expectingSubcode();
            break;
        case ProcessSection:
            m_currentState = processSection();
            break;
        }
    }
}

F3FrameToF2Section::State F3FrameToF2Section::expectingSync0()
{
    F3Frame f3Frame = m_inputBuffer.dequeue();
    State nextState = ExpectingSync0;

    switch (f3Frame.getF3FrameType()) {
    case F3Frame::Sync0:
        m_sectionBuffer.clear();
        m_sectionBuffer.append(f3Frame);
        nextState = ExpectingSync1;
        break;
    case F3Frame::Sync1:
        m_missedSync0s++;
        if (m_showDebug)
            qDebug() << "F3FrameToF2Section::expectingSync0 - Sync1 frame received when expecting Sync0";
        if (!m_sectionBuffer.isEmpty()) {
            F3Frame sync0Frame = m_sectionBuffer.last();
            sync0Frame.setFrameTypeAsSync0();
            m_sectionBuffer.clear();
            m_sectionBuffer.append(sync0Frame);
            m_sectionBuffer.append(f3Frame);
            nextState = ExpectingSubcode;
        } else {
            F3Frame sync0Frame;
            sync0Frame.setFrameTypeAsSync0();
            m_sectionBuffer.clear();
            m_sectionBuffer.append(sync0Frame);
            m_sectionBuffer.append(f3Frame);
            nextState = ExpectingSubcode;
        }
        break;
    case F3Frame::Subcode:
        m_missedSync0s++;
        if (m_showDebug)
            qDebug() << "F3FrameToF2Section::expectingSync0 - Subcode frame received when expecting Sync0";
        f3Frame.setFrameTypeAsSync0();
        m_sectionBuffer.clear();
        m_sectionBuffer.append(f3Frame);
        nextState = ExpectingSync1;
        break;
    }

    return nextState;
}

F3FrameToF2Section::State F3FrameToF2Section::expectingSync1()
{
    F3Frame f3Frame = m_inputBuffer.dequeue();
    State nextState = ExpectingSync1;

    switch (f3Frame.getF3FrameType()) {
    case F3Frame::Sync1:
        m_sectionBuffer.append(f3Frame);
        nextState = ExpectingSubcode;
        break;
    case F3Frame::Sync0:
        m_missedSync1s++;
        if (m_showDebug)
            qDebug() << "F3FrameToF2Section::expectingSync1 - Sync0 frame received when expecting Sync1";
        m_sectionBuffer.clear();
        m_sectionBuffer.append(f3Frame);
        nextState = ExpectingSync1;
        break;
    case F3Frame::Subcode:
        m_missedSync1s++;
        if (m_showDebug)
            qDebug() << "F3FrameToF2Section::expectingSync1 - Subcode frame received when expecting Sync1";
        f3Frame.setFrameTypeAsSync1();
        m_sectionBuffer.append(f3Frame);
        nextState = ExpectingSubcode;
        break;
    }

    return nextState;
}

F3FrameToF2Section::State F3FrameToF2Section::expectingSubcode()
{
    F3Frame f3Frame = m_inputBuffer.dequeue();
    State nextState = ExpectingSubcode;

    switch (f3Frame.getF3FrameType()) {
    case F3Frame::Subcode:
        m_sectionBuffer.append(f3Frame);
        if (m_sectionBuffer.size() == 98) {
            m_validSections++;
            nextState = ProcessSection;
        }
        break;
    case F3Frame::Sync0:
        m_missedSubcodes++;
        if (m_showDebug)
            qDebug() << "F3FrameToF2Section::expectingSubcode - Sync0 frame received when expecting Subcode";
        m_sectionBuffer.clear();
        m_invalidSections++;
        m_sectionBuffer.append(f3Frame);
        nextState = ExpectingSync1;
        break;
    case F3Frame::Sync1:
        m_missedSubcodes++;
        if (m_showDebug)
            qDebug() << "F3FrameToF2Section::expectingSubcode - Sync1 frame received when expecting Subcode";
        f3Frame.setFrameTypeAsSubcode(0);
        m_sectionBuffer.append(f3Frame);
        if (m_sectionBuffer.size() == 98) {
            m_validSections++;
            nextState = ProcessSection;
        }
        break;
    }

    return nextState;
}

F3FrameToF2Section::State F3FrameToF2Section::processSection()
{
    if (m_sectionBuffer.size() != 98) {
        qFatal("F3FrameToF2Section::processSection - Section buffer is not full");
    }

    if (m_sectionBuffer.first().getF3FrameType() != F3Frame::Sync0) {
        qFatal("F3FrameToF2Section::processSection - First frame in section buffer is not a Sync0");
    }

    if (m_sectionBuffer.at(1).getF3FrameType() != F3Frame::Sync1) {
        qFatal("F3FrameToF2Section::processSection - Second frame in section buffer is not a Sync1");
    }

    for (int i = 2; i < 98; ++i) {
        if (m_sectionBuffer.at(i).getF3FrameType() != F3Frame::Subcode) {
            qFatal("F3FrameToF2Section::processSection - Frame %d in section buffer is not a Subcode", i);
        }
    }

    Subcode subcode;
    if (m_showDebug)
        subcode.setShowDebug(true);

    QByteArray subcodeData;
    for (int i = 0; i < 98; ++i) {
        subcodeData.append(m_sectionBuffer[i].getSubcodeByte());
    }
    SectionMetadata sectionMetadata = subcode.fromData(subcodeData);

    F2Section f2Section;
    for (quint32 index = 0; index < 98; ++index) {
        F2Frame f2Frame;
        f2Frame.setData(m_sectionBuffer[index].getData());
        f2Frame.setErrorData(m_sectionBuffer[index].getErrorData());
        f2Section.pushFrame(f2Frame);
    }

    f2Section.metadata = sectionMetadata;
    m_outputBuffer.enqueue(f2Section);

    m_sectionBuffer.clear();
    return ExpectingSync0;
}

void F3FrameToF2Section::showStatistics()
{
    qInfo() << "F3 Frame to F2 Section statistics:";
    qInfo() << "  F2 Sections:";
    qInfo() << "    Valid F2 sections:" << m_validSections;
    qInfo() << "    Invalid F2 sections:" << m_invalidSections;
    qInfo() << "  Sync tracking:";
    qInfo() << "    Missed sync0s:" << m_missedSync0s;
    qInfo() << "    Missed sync1s:" << m_missedSync1s;
    qInfo() << "    Missed subcodes:" << m_missedSubcodes;
    qInfo() << "  F3 Frames:";
    qInfo() << "    Input F3 frames:" << m_inputF3Frames;
    qInfo() << "    Output F2 frames:" << (m_validSections * 98);
    qInfo() << "    Discarded F3 frames:" << m_inputF3Frames - (m_validSections * 98);
}