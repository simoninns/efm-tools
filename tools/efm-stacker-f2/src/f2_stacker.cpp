/************************************************************************

    f2_stacker.cpp

    efm-stacker-f2 - EFM F2 Section stacker
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    This application is free software: you can redistribute it and/or
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

#include "f2_stacker.h"

F2Stacker::F2Stacker()
{}

bool F2Stacker::process(const QVector<QString> &inputFilenames, const QString &outputFilename)
{
    // Start by opening all the input F2 section files
    for (int index = 0; index < inputFilenames.size(); index++) {
        ReaderF2Section* reader = new ReaderF2Section();
        if (!reader->open(inputFilenames[index])) {
            qCritical() << "F2Stacker::process() - Could not open input file" << inputFilenames[index];
            delete reader;
            return false;
        }
        m_inputFiles.append(reader);
        qDebug() << "Opened input file" << inputFilenames[index];
    }

    // Figure out the time range covered by the input files
    // Note: This is assuming that the input files are in chronological order...
    QVector<SectionTime> m_startTimes;
    QVector<SectionTime> m_endTimes;

    qInfo() << "Scanning input files to get time range of data from each...";
    for (int inputFileIdx = 0; inputFileIdx < m_inputFiles.size(); inputFileIdx++) {
        m_inputFiles[inputFileIdx]->seekToSection(0);
        SectionTime startTime = m_inputFiles[inputFileIdx]->read().metadata.absoluteSectionTime();
        m_inputFiles[inputFileIdx]->seekToSection(m_inputFiles[inputFileIdx]->size() - 1);
        SectionTime endTime = m_inputFiles[inputFileIdx]->read().metadata.absoluteSectionTime();
        m_startTimes.append(startTime);
        m_endTimes.append(endTime);
        
        // Seek back to the start of the file
        m_inputFiles[inputFileIdx]->seekToSection(0);
        qInfo().noquote() << "Input File" << inputFilenames[inputFileIdx] << "- Start:" << startTime.toString() << "End:" << endTime.toString();
    }

    // The start time (for the stacking) is the earliest start time of all the input files
    // The end time (for the stacking) is the latest end time of all the input files
    SectionTime stackStartTime(59,59,74);
    SectionTime stackEndTime(0,0,0);
    for (int index = 0; index < m_startTimes.size(); index++) {
        if (m_startTimes[index] < stackStartTime) {
            stackStartTime = m_startTimes[index];
        }
        if (m_endTimes[index] > stackEndTime) {
            stackEndTime = m_endTimes[index];
        }
    }
    qInfo().noquote() << "Stacking Start Time:" << stackStartTime.toString() << "End Time:" << stackEndTime.toString();

    // Close the input files
    for (int index = 0; index < m_inputFiles.size(); index++) {
        m_inputFiles[index]->close();
        delete m_inputFiles[index];
    }
    m_inputFiles.clear();

    return true;
}