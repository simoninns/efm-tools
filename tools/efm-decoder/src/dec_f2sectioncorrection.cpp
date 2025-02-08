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

F2SectionCorrection::F2SectionCorrection() {
    has_last_good_section = false;
    window_size = 6;

    total_sections = 0;
    corrected_sections = 0;
    uncorrectable_sections = 0;

    // Time statistics
    absolute_start_time = SectionTime(59,59,74);
    absolute_end_time = SectionTime(0,0,0);
}

void F2SectionCorrection::push_section(F2Section data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

F2Section F2SectionCorrection::pop_section() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool F2SectionCorrection::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void F2SectionCorrection::process_queue() {
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        // Dequeue the next section
        F2Section current_section = input_buffer.dequeue();
        bool is_current_section_valid = current_section.metadata.is_valid();
        total_sections++;

        // If we haven't see a good section yet, try to use the first section
        if (!has_last_good_section) {
            // Is the metadata valid in the current section?
            if (is_current_section_valid) {
                // Use this section to start
                has_last_good_section = true;
                last_good_section = current_section;
                output_section(current_section);
                if (show_debug) qDebug() << "F2SectionCorrection::process_queue - First good section:" <<
                    current_section.metadata.get_absolute_section_time().to_string();
            } else {
                // If the first section is bad, add it to the window
                window.enqueue(current_section);
                if (window.size() >= window_size) {
                    flush_window();
                }
                if (show_debug) qDebug() << "F2SectionCorrection::process_queue - First bad section:" <<
                    current_section.metadata.get_absolute_section_time().to_string();
            }
        } else {
            // We have a last known good section
            if (is_current_section_valid && is_section_valid(current_section, last_good_section, window.size())) {
                // If the incoming section is valid...
                if (!window.isEmpty()) {
                    // …and we have a buffered set of sections waiting for correction,
                    // add the current section as the "good" end marker, then correct.
                    window.enqueue(current_section);
                    correct_and_flush_window();
                } else {
                    // No buffered sections; just output the section as is.
                    output_section(current_section);
                    last_good_section = current_section;
                }
            } else {
                if (!is_current_section_valid && show_debug) qDebug() << "F2SectionCorrection::process_queue - Section is invalid - Invalid metadata (bad CRC)";
                //qDebug() << "Sections in window between last good section and current section:" << window.size();

                if (!is_section_valid(current_section, last_good_section, window.size()) && show_debug) qDebug().noquote() << "F2SectionCorrection::process_queue - Section is invalid - current:" <<
                    current_section.metadata.get_absolute_section_time().to_string() << "last good:" <<
                    last_good_section.metadata.get_absolute_section_time().to_string();

                // The section is either marked invalid or fails our validity check.
                // Add it to our sliding window.
                window.enqueue(current_section);
                // If we’ve reached our maximum window size, flush the window.
                if (window.size() >= window_size)
                    flush_window();
            }
        }
    }
}

bool F2SectionCorrection::is_section_valid(F2Section current, F2Section last_good, uint32_t window_size) {
    // For the current section to be good, it must be one absolute frame
    // ahead of the last good section and have valid metadata (CRC passed)
    SectionMetadata current_metadata = current.metadata;
    SectionMetadata last_good_metadata = last_good.metadata;

    // If the current section's metadata is invalid, it's invalid.
    if (!current_metadata.is_valid()) {
        if (show_debug) qDebug() << "F2SectionCorrection::is_section_valid - Section is invalid - Invalid metadata (bad CRC)";
        return false;
    }

    // If the current section is windows.size() frame(s) ahead of the last good section, it's valid.
    // to avoid inter-track issues we use the absolute frame time.
    if (current_metadata.get_absolute_section_time() == last_good_metadata.get_absolute_section_time() + SectionTime(0,0,window_size+1)) {
        return true;
    }

    // Otherwise, the section is invalid
    if (show_debug) qDebug().noquote() << "F2SectionCorrection::is_section_valid - Section is invalid - current:" <<
        current_metadata.get_absolute_section_time().to_string() << "last good:" <<
        last_good_metadata.get_absolute_section_time().to_string();
    return false;
}

// Correct sections in the window using linear interpolation between
// last_good_section (the good section before the window) and the last
// section in the window (which should be valid). After correction,
// all sections are output.
void F2SectionCorrection::correct_and_flush_window() {
    // The last section in the window is our “next good” section.
    F2Section next_good_section = window.last();
    SectionMetadata next_good_metadata = next_good_section.metadata;
    SectionMetadata last_good_metadata = last_good_section.metadata;  
    int total_buffered = window.size(); // number of frames in the window

    // Determine the total gap in track time and track numbers.
    int32_t deltaTime = next_good_metadata.get_absolute_section_time().get_time_in_frames() - last_good_metadata.get_absolute_section_time().get_time_in_frames();
    uint8_t deltaTrack = next_good_metadata.get_track_number() - last_good_metadata.get_track_number();

    // We now have last_good_section as “position 0” and next_good_section as
    // “position totalBuffered”. We will update each buffered section
    // (positions 1..totalBuffered) to have interpolated values.
    QList<F2Section> temp_list;
    while (!window.isEmpty())
        temp_list.append(window.dequeue());

    // Note: we update all sections in the window including the last one.
    // (The last frame is already valid, but we reassign its values so that
    // everything is consistent.)
    for (int index = 0; index < temp_list.size(); ++index) {
        // Compute the “position” of this section between the last good and the next good.
        // (Position i+1 out of totalBuffered intervals.)
        int pos = index + 1;

        F2Section corrected = temp_list[index];
        SectionMetadata corrected_metadata = corrected.metadata;

        // Linear interpolation for absolute track time and track number.
        int32_t corrected_abs_time = last_good_metadata.get_absolute_section_time().get_time_in_frames() + (deltaTime * pos) / total_buffered;
        int32_t corrected_time = last_good_metadata.get_section_time().get_time_in_frames() + (deltaTime * pos) / total_buffered;
        uint8_t corrected_track = last_good_metadata.get_track_number() + (deltaTrack * pos) / total_buffered;

        SectionTime abs_time;
        abs_time.set_time_in_frames(corrected_abs_time);

        SectionTime trk_time;
        trk_time.set_time_in_frames(corrected_time);

        if (corrected_metadata.get_absolute_section_time().get_time_in_frames() != abs_time.get_time_in_frames()) {
            if (show_debug) qDebug().noquote() << "F2SectionCorrection::correct_and_flush_window - Corrected frame abs time to" << abs_time.to_string() <<
            "/ track time to" << trk_time.to_string() << "/ track number to" << corrected_track;
        }

        // Set the corrected ABS time and track number in the section
        // We also copy the frame type from the last good section.
        corrected.metadata.set_section_type(last_good_metadata.get_section_type());
        corrected.metadata.set_track_number(corrected_track);
        corrected.metadata.set_absolute_section_time(abs_time);
        corrected.metadata.set_section_time(trk_time);

        output_section(corrected);
        corrected_sections++;

        // Update the last good frame as we go.
        last_good_section = corrected;
    }
}

// Flush the window without any correction - output all frames in order.
void F2SectionCorrection::flush_window() {
    if (show_debug) qDebug() << "F2SectionCorrection::flush_window - Flushing window without correction";
    while (!window.isEmpty()) {
        F2Section section = window.dequeue();
        uncorrectable_sections++;
        output_section(section);
        if (section.metadata.is_valid())
            last_good_section = section;
    }
}

// Output the F2 section to the output buffer
// and count the number of tracks, durations, etc.
void F2SectionCorrection::output_section(F2Section section) {
    // Add the F2Section to the output buffer
    output_buffer.enqueue(section);

    // Statistics generation...
    uint8_t track_number = section.metadata.get_track_number();
    SectionTime section_time = section.metadata.get_section_time();
    SectionTime absolute_time = section.metadata.get_absolute_section_time();

    // Set the absolute start and end times
    if (absolute_time <= absolute_start_time) absolute_start_time = absolute_time;
    if (absolute_time > absolute_end_time) absolute_end_time = absolute_time;

    // Do we have a new track?
    if (!track_numbers.contains(track_number)) {
        // Append the new track to the statistics
        track_numbers.append(track_number);
        track_start_times.append(section_time);
        track_end_times.append(section_time);
    } else {
        // Update the end time for the existing track
        int index = track_numbers.indexOf(track_number);
        if (section_time < track_start_times[index]) track_start_times[index] = section_time;
        if (section_time >= track_end_times[index]) track_end_times[index] = section_time;
    }

    // Save the current track and frame time
    current_track = track_number;
    current_section_time = section_time;
    current_absolute_time = absolute_time;
}

void F2SectionCorrection::show_statistics() {
    qInfo() << "F2 Section Metadata Correction statistics:";
    qInfo() << "  F2 Sections:";
    qInfo().nospace() << "    Total: " << total_sections << " (" << total_sections * 98 << " F2)";
    qInfo() << "    Corrected:" << corrected_sections;
    qInfo() << "    Uncorrectable:" << uncorrectable_sections;

    qInfo() << "  Absolute Time:";
    qInfo().noquote() << "    Start time:" << absolute_start_time.to_string();
    qInfo().noquote() << "    End time:" << absolute_end_time.to_string();
    qInfo().noquote() << "    Duration:" << (absolute_end_time - absolute_start_time).to_string();

    // Show each track in order of appearance
    for (int i = 0; i < track_numbers.size(); i++) {
        qInfo().nospace() << "  Track " << track_numbers[i] << ":";
        qInfo().noquote() << "    Start time:" << track_start_times[i].to_string();
        qInfo().noquote() << "    End time:" << track_end_times[i].to_string();
        qInfo().noquote() << "    Duration:" << (track_end_times[i] - track_start_times[i]).to_string();
    }

    // Show the total duration of all tracks
    SectionTime total_duration = SectionTime(0,0,0);
    for (int i = 0; i < track_numbers.size(); i++) {
        total_duration = total_duration + (track_end_times[i] - track_start_times[i]);
    }
    if (track_numbers.size() > 1) qInfo().noquote() << "  Total duration of" << track_numbers.size() << "tracks:" << total_duration.to_string();
    else qInfo().noquote() << "  Total duration of" << track_numbers.size() << "track:" << total_duration.to_string();
}