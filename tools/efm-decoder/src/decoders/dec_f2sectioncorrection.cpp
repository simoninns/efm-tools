/************************************************************************

    dec_f2sectioncorrection.cpp

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

#include "dec_f2sectioncorrection.h"

// Note: The reason for the internal buffer is to allow for skips back in time, so that
// we can correct for missing sections.  However, this check is not implemented yet.

F2SectionCorrection::F2SectionCorrection()
    : m_leadinComplete(false),
      m_maximumGapSize(3),
      m_maximumInternalBufferSize(375),
      m_totalSections(0),
      m_correctedSections(0),
      m_uncorrectableSections(0),
      m_preLeadinSections(0),
      m_missingSections(0),
      m_absoluteStartTime(59, 59, 74),
      m_absoluteEndTime(0, 0, 0),
      m_outOfOrderSections(0)
{}

void F2SectionCorrection::pushSection(const F2Section &data)
{
    // Add the data to the input buffer
    m_inputBuffer.enqueue(data);

    // Process the queue
    processQueue();
}

F2Section F2SectionCorrection::popSection()
{
    // Return the first item in the output buffer
    return m_outputBuffer.dequeue();
}

bool F2SectionCorrection::isReady() const
{
    // Return true if the output buffer is not empty
    return !m_outputBuffer.isEmpty();
}

void F2SectionCorrection::processQueue()
{
    // Process the input buffer
    while (!m_inputBuffer.isEmpty()) {
        // Dequeue the next section
        F2Section f2Section = m_inputBuffer.dequeue();

        // Do we have a last known good section?
        if (!m_leadinComplete) {
            waitForInputToSettle(f2Section);
        } else {
            waitingForSection(f2Section);
        }
    }
}

// This function waits for the input to settle before processing the sections.
// Especially if the input EFM is from a whole disc capture, there will be frames
// at the start in a random order (from the disc spinning up) and we need to wait
// until we receive a few valid sections in chronological order before we can start
// processing them.
//
// This function collects sections until there are 5 valid, chronological sections
// in a row.  Once we have these, we can start processing the sections.
void F2SectionCorrection::waitForInputToSettle(F2Section &f2Section)
{
    // Does the current section have valid metadata?
    if (f2Section.metadata.isValid()) {
        // Do we have any sections in the leadin buffer?
        if (!m_leadinBuffer.isEmpty()) {
            // Ensure that the current section's time-stamp is one greater than the last section in
            // the leadin buffer
            SectionTime expectedAbsoluteTime =
                    m_leadinBuffer.last().metadata.absoluteSectionTime() + 1;
            if (f2Section.metadata.absoluteSectionTime() == expectedAbsoluteTime) {
                // Add the new section to the leadin buffer
                m_leadinBuffer.enqueue(f2Section);
                if (m_showDebug)
                    qDebug() << "F2SectionCorrection::waitForInputToSettle(): Added valid "
                                "section to leadin buffer with absolute time"
                             << f2Section.metadata.absoluteSectionTime().toString();

                // Do we have 5 valid, contigious sections in the leadin buffer?
                if (m_leadinBuffer.size() >= 5) {
                    m_leadinComplete = true;

                    // Feed the leadin buffer into the section correction process
                    if (m_showDebug)
                        qDebug() << "F2SectionCorrection::waitForInputToSettle(): Leadin "
                                    "buffer complete, pushing collected sections for processing.";
                    while (!m_leadinBuffer.isEmpty()) {
                        F2Section leadinSection = m_leadinBuffer.dequeue();
                        waitingForSection(leadinSection);
                    }
                }
            } else {
                // The current section's time-stamp is not one greater than the last section in the
                // leadin buffer This invalidates the whole leadin buffer
                m_preLeadinSections += m_leadinBuffer.size() + 1;
                m_leadinBuffer.clear();
                if (m_showDebug)
                    qDebug() << "F2SectionCorrection::waitForInputToSettle(): Got section with "
                                "invalid absolute time whilst waiting for input to settle (lead in "
                                "buffer discarded).";
            }
        } else {
            // The leadin buffer is empty, so we can just add the section
            m_leadinBuffer.enqueue(f2Section);
            if (m_showDebug)
                qDebug() << "F2SectionCorrection::waitForInputToSettle(): Added section to "
                            "lead in buffer with absolute time"
                         << f2Section.metadata.absoluteSectionTime().toString();
        }
    } else {
        // The current section doesn't have valid metadata
        // This invalidates the whole buffer
        m_preLeadinSections += m_leadinBuffer.size() + 1;
        m_leadinBuffer.clear();
        if (m_showDebug)
            qDebug() << "F2SectionCorrection::waitForInputToSettle(): Got invalid metadata "
                        "section whilst waiting for input to settle (lead in buffer discarded).";
    }
}

void F2SectionCorrection::waitingForSection(F2Section &f2Section)
{
    bool outputSection = true;

    // Check that this isn't the first section in the internal buffer (as we can't calculate
    // the expected time for the first section)
    if (m_internalBuffer.isEmpty()) {
        if (f2Section.metadata.isValid()) {
            // The internal buffer is empty, so we can just add the section
            m_internalBuffer.enqueue(f2Section);
            if (m_showDebug)
                qDebug() << "F2SectionCorrection::waitingForSection(): Added section to internal "
                            "buffer with absolute time"
                         << f2Section.metadata.absoluteSectionTime().toString();
            return;
        } else {
            qDebug() << "F2SectionCorrection::waitingForSection(): Got invalid metadata section "
                        "whilst waiting for first section.";
            return;
        }
    }

    // What is the next expected section time?
    SectionTime expectedAbsoluteTime = getExpectedAbsoluteTime();

    // Push the section into the internal buffer
    if (m_showDebug) {
        if (f2Section.metadata.isValid()) {
            // if (m_showDebug) qDebug() << "F2SectionCorrection::waitingForSection(): Expected
            // absolute time is" << expectedAbsoluteTime.toString() << "actual absolute time is"
            // << f2Section.metadata.absoluteSectionTime().toString();
        } else {
            if (m_showDebug)
                qDebug() << "F2SectionCorrection::waitingForSection(): Pushing F2 Section with "
                            "invalid metadata CRC to internal buffer.  Expected absolute time is"
                         << expectedAbsoluteTime.toString();
        }
    }

    // Does the current section have the expected absolute time?
    if (f2Section.metadata.isValid()
        && f2Section.metadata.absoluteSectionTime() != expectedAbsoluteTime) {

        // The current section is not the expected section
        if (f2Section.metadata.absoluteSectionTime() > expectedAbsoluteTime) {
            // The current section is ahead of the expected section in time, so we have 
            // one or more missing sections

            // Note: This will kick up the number of C1/C2 errors in the output. However, some
            // LaserDiscs (like Domesday AIV) have gaps in the EFM data, so there is no actual
            // data loss.  TODO: This could be improved by handling it better in the statistics
            // output.

            // How many sections are missing?
            qint32 missingSections = f2Section.metadata.absoluteSectionTime().frames()
                    - expectedAbsoluteTime.frames();

            if (missingSections > 5) {
                // The gap is large - warn the user
                qWarning() << "F2SectionCorrection::waitingForSection(): Missing section gap of" << missingSections << "is larger than 5,"
                        << "expected absolute time is"
                        << expectedAbsoluteTime.toString() << "actual absolute time is"
                        << f2Section.metadata.absoluteSectionTime().toString();
                qWarning() << "F2SectionCorrection::waitingForSection(): It's possible that there is a gap in the EFM data,"
                        << "this will show up as C1/C2 errors in the output but will not actually result in real data loss.";
            }

            if (m_showDebug && missingSections == 1)
                qWarning() << "F2SectionCorrection::waitingForSection(): Missing section detected, "
                            "expected absolute time is"
                        << expectedAbsoluteTime.toString() << "actual absolute time is"
                        << f2Section.metadata.absoluteSectionTime().toString();

            if (m_showDebug && missingSections > 1)
                qWarning() << "F2SectionCorrection::waitingForSection():" << missingSections << "missing sections detected, "
                            "expected absolute time is"
                        << expectedAbsoluteTime.toString() << "actual absolute time is"
                        << f2Section.metadata.absoluteSectionTime().toString();

            for (int i = 0; i < missingSections; ++i) {
                // We have to insert a dummy section into the internal buffer or this
                // will throw off the correction process due to the delay lines
                m_missingSections++;

                // It's important that all the metadata is correct otherwise track numbers and so on
                // will be incorrect
                F2Section missingSection;

                // Copy the metadata from the next section as a good default
                missingSection.metadata = f2Section.metadata;

                missingSection.metadata.setAbsoluteSectionTime(expectedAbsoluteTime + i);
                missingSection.metadata.setValid(true);

                // To-do: Perhaps this could be improved if spanning a track boundary?
                // might not be required though...
                missingSection.metadata.setSectionType(f2Section.metadata.sectionType());
                missingSection.metadata.setTrackNumber(f2Section.metadata.trackNumber());
                missingSection.metadata.setSectionTime(f2Section.metadata.sectionTime() - (i + 1));

                // Push 98 error frames in to the missing section
                for (int i = 0; i < 98; ++i) {
                    F2Frame errorFrame;
                    errorFrame.setData(QVector<quint8>(32, 0x00));
                    errorFrame.setErrorData(QVector<quint8>(32, 0x01)); // Flag as error
                    missingSection.pushFrame(errorFrame);
                }

                // Push it into the internal buffer
                m_internalBuffer.enqueue(missingSection);

                if (m_showDebug)
                    qDebug() << "F2SectionCorrection::waitingForSection(): Inserted missing section "
                                "into internal buffer with absolute time"
                             << missingSection.metadata.absoluteSectionTime().toString();
            }
        } else {
            // The current section is behind the expected section in time, so we have a section
            // that is out of order
            qWarning() << "F2SectionCorrection::waitingForSection(): Section out of order detected, "
                          "expected absolute time is"
                      << expectedAbsoluteTime.toString() << "actual absolute time is"
                      << f2Section.metadata.absoluteSectionTime().toString();

            // TODO: This is not a good way to deal with this...
            outputSection = false;
            m_outOfOrderSections++;
        }
    }

    if (outputSection) m_internalBuffer.enqueue(f2Section);
    correctInternalBuffer();
    while (m_internalBuffer.size() > m_maximumInternalBufferSize)
        outputSections();
}

// Figure out what absolute time is expected for the next section by looking at the internal buffer
SectionTime F2SectionCorrection::getExpectedAbsoluteTime() const
{
    SectionTime expectedAbsoluteTime(0, 0, 0);

    // Find the last valid section in the internal buffer
    for (int i = m_internalBuffer.size() - 1; i >= 0; --i) {
        if (m_internalBuffer[i].metadata.isValid()) {
            // The expected time is the time of the last valid section in the internal buffer plus
            // any following sections until the end of the buffer
            expectedAbsoluteTime = m_internalBuffer[i].metadata.absoluteSectionTime()
                    + (m_internalBuffer.size() - i);
            break;
        }
    }

    return expectedAbsoluteTime;
}

// This function corrects any missing sections in the internal buffer (if possible)
void F2SectionCorrection::correctInternalBuffer()
{
    // Sanity check - there cannot be an invalid section at the start of the buffer
    if (!m_internalBuffer.isEmpty() && !m_internalBuffer.first().metadata.isValid()) {
        if (m_showDebug)
            qDebug() << "F2SectionCorrection::correctInternalBuffer(): Invalid section at start "
                        "of internal buffer!";
        qFatal("F2SectionCorrection::correctInternalBuffer(): Exiting due to invalid section at "
               "start of internal buffer.");
        return;
    }

    // Sanity check - there cannot be an invalid section at the end of the buffer
    if (!m_internalBuffer.isEmpty() && !m_internalBuffer.last().metadata.isValid()) {
        if (m_showDebug)
            qDebug() << "F2SectionCorrection::correctInternalBuffer(): Invalid section at end of "
                        "internal buffer - cannot correct internal buffer until valid section is "
                        "pushed";
        return;
    }

    // Sanity check - there must be at least 3 sections in the buffer
    if (m_internalBuffer.size() < 3) {
        if (m_showDebug)
            qDebug() << "F2SectionCorrection::correctInternalBuffer(): Not enough sections in "
                        "internal buffer to correct.";
        return;
    }

    // Starting from the second section in the buffer, look for an invalid section
    for (int index = 1; index < m_internalBuffer.size(); ++index) {
        // Is the current section invalid?
        int errorStart = -1;
        int errorEnd = -1;

        if (!m_internalBuffer[index].metadata.isValid()) {
            errorStart = index - 1; // This is the "last known good" section

            // Count how many invalid sections there are before the next valid section
            for (int i = index + 1; i < m_internalBuffer.size(); ++i) {
                if (m_internalBuffer[i].metadata.isValid()) {
                    errorEnd = i;
                    break;
                }
            }

            int gapLength = errorEnd - errorStart - 1;
            int timeDifference =
                    m_internalBuffer[errorEnd].metadata.absoluteSectionTime().frames()
                    - m_internalBuffer[errorStart].metadata.absoluteSectionTime().frames()
                    - 1;

            if (m_showDebug)
                qDebug().nospace().noquote()
                        << "F2SectionCorrection::correctInternalBuffer(): Error start position "
                        << errorStart << " ("
                        << m_internalBuffer[errorStart]
                                   .metadata.absoluteSectionTime()
                                   .toString()
                        << ") Error end position " << errorEnd << " ("
                        << m_internalBuffer[errorEnd]
                                   .metadata.absoluteSectionTime()
                                   .toString()
                        << ") gap length is " << gapLength << " time difference is "
                        << timeDifference;

            // Is the gap length below the allowed maximum?
            if (gapLength > m_maximumGapSize) {
                // The gap is too large to correct
                qFatal("F2SectionCorrection::correctInternalBuffer(): Exiting due to gap too "
                       "large in internal buffer.");
            }

            // Can we correct the error?
            if (gapLength == timeDifference) {
                // We can correct the error
                for (int i = errorStart + 1; i < errorEnd; ++i) {
                    // Firstly copy the metadata from the last known good section to ensure good
                    // defaults
                    m_internalBuffer[i].metadata = m_internalBuffer[errorStart].metadata;

                    // Now set the absolute time for the section
                    SectionTime expectedTime =
                            m_internalBuffer[errorStart].metadata.absoluteSectionTime()
                            + (i - errorStart);
                    m_internalBuffer[i].metadata.setAbsoluteSectionTime(expectedTime);

                    // Is the track number the same at the start and end of the gap?
                    if (m_internalBuffer[errorStart].metadata.trackNumber()
                        != m_internalBuffer[errorEnd].metadata.trackNumber()) {
                        if (m_showDebug)
                            qDebug() << "F2SectionCorrection::correctInternalBuffer(): Gap "
                                        "starts on track"
                                     << m_internalBuffer[errorStart].metadata.trackNumber()
                                     << "and ends on track"
                                     << m_internalBuffer[errorEnd].metadata.trackNumber();

                        // Firstly, we have to figure out which track the error section in on.  We
                        // can do this by looking at the section time of the error_end section and
                        // calculating the current section time from it.  If the time is positive
                        // the error section is in the same track as error_end, otherwise it is in
                        // the same track as error_start
                        SectionTime currentTime =
                                m_internalBuffer[errorEnd].metadata.sectionTime()
                                - (errorEnd - i);

                        // Now we can set the track number and correct the section time
                        if (currentTime.frames() >= 0) {
                            m_internalBuffer[i].metadata.setTrackNumber(
                                    m_internalBuffer[errorEnd].metadata.trackNumber());
                            m_internalBuffer[i].metadata.setSectionTime(
                                    m_internalBuffer[errorEnd].metadata.sectionTime()
                                    - (errorEnd - i));
                        } else {
                            m_internalBuffer[i].metadata.setTrackNumber(
                                    m_internalBuffer[errorStart].metadata.trackNumber());
                            m_internalBuffer[i].metadata.setSectionTime(
                                    m_internalBuffer[errorStart].metadata.sectionTime()
                                    + (i - errorStart));
                        }

                        // Adding a qFatal here as this functionality is untested
                        // (I haven't found any examples of this in the test data)
                        qFatal("F2SectionCorrection::correctInternalBuffer(): Exiting due to "
                               "track change in internal buffer - untested functionality - please "
                               "confirm!");
                    } else {
                        // The track number is the same, so we can correct the track number by just
                        // copying it
                        m_internalBuffer[i].metadata.setTrackNumber(
                                m_internalBuffer[errorStart].metadata.trackNumber());

                        // Same thing for the section time
                        SectionTime expectedSectionTime =
                                m_internalBuffer[errorStart].metadata.sectionTime()
                                + (i - errorStart);
                    }

                    // Mark the corrected metadata as valid
                    m_internalBuffer[i].metadata.setValid(true);

                    m_correctedSections++;
                    if (m_showDebug)
                        qDebug().noquote().nospace()
                                << "F2SectionCorrection::correctInternalBuffer(): Corrected "
                                   "section "
                                << i << " with absolute time "
                                << m_internalBuffer[i]
                                           .metadata.absoluteSectionTime()
                                           .toString()
                                << ", Track number "
                                << m_internalBuffer[i].metadata.trackNumber()
                                << " and track time "
                                << m_internalBuffer[i].metadata.sectionTime().toString();
                }
            } else {
                // We cannot correct the error
                qFatal("F2SectionCorrection::correctInternalBuffer(): Exiting due to "
                       "uncorrectable error in internal buffer.");
            }
        }
    }
}

// This function queues up the processed output sections and keeps track of the statistics
void F2SectionCorrection::outputSections()
{
    // TODO: There should probably be some checks here to ensure the internal buffer
    // is in a good state before outputting the sections

    // Pop
    F2Section section = m_internalBuffer.dequeue();

    // Push
    m_totalSections++;
    m_outputBuffer.enqueue(section);

    // Statistics generation...
    quint8 trackNumber = section.metadata.trackNumber();
    SectionTime sectionTime = section.metadata.sectionTime();
    SectionTime absoluteTime = section.metadata.absoluteSectionTime();

    // Set the absolute start and end times
    if (absoluteTime <= m_absoluteStartTime)
        m_absoluteStartTime = absoluteTime;
    if (absoluteTime > m_absoluteEndTime)
        m_absoluteEndTime = absoluteTime;

    // Do we have a new track?
    if (!m_trackNumbers.contains(trackNumber)) {
        // Append the new track to the statistics
        m_trackNumbers.append(trackNumber);
        m_trackStartTimes.append(sectionTime);
        m_trackEndTimes.append(sectionTime);

        if (m_showDebug)
            qDebug() << "F2SectionCorrection::outputSections(): New track" << trackNumber
                    << "detected with start time" << sectionTime.toString();
        if (trackNumber == 0) {
            if (section.metadata.sectionType().type() == SectionType::LeadIn) {
                // This is a lead-in track
                if (m_showDebug)
                    qDebug() << "F2SectionCorrection::outputSections(): LeadIn track detected "
                                "with start time"
                            << sectionTime.toString();
            } else if (section.metadata.sectionType().type() == SectionType::LeadOut) {
                // This is a lead-out track
                if (m_showDebug)
                    qDebug() << "F2SectionCorrection::outputSections(): LeadOut track detected "
                                "with start time"
                            << sectionTime.toString();
            } else if (section.metadata.sectionType().type() == SectionType::UserData) {
                // This is a user data track
                if (m_showDebug)
                    qDebug() << "F2SectionCorrection::outputSections(): UserData track detected "
                                "with start time"
                            << sectionTime.toString();
            } else {
                // This is an unknown track
                if (m_showDebug)
                    qDebug() << "F2SectionCorrection::outputSections(): UNKNOWN track detected "
                                "with start time"
                            << sectionTime.toString();
            }
            qFatal("F2SectionCorrection::outputSections(): Exiting due to track 0 detected in "
                "output sections.");
        }
    } else {
        // Update the end time for the existing track
        int index = m_trackNumbers.indexOf(trackNumber);
        if (sectionTime < m_trackStartTimes[index])
            m_trackStartTimes[index] = sectionTime;
        if (sectionTime >= m_trackEndTimes[index])
            m_trackEndTimes[index] = sectionTime;
    }
}

void F2SectionCorrection::flush()
{
    // Flush the internal buffer

    // TODO: What about any remaining invalid sections in the internal buffer?

    while (!m_internalBuffer.isEmpty()) {
        outputSections();
    }
}

void F2SectionCorrection::showStatistics() const
{
    qInfo() << "F2 Section Metadata Correction statistics:";
    qInfo() << "  F2 Sections:";
    qInfo().nospace() << "    Total: " << m_totalSections << " (" << m_totalSections * 98 << " F2)";
    qInfo() << "    Corrected:" << m_correctedSections;
    qInfo() << "    Uncorrectable:" << m_uncorrectableSections;
    qInfo() << "    Pre-Leadin:" << m_preLeadinSections;
    qInfo() << "    Missing:" << m_missingSections;
    qInfo() << "    Out of order:" << m_outOfOrderSections;

    qInfo() << "  Absolute Time:";
    qInfo().noquote() << "    Start time:" << m_absoluteStartTime.toString();
    qInfo().noquote() << "    End time:" << m_absoluteEndTime.toString();
    qInfo().noquote() << "    Duration:" << (m_absoluteEndTime - m_absoluteStartTime).toString();

    // Show each track in order of appearance
    for (int i = 0; i < m_trackNumbers.size(); i++) {
        qInfo().nospace() << "  Track " << m_trackNumbers[i] << ":";
        qInfo().noquote() << "    Start time:" << m_trackStartTimes[i].toString();
        qInfo().noquote() << "    End time:" << m_trackEndTimes[i].toString();
        qInfo().noquote() << "    Duration:"
                          << (m_trackEndTimes[i] - m_trackStartTimes[i]).toString();
    }
}