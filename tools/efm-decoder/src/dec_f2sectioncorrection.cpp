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
    last_good_section = F2Section();
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
        F2Section f2_section_current = input_buffer.dequeue();
        
        // Do we have a last known good section?
        if (!has_last_good_section) {
            waiting_for_first_valid_section(f2_section_current);
        } else {
            waiting_for_section(f2_section_current);
        }
    }
}

void F2SectionCorrection::waiting_for_first_valid_section(F2Section& f2_section_current) {
    // Is the metadata of the current section valid?
    if (f2_section_current.metadata.is_valid()) {
        // We have a valid section
        last_good_section = f2_section_current;
        has_last_good_section = true;
        if (show_debug) qDebug() << "F2SectionCorrection::waiting_for_first_valid_section(): Found first valid section with abs time" << f2_section_current.metadata.get_absolute_section_time().to_string();

        // Output the first valid section
        output_section(f2_section_current);
        total_sections++;
    } else {
        // We have an invalid section
        // We can't do anything with this section
        uncorrectable_sections++;
        if (show_debug) qDebug() << "F2SectionCorrection::waiting_for_first_valid_section(): Got invalid section whilst waiting fo the first valid (discarded).";
    }
}

void F2SectionCorrection::waiting_for_section(F2Section& f2_section_current) {
    // We have a last know good section
    // Is the metadata of the current section valid?
    if (f2_section_current.metadata.is_valid()) {
        // We have a section with valid metadata
        // Does it match the last known good section (plus the additional windowed sections?)
        SectionTime expected_absolute_time = last_good_section.metadata.get_absolute_section_time() + window.size() + 1;

        if (f2_section_current.metadata.get_absolute_section_time() == expected_absolute_time) {
            got_valid_section_correct_time(f2_section_current, expected_absolute_time);
        } else {
            got_valid_section_incorrect_time(f2_section_current, expected_absolute_time);
        }
    } else {
        got_invalid_section(f2_section_current);
    }
}

void F2SectionCorrection::got_valid_section_correct_time(F2Section& f2_section_current, SectionTime expected_absolute_time) {
    // Are there any sections in the window?
    if (window.size() > 0) {
        if (show_debug) qDebug() << "F2SectionCorrection::got_valid_section_correct_time(): Got valid section with absolute time" <<
            f2_section_current.metadata.get_absolute_section_time().to_string() << "- correcting windowed sections.";
        // The section has the correct time, so we can correct the windowed sections
        // based on the last known good and the current section
        SectionTime section_absolute_time = last_good_section.metadata.get_absolute_section_time() + 1;
        for (int i = 0; i < window.size(); i++) {
            F2Section window_section = window.dequeue();

            // Now we have to correct the metadata including the absolute time, track number, q-mode, track time, etc.
            SectionMetadata corrected_metadata = last_good_section.metadata; // Start with the last know good metadata
            corrected_metadata.set_absolute_section_time(corrected_metadata.get_absolute_section_time() + (1 + i)); // Set the absolute time

            // Is the track number the same?
            if (corrected_metadata.get_track_number() != last_good_section.metadata.get_track_number()) {
                // The track number has changed, so we have to update the track number and the track start time
                qFatal("F2SectionCorrection::got_valid_section_correct_time(): Track number change not supported.");
            } else {
                // The track number is the same, so we can just update the track time
                corrected_metadata.set_section_time(corrected_metadata.get_section_time() + (1 + i));
            }

            // Correct the section
            window_section.metadata = corrected_metadata;
            
            // Put the window in the output buffer
            output_section(window_section);
            total_sections++;
            corrected_sections++;

            if (show_debug) qDebug().nospace().noquote() << "F2SectionCorrection::got_valid_section_correct_time(): Corrected windowed section " << i <<
                " with absolute time " << section_absolute_time.to_string() <<
                ", track number " << corrected_metadata.get_track_number() <<
                " and track time " << corrected_metadata.get_section_time().to_string();
            section_absolute_time = section_absolute_time + 1;
        }

        // Now we can output the current section
        output_section(f2_section_current);
        last_good_section = f2_section_current;
        total_sections++;
    } else {
        // Nothing in the window, so we can simply output the current section
        // and make it the last known good section
        output_section(f2_section_current);
        last_good_section = f2_section_current;
        total_sections++;
        if (show_debug) qDebug() << "F2SectionCorrection::got_valid_section_correct_time(): Valid section with absolute time" << f2_section_current.metadata.get_absolute_section_time().to_string();
    }
}

void F2SectionCorrection::got_valid_section_incorrect_time(F2Section& f2_section_current, SectionTime expected_absolute_time) {
    // The current section's valid metadata doesn't match the expected time, so it's
    // likely we have a skip in the continuity of the sections.  We can't correct this
    // so we have to drop the window, flush the CIRC delay lines and start again from
    // the current section.
    if (show_debug) qDebug() << "F2SectionCorrection::got_valid_section_incorrect_time(): Got section with valid metadata but the expected absolute time was" <<
        expected_absolute_time.to_string() << "but the actual metadata was" << f2_section_current.metadata.get_absolute_section_time().to_string();

    // This can happen during lead-in as the original media settles.  Since we have
    // valid metadata, we can see if the track number is zero (lead-in)...
    if (f2_section_current.metadata.get_track_number() == 0) {
        if (f2_section_current.metadata.get_absolute_section_time() < last_good_section.metadata.get_absolute_section_time()) {
            last_good_section = f2_section_current;
            if (show_debug) qDebug() << "F2SectionCorrection::got_valid_section_incorrect_time(): Lead-in section with absolute time" << f2_section_current.metadata.get_absolute_section_time().to_string();
            output_section(f2_section_current);
        }
    } else {
        // We have a track number, so this is a real issue due to skipping
        if (show_debug) qDebug() << "F2SectionCorrection::got_valid_section_incorrect_time(): Track number is" << f2_section_current.metadata.get_track_number() <<
            "and track time is" << f2_section_current.metadata.get_section_time().to_string();
        qFatal("F2SectionCorrection::got_valid_section_incorrect_time(): Section time mismatch, can't correct.");
    }
}

void F2SectionCorrection::got_invalid_section(F2Section& f2_section_current) {
    // If the metadata is invalid, we can only put it in the 
    // window and try to correct it later

    // Is there space in the window?
    if (window.size() < window_size) {
        // Add the section to the window
        window.enqueue(f2_section_current);
        if (show_debug) qDebug().nospace() << "F2SectionCorrection::got_invalid_section(): Added section (with invalid metadata) to window (window size is " << window.size() << ")";
    } else {
        // The window is full, we can't correct this section
        qFatal("F2SectionCorrection::got_invalid_section(): Window is full, can't correct section.");

        uncorrectable_sections++; // Dummy for now
    }
}

// This function queues up the processed output sections and keeps track of the statistics
void F2SectionCorrection::output_section(F2Section section) {
    // Add the section to the output buffer
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