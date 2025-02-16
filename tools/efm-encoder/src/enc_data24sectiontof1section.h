/************************************************************************

    enc_data24sectiontof1section.h

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

#ifndef ENC_DATA24SECTIONTOF1SECTION_H
#define ENC_DATA24SECTIONTOF1SECTION_H

#include "encoders.h"
#include "section.h"

class Data24SectionToF1Section : Encoder
{
public:
    Data24SectionToF1Section();
    void pushSection(Data24Section data);
    F1Section popSection();
    bool isReady() const;

    quint32 validOutputSectionsCount() const override { return m_validF1SectionsCount; }

private:
    void processQueue();

    QQueue<Data24Section> m_inputBuffer;
    QQueue<F1Section> m_outputBuffer;

    quint32 m_validF1SectionsCount;
};

#endif // ENC_DATA24SECTIONTOF1SECTION_H