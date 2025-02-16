/************************************************************************

    enc_data24sectiontof1section.cpp

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

#include "enc_data24sectiontof1section.h"

// Data24SectionToF1Section class implementation
Data24SectionToF1Section::Data24SectionToF1Section()
    : validF1SectionsCount(0)
{
}

void Data24SectionToF1Section::pushSection(Data24Section data24Section)
{
    inputBuffer.enqueue(data24Section);
    processQueue();
}

F1Section Data24SectionToF1Section::popSection()
{
    if (!isReady()) {
        qFatal("Data24SectionToF1Section::popSection(): No F1 sections are available.");
    }
    return outputBuffer.dequeue();
}

void Data24SectionToF1Section::processQueue()
{
    while (!inputBuffer.isEmpty()) {
        Data24Section data24Section = inputBuffer.dequeue();
        F1Section f1Section;
        f1Section.metadata = data24Section.metadata;

        for (qint32 index = 0; index < 98; ++index) {
            Data24 data24 = data24Section.frame(index);

            // ECMA-130 issue 2 page 16 - Clause 16
            // All byte pairs are swapped by the F1 Frame encoder
            QVector<quint8> data = data24.data();
            for (qint32 i = 0; i < data.size(); i += 2) {
                std::swap(data[i], data[i + 1]);
            }

            F1Frame f1Frame;
            f1Frame.setData(data);
            f1Section.pushFrame(f1Frame);
        }

        validF1SectionsCount++;
        outputBuffer.enqueue(f1Section);
    }
}

bool Data24SectionToF1Section::isReady() const
{
    return !outputBuffer.isEmpty();
}
