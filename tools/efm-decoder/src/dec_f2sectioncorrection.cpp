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

F2SectionCorrection::F2SectionCorrection()
{
    leadin_complete = false;

    // The maximum gap size is the maximum number of missing sections that can be corrected
    // by examining the time difference between the first and last valid sections surrounding
    // the gap
    maximum_gap_size = 3;

    // The maximum internal buffer size is the maximum number of sections that can be stored
    // in the internal buffer before outputting them.  This effects the maximum time range
    // that out of order sections can be corrected in.
    //
    // Note: Each section is 1/75 of a second, so a 5 second buffer is 375 sections
    maximum_internal_buffer_size = 375;

    // Note: The reason for the internal buffer is to allow for skips back in time, so that
    // we can correct for missing sections.  However, this check is not implemented yet.

    total_sections = 0;
    corrected_sections = 0;
    uncorrectable_sections = 0;
    pre_leadin_sections = 0;
    missing_sections = 0;

    // Time statistics
    absolute_start_time = SectionTime(59, 59, 74);
    absolute_end_time = SectionTime(0, 0, 0);
}

void F2SectionCorrection::push_section(F2Section data)
{
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

F2Section F2SectionCorrection::pop_section()
{
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool F2SectionCorrection::is_ready() const
{
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void F2SectionCorrection::process_queue()
{
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        // Dequeue the next section
        F2Section f2_section = input_buffer.dequeue();

        // Do we have a last known good section?
        if (!leadin_complete) {
            wait_for_input_to_settle(f2_section);
        } else {
            waiting_for_section(f2_section);
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
void F2SectionCorrection::wait_for_input_to_settle(F2Section &f2_section)
{
    // Does the current section have valid metadata?
    if (f2_section.metadata.is_valid()) {
        // Do we have any sections in the leadin buffer?
        if (leadin_buffer.size() > 0) {
            // Ensure that the current section's time-stamp is one greater than the last section in
            // the leadin buffer
            SectionTime expected_absolute_time =
                    leadin_buffer.last().metadata.get_absolute_section_time() + 1;
            if (f2_section.metadata.get_absolute_section_time() == expected_absolute_time) {
                // Add the new section to the leadin buffer
                leadin_buffer.enqueue(f2_section);
                if (show_debug)
                    qDebug() << "F2SectionCorrection::wait_for_input_to_settle(): Added valid "
                                "section to leadin buffer with absolute time"
                             << f2_section.metadata.get_absolute_section_time().to_string();

                // Do we have 5 valid, contigious sections in the leadin buffer?
                if (leadin_buffer.size() >= 5) {
                    leadin_complete = true;

                    // Feed the leadin buffer into the section correction process
                    if (show_debug)
                        qDebug() << "F2SectionCorrection::wait_for_input_to_settle(): Leadin "
                                    "buffer complete, pushing collected sections for processing.";
                    while (!leadin_buffer.isEmpty()) {
                        F2Section leadin_section = leadin_buffer.dequeue();
                        waiting_for_section(leadin_section);
                    }
                }
            } else {
                // The current section's time-stamp is not one greater than the last section in the
                // leadin buffer This invalidates the whole leadin buffer
                pre_leadin_sections += leadin_buffer.size() + 1;
                leadin_buffer.clear();
                if (show_debug)
                    qDebug() << "F2SectionCorrection::wait_for_input_to_settle(): Got section with "
                                "invalid absolute time whilst waiting for input to settle (lead in "
                                "buffer discarded).";
            }
        } else {
            // The leadin buffer is empty, so we can just add the section
            leadin_buffer.enqueue(f2_section);
            if (show_debug)
                qDebug() << "F2SectionCorrection::wait_for_input_to_settle(): Added section to "
                            "lead in buffer with absolute time"
                         << f2_section.metadata.get_absolute_section_time().to_string();
        }
    } else {
        // The current section doesn't have valid metadata
        // This invalidates the whole buffer
        pre_leadin_sections += leadin_buffer.size() + 1;
        leadin_buffer.clear();
        if (show_debug)
            qDebug() << "F2SectionCorrection::wait_for_input_to_settle(): Got invalid metadata "
                        "section whilst waiting for input to settle (lead in buffer discarded).";
    }
}

void F2SectionCorrection::waiting_for_section(F2Section &f2_section)
{
    // Check that this isn't the first section in the internal buffer (as we can't calculate
    /// the expected time for the first section)
    if (internal_buffer.size() == 0) {
        if (f2_section.metadata.is_valid()) {
            // The internal buffer is empty, so we can just add the section
            internal_buffer.enqueue(f2_section);
            if (show_debug)
                qDebug() << "F2SectionCorrection::waiting_for_section(): Added section to internal "
                            "buffer with absolute time"
                         << f2_section.metadata.get_absolute_section_time().to_string();
            return;
        } else {
            qDebug() << "F2SectionCorrection::waiting_for_section(): Got invalid metadata section "
                        "whilst waiting for first section.";
            return;
        }
    }

    // What is the next expected section time?
    SectionTime expected_absolute_time = get_expected_absolute_time();

    // Push the section into the internal buffer
    if (show_debug) {
        if (f2_section.metadata.is_valid()) {
            // if (show_debug) qDebug() << "F2SectionCorrection::waiting_for_section(): Expected
            // absolute time is" << expected_absolute_time.to_string() << "actual absolute time is"
            // << f2_section.metadata.get_absolute_section_time().to_string();
        } else {
            if (show_debug)
                qDebug() << "F2SectionCorrection::waiting_for_section(): Pushing F2 Section with "
                            "invalid metadata CRC to internal buffer.  Expected absolute time is"
                         << expected_absolute_time.to_string();
        }
    }

    // Check for a missing section...
    if (f2_section.metadata.is_valid()
        && f2_section.metadata.get_absolute_section_time() != expected_absolute_time) {
        // We have a missing section
        missing_sections++;
        if (show_debug)
            qWarning() << "F2SectionCorrection::waiting_for_section(): Missing section detected, "
                          "expected absolute time is"
                       << expected_absolute_time.to_string() << "actual absolute time is"
                       << f2_section.metadata.get_absolute_section_time().to_string();

        // We have to insert a dummy section into the internal buffer or this
        // will throw off the correction process due to the delay lines
        //
        // It's important that all the metadata is correct otherwise track numbers and so on
        // will be incorrect
        F2Section missing_section;

        // Copy the metadata from the next section as a good default
        missing_section.metadata = f2_section.metadata;

        missing_section.metadata.set_absolute_section_time(expected_absolute_time);
        missing_section.metadata.set_valid(true);

        // To-do: Perhaps this could be improved if spanning a track boundary?
        // might not be required though...
        missing_section.metadata.set_section_type(f2_section.metadata.get_section_type());
        missing_section.metadata.set_track_number(f2_section.metadata.get_track_number());
        missing_section.metadata.set_section_time(f2_section.metadata.get_section_time() - 1);

        // Push 98 error frames in to the missing section
        for (int i = 0; i < 98; ++i) {
            F2Frame error_frame;
            error_frame.set_data(QVector<uint8_t>(32, 0x00));
            error_frame.set_error_data(QVector<uint8_t>(32, 0x01));
            missing_section.push_frame(error_frame);
        }

        // Push it into the internal buffer
        internal_buffer.enqueue(missing_section);
    }

    internal_buffer.enqueue(f2_section);
    correct_internal_buffer();
    if (internal_buffer.size() > maximum_internal_buffer_size)
        output_sections();
}

// Figure out what absolute time is expected for the next section by looking at the internal buffer
SectionTime F2SectionCorrection::get_expected_absolute_time()
{
    SectionTime expected_absolute_time = SectionTime(0, 0, 0);

    // Find the last valid section in the internal buffer
    for (int i = internal_buffer.size() - 1; i >= 0; --i) {
        if (internal_buffer[i].metadata.is_valid()) {
            // The expected time is the time of the last valid section in the internal buffer plus
            // any following sections until the end of the buffer
            expected_absolute_time = internal_buffer[i].metadata.get_absolute_section_time()
                    + (internal_buffer.size() - i);
            break;
        }
    }

    return expected_absolute_time;
}

// This function corrects any missing sections in the internal buffer (if possible)
void F2SectionCorrection::correct_internal_buffer()
{
    // Sanity check - there cannot be an invalid section at the start of the buffer
    if (internal_buffer.size() > 0 && !internal_buffer.first().metadata.is_valid()) {
        if (show_debug)
            qDebug() << "F2SectionCorrection::correct_internal_buffer(): Invalid section at start "
                        "of internal buffer!";
        qFatal("F2SectionCorrection::correct_internal_buffer(): Exiting due to invalid section at "
               "start of internal buffer.");
        return;
    }

    // Sanity check - there cannot be an invalid section at the end of the buffer
    if (internal_buffer.size() > 0 && !internal_buffer.last().metadata.is_valid()) {
        if (show_debug)
            qDebug() << "F2SectionCorrection::correct_internal_buffer(): Invalid section at end of "
                        "internal buffer - cannot correct internal buffer until valid section is "
                        "pushed";
        return;
    }

    // Sanity check - there must be at least 3 sections in the buffer
    if (internal_buffer.size() < 3) {
        if (show_debug)
            qDebug() << "F2SectionCorrection::correct_internal_buffer(): Not enough sections in "
                        "internal buffer to correct.";
        return;
    }

    // Starting from the second section in the buffer, look for an invalid section
    for (int index = 1; index < internal_buffer.size(); ++index) {
        // Is the current section invalid?
        int32_t error_start = -1;
        int32_t error_end = -1;

        if (!internal_buffer[index].metadata.is_valid()) {
            error_start = index - 1; // This is the "last known good" section

            // Count how many invalid sections there are before the next valid section
            for (int i = index + 1; i < internal_buffer.size(); ++i) {
                if (internal_buffer[i].metadata.is_valid()) {
                    error_end = i;
                    break;
                }
            }

            int32_t gap_length = error_end - error_start - 1;
            int32_t time_difference =
                    internal_buffer[error_end].metadata.get_absolute_section_time().get_frames()
                    - internal_buffer[error_start].metadata.get_absolute_section_time().get_frames()
                    - 1;

            if (show_debug)
                qDebug().nospace().noquote()
                        << "F2SectionCorrection::correct_internal_buffer(): Error start position "
                        << error_start << " ("
                        << internal_buffer[error_start]
                                   .metadata.get_absolute_section_time()
                                   .to_string()
                        << ") Error end position " << error_end << " ("
                        << internal_buffer[error_end]
                                   .metadata.get_absolute_section_time()
                                   .to_string()
                        << ") gap length is " << gap_length << " time difference is "
                        << time_difference;

            // Is the gap length below the allowed maximum?
            if (gap_length > maximum_gap_size) {
                // The gap is too large to correct
                qFatal("F2SectionCorrection::correct_internal_buffer(): Exiting due to gap too "
                       "large in internal buffer.");
            }

            // Can we correct the error?
            if (gap_length == time_difference) {
                // We can correct the error
                for (int i = error_start + 1; i < error_end; ++i) {
                    // Firstly copy the metadata from the last known good section to ensure good
                    // defaults
                    internal_buffer[i].metadata = internal_buffer[error_start].metadata;

                    // Now set the absolute time for the section
                    SectionTime expected_time =
                            internal_buffer[error_start].metadata.get_absolute_section_time()
                            + (i - error_start);
                    internal_buffer[i].metadata.set_absolute_section_time(expected_time);

                    // Is the track number the same at the start and end of the gap?
                    if (internal_buffer[error_start].metadata.get_track_number()
                        != internal_buffer[error_end].metadata.get_track_number()) {
                        if (show_debug)
                            qDebug() << "F2SectionCorrection::correct_internal_buffer(): Gap "
                                        "starts on track"
                                     << internal_buffer[error_start].metadata.get_track_number()
                                     << "and ends on track"
                                     << internal_buffer[error_end].metadata.get_track_number();

                        // Firstly, we have to figure out which track the error section in on.  We
                        // can do this by looking at the section time of the error_end section and
                        // calculating the current section time from it.  If the time is positive
                        // the error section is in the same track as error_end, otherwise it is in
                        // the same track as error_start
                        SectionTime current_time =
                                internal_buffer[error_end].metadata.get_section_time()
                                - (error_end - i);

                        // Now we can set the track number and correct the section time
                        if (current_time.get_frames() >= 0) {
                            internal_buffer[i].metadata.set_track_number(
                                    internal_buffer[error_end].metadata.get_track_number());
                            internal_buffer[i].metadata.set_section_time(
                                    internal_buffer[error_end].metadata.get_section_time()
                                    - (error_end - i));
                        } else {
                            internal_buffer[i].metadata.set_track_number(
                                    internal_buffer[error_start].metadata.get_track_number());
                            internal_buffer[i].metadata.set_section_time(
                                    internal_buffer[error_start].metadata.get_section_time()
                                    + (i - error_start));
                        }

                        // Adding a qFatal here as this functionality is untested
                        // (I haven't found any examples of this in the test data)
                        qFatal("F2SectionCorrection::correct_internal_buffer(): Exiting due to "
                               "track change in internal buffer - untested functionality - please "
                               "confirm!");
                    } else {
                        // The track number is the same, so we can correct the track number by just
                        // copying it
                        internal_buffer[i].metadata.set_track_number(
                                internal_buffer[error_start].metadata.get_track_number());

                        // Same thing for the section time
                        SectionTime expected_section_time =
                                internal_buffer[error_start].metadata.get_section_time()
                                + (i - error_start);
                    }

                    // Mark the corrected metadata as valid
                    internal_buffer[i].metadata.set_valid(true);

                    corrected_sections++;
                    if (show_debug)
                        qDebug().noquote().nospace()
                                << "F2SectionCorrection::correct_internal_buffer(): Corrected "
                                   "section "
                                << i << " with absolute time "
                                << internal_buffer[i]
                                           .metadata.get_absolute_section_time()
                                           .to_string()
                                << ", Track number "
                                << internal_buffer[i].metadata.get_track_number()
                                << " and track time "
                                << internal_buffer[i].metadata.get_section_time().to_string();
                }
            } else {
                // We cannot correct the error
                qFatal("F2SectionCorrection::correct_internal_buffer(): Exiting due to "
                       "uncorrectable error in internal buffer.");
            }
        }
    }
}

// This function queues up the processed output sections and keeps track of the statistics
void F2SectionCorrection::output_sections()
{
    // Pop
    F2Section section = internal_buffer.dequeue();

    // Push
    total_sections++;
    output_buffer.enqueue(section);

    // Statistics generation...
    uint8_t track_number = section.metadata.get_track_number();
    SectionTime section_time = section.metadata.get_section_time();
    SectionTime absolute_time = section.metadata.get_absolute_section_time();

    // Set the absolute start and end times
    if (absolute_time <= absolute_start_time)
        absolute_start_time = absolute_time;
    if (absolute_time > absolute_end_time)
        absolute_end_time = absolute_time;

    // Do we have a new track?
    if (!track_numbers.contains(track_number)) {
        // Append the new track to the statistics
        track_numbers.append(track_number);
        track_start_times.append(section_time);
        track_end_times.append(section_time);

        if (show_debug)
            qDebug() << "F2SectionCorrection::output_sections(): New track" << track_number
                     << "detected with start time" << section_time.to_string();
        if (track_number == 0) {
            if (section.metadata.get_section_type() == SectionType::LEAD_IN) {
                // This is a lead-in track
                if (show_debug)
                    qDebug() << "F2SectionCorrection::output_sections(): LEAD_IN track detected "
                                "with start time"
                             << section_time.to_string();
            } else if (section.metadata.get_section_type() == SectionType::LEAD_OUT) {
                // This is a lead-out track
                if (show_debug)
                    qDebug() << "F2SectionCorrection::output_sections(): LEAD_OUT track detected "
                                "with start time"
                             << section_time.to_string();
            } else if (section.metadata.get_section_type() == SectionType::USER_DATA) {
                // This is a user data track
                if (show_debug)
                    qDebug() << "F2SectionCorrection::output_sections(): USER_DATA track detected "
                                "with start time"
                             << section_time.to_string();
            } else {
                // This is an unknown track
                if (show_debug)
                    qDebug() << "F2SectionCorrection::output_sections(): UNKNOWN track detected "
                                "with start time"
                             << section_time.to_string();
            }
            qFatal("F2SectionCorrection::output_sections(): Exiting due to track 0 detected in "
                   "output sections.");
        }
    } else {
        // Update the end time for the existing track
        int index = track_numbers.indexOf(track_number);
        if (section_time < track_start_times[index])
            track_start_times[index] = section_time;
        if (section_time >= track_end_times[index])
            track_end_times[index] = section_time;
    }
}

void F2SectionCorrection::flush()
{
    // Flush the internal buffer

    // TODO: What about any remaining invalid sections in the internal buffer?

    while (!internal_buffer.isEmpty()) {
        output_sections();
    }
}

void F2SectionCorrection::show_statistics()
{
    qInfo() << "F2 Section Metadata Correction statistics:";
    qInfo() << "  F2 Sections:";
    qInfo().nospace() << "    Total: " << total_sections << " (" << total_sections * 98 << " F2)";
    qInfo() << "    Corrected:" << corrected_sections;
    qInfo() << "    Uncorrectable:" << uncorrectable_sections;
    qInfo() << "    Pre-Leadin:" << pre_leadin_sections;
    qInfo() << "    Missing:" << missing_sections;

    qInfo() << "  Absolute Time:";
    qInfo().noquote() << "    Start time:" << absolute_start_time.to_string();
    qInfo().noquote() << "    End time:" << absolute_end_time.to_string();
    qInfo().noquote() << "    Duration:" << (absolute_end_time - absolute_start_time).to_string();

    // Show each track in order of appearance
    for (int i = 0; i < track_numbers.size(); i++) {
        qInfo().nospace() << "  Track " << track_numbers[i] << ":";
        qInfo().noquote() << "    Start time:" << track_start_times[i].to_string();
        qInfo().noquote() << "    End time:" << track_end_times[i].to_string();
        qInfo().noquote() << "    Duration:"
                          << (track_end_times[i] - track_start_times[i]).to_string();
    }
}