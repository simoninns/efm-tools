/************************************************************************

    dec_f1sectiontodata24section.cpp

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

#include "dec_f1sectiontodata24section.h"

F1SectionToData24Section::F1SectionToData24Section()
    : m_invalidF1FramesCount(0),
      m_validF1FramesCount(0),
      m_corruptBytesCount(0)
{
}

void F1SectionToData24Section::pushSection(const F1Section &f1Section)
{
    // Add the data to the input buffer
    m_inputBuffer.enqueue(f1Section);

    // Process the queue
    processQueue();
}

Data24Section F1SectionToData24Section::popSection()
{
    // Return the first item in the output buffer
    return m_outputBuffer.dequeue();
}

bool F1SectionToData24Section::isReady() const
{
    // Return true if the output buffer is not empty
    return !m_outputBuffer.isEmpty();
}

void F1SectionToData24Section::processQueue()
{
    // Process the input buffer
    while (!m_inputBuffer.isEmpty()) {
        F1Section f1Section = m_inputBuffer.dequeue();
        Data24Section data24Section;

        // Sanity check the F1 section
        if (!f1Section.isComplete()) {
            qFatal("F1SectionToData24Section::processQueue - F1 Section is not complete");
        }

        for (int index = 0; index < 98; ++index) {
            QVector<quint8> data = f1Section.frame(index).data();
            QVector<quint8> errorData = f1Section.frame(index).errorData();

            // ECMA-130 issue 2 page 16 - Clause 16
            // All byte pairs are swapped by the F1 Frame encoder
            if (data.size() == errorData.size()) {
                for (int i = 0; i < data.size() - 1; i += 2) {
                    std::swap(data[i], data[i + 1]);
                    std::swap(errorData[i], errorData[i + 1]);
                }
            } else {
                qFatal("Data and error data size mismatch in F1 frame %d", index);
            }

            // Check the error data (and count any flagged errors)
            quint32 errorCount = f1Section.frame(index).countErrors();

            m_corruptBytesCount += errorCount;

            if (errorCount > 0)
                ++m_invalidF1FramesCount;
            else
                ++m_validF1FramesCount;

            // Put the resulting data into a Data24 frame and push it to the output buffer
            Data24 data24;
            data24.setData(data);
            data24.setErrorData(errorData);

            data24Section.pushFrame(data24);
        }

        data24Section.metadata = f1Section.metadata;

        // Add the section to the output buffer
        m_outputBuffer.enqueue(data24Section);
    }
}

void F1SectionToData24Section::showStatistics()
{
    qInfo() << "F1 Section to Data24 Section statistics:";

    qInfo() << "  Frames:";
    qInfo() << "    Total F1 frames:" << m_validF1FramesCount + m_invalidF1FramesCount;
    qInfo() << "    Valid F1 frames:" << m_validF1FramesCount;
    qInfo() << "    Invalid F1 frames:" << m_invalidF1FramesCount;

    qInfo() << "  Bytes:";
    quint32 validBytes = (m_validF1FramesCount + m_invalidF1FramesCount) * 24;
    qInfo().nospace() << "    Total bytes: " << validBytes + m_corruptBytesCount;
    qInfo().nospace() << "    Valid bytes: " << validBytes;
    qInfo().nospace() << "    Corrupt bytes: " << m_corruptBytesCount;
    qInfo().nospace().noquote() << "    Data loss: "
                                << QString::number((m_corruptBytesCount * 100.0) / validBytes, 'f', 3)
                                << "%";
}