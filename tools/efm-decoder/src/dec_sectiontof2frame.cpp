/************************************************************************

    dec_sectiontof2frame.cpp

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

#include "dec_sectiontof2frame.h"

SectionToF2Frame::SectionToF2Frame() {
    has_last_good_section = false;
    window_size = 6;

    total_sections = 0;
    corrected_sections = 0;
    uncorrectable_sections = 0;
    total_f2_frames = 0;

    // Time statistics
    absolute_start_time = FrameTime(59,59,74);
    absolute_end_time = FrameTime(0,0,0);
}

void SectionToF2Frame::push_frame(Section data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

QVector<F2Frame> SectionToF2Frame::pop_frames() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool SectionToF2Frame::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void SectionToF2Frame::process_queue() {
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        // Dequeue the next section
        Section current_section = input_buffer.dequeue();
        bool is_current_section_valid = current_section.get_f2_frame(0).frame_metadata.is_valid();
        total_sections++;

        // If we haven't see a good section yet, try to use the first section
        if (!has_last_good_section) {
            // Is the metadata valid in the current section?
            if (is_current_section_valid) {
                // Use this section to start
                has_last_good_section = true;
                last_good_section = current_section;
                output_section(current_section);
                qDebug() << "SectionToF2Frame::process_queue - First good section:" <<
                    current_section.get_f2_frame(0).frame_metadata.get_absolute_frame_time().to_string();
            } else {
                // If the first section is bad, add it to the window
                window.enqueue(current_section);
                if (window.size() >= window_size) {
                    flush_window();
                }
                qDebug() << "SectionToF2Frame::process_queue - First bad section:" <<
                    current_section.get_f2_frame(0).frame_metadata.get_absolute_frame_time().to_string();
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
                if (!is_current_section_valid) qDebug() << "SectionToF2Frame::process_queue - Section is invalid - Invalid metadata (bad CRC)";
                //qDebug() << "Sections in window between last good section and current section:" << window.size();

                if (!is_section_valid(current_section, last_good_section, window.size())) qDebug().noquote() << "SectionToF2Frame::process_queue - Section is invalid - current:" <<
                    current_section.get_f2_frame(0).frame_metadata.get_absolute_frame_time().to_string() << "last good:" <<
                    last_good_section.get_f2_frame(0).frame_metadata.get_absolute_frame_time().to_string();

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

bool SectionToF2Frame::is_section_valid(Section current, Section last_good, uint32_t window_size) {
    // For the current section to be good, it must be one absolute frame
    // ahead of the last good section and have valid metadata (CRC passed)
    FrameMetadata current_metadata = current.get_f2_frame(0).frame_metadata;
    FrameMetadata last_good_metadata = last_good.get_f2_frame(0).frame_metadata;

    // If the current section's metadata is invalid, it's invalid.
    if (!current_metadata.is_valid()) {
        qDebug() << "SectionToF2Frame::is_section_valid - Section is invalid - Invalid metadata (bad CRC)";
        return false;
    }

    // If the current section is windows.size() frame(s) ahead of the last good section, it's valid.
    // to avoid inter-track issues we use the absolute frame time.
    if (current_metadata.get_absolute_frame_time() == last_good_metadata.get_absolute_frame_time() + FrameTime(0,0,window_size+1)) {
        return true;
    }

    // Otherwise, the section is invalid
    qDebug().noquote() << "SectionToF2Frame::is_section_valid - Section is invalid - current:" <<
        current_metadata.get_absolute_frame_time().to_string() << "last good:" <<
        last_good_metadata.get_absolute_frame_time().to_string();
    return false;
}

// Correct sections in the window using linear interpolation between
// last_good_section (the good section before the window) and the last
// section in the window (which should be valid). After correction,
// all sections are output.
void SectionToF2Frame::correct_and_flush_window() {
    qDebug().noquote() << "SectionToF2Frame::correct_and_flush_window - Correcting window. Last good section:" <<
        last_good_section.get_f2_frame(0).frame_metadata.get_absolute_frame_time().to_string();
    // The last section in the window is our “next good” section.
    Section next_good_section = window.last();
    FrameMetadata next_good_metadata = next_good_section.get_f2_frame(0).frame_metadata;
    FrameMetadata last_good_metadata = last_good_section.get_f2_frame(0).frame_metadata;  
    int total_buffered = window.size(); // number of frames in the window

    // Determine the total gap in track time and track numbers.
    int32_t deltaTime = next_good_metadata.get_absolute_frame_time().get_time_in_frames() - last_good_metadata.get_absolute_frame_time().get_time_in_frames();
    uint8_t deltaTrack = next_good_metadata.get_track_number() - last_good_metadata.get_track_number();

    // We now have last_good_section as “position 0” and next_good_section as
    // “position totalBuffered”. We will update each buffered section
    // (positions 1..totalBuffered) to have interpolated values.
    QList<Section> temp_list;
    while (!window.isEmpty())
        temp_list.append(window.dequeue());

    // Note: we update all sections in the window including the last one.
    // (The last frame is already valid, but we reassign its values so that
    // everything is consistent.)
    for (int index = 0; index < temp_list.size(); ++index) {
        // Compute the “position” of this section between the last good and the next good.
        // (Position i+1 out of totalBuffered intervals.)
        int pos = index + 1;

        Section corrected = temp_list[index];
        FrameMetadata corrected_metadata = corrected.get_f2_frame(0).frame_metadata;

        // Linear interpolation for absolute track time and track number.
        int32_t corrected_abs_time = last_good_metadata.get_absolute_frame_time().get_time_in_frames() + (deltaTime * pos) / total_buffered;
        int32_t corrected_time = last_good_metadata.get_frame_time().get_time_in_frames() + (deltaTime * pos) / total_buffered;
        uint8_t corrected_track = last_good_metadata.get_track_number() + (deltaTrack * pos) / total_buffered;

        FrameTime abs_time;
        abs_time.set_time_in_frames(corrected_abs_time);

        FrameTime trk_time;
        trk_time.set_time_in_frames(corrected_time);

        if (corrected_metadata.get_absolute_frame_time().get_time_in_frames() != abs_time.get_time_in_frames()) {
            qDebug().noquote() << "SectionToF2Frame::correct_and_flush_window - Corrected frame from abs:" <<
                temp_list[index].get_f2_frame(0).frame_metadata.get_absolute_frame_time().to_string() << "to" << abs_time.to_string();
            qDebug().noquote() << "SectionToF2Frame::correct_and_flush_window - Corrected track number from" <<
                temp_list[index].get_f2_frame(0).frame_metadata.get_track_number() << "to" << corrected_track;
        }

        // Set the corrected ABS time and track number in the section
        // We also copy the frame type from the last good section.
        for (int f2_index = 0; f2_index < 98; f2_index++) { 
            F2Frame f2_frame = corrected.get_f2_frame(f2_index);

            f2_frame.frame_metadata.set_frame_type(last_good_metadata.get_frame_type());
            f2_frame.frame_metadata.set_track_number(corrected_track);
            f2_frame.frame_metadata.set_absolute_frame_time(abs_time);
            f2_frame.frame_metadata.set_frame_time(trk_time);

            corrected.set_f2_frame(f2_index, f2_frame);
        }

        output_section(corrected);
        corrected_sections++;

        // Update the last good frame as we go.
        last_good_section = corrected;
    }
}

// Flush the window without any correction - output all frames in order.
void SectionToF2Frame::flush_window() {
    qDebug() << "SectionToF2Frame::flush_window - Flushing window without correction";
    while (!window.isEmpty()) {
        Section section = window.dequeue();
        uncorrectable_sections++;
        output_section(section);
        if (section.get_f2_frame(0).frame_metadata.is_valid())
            last_good_section = section;
    }
}

// Output the F2 frames in the section to the output buffer
// and count the number of tracks, durations, etc.
void SectionToF2Frame::output_section(Section section) {
    QVector<F2Frame> f2_frames;
    for (int i = 0; i < 98; i++) {
        f2_frames.append(section.get_f2_frame(i));
        total_f2_frames++;
    }

    // Add the F2 frames to the output buffer
    output_buffer.enqueue(f2_frames);

    // Statistics generation...
    uint8_t track_number = section.get_f2_frame(0).frame_metadata.get_track_number();
    FrameTime frame_time = section.get_f2_frame(0).frame_metadata.get_frame_time();
    FrameTime absolute_time = section.get_f2_frame(0).frame_metadata.get_absolute_frame_time();

    // Set the absolute start and end times
    if (absolute_time < absolute_start_time) absolute_start_time = absolute_time;
    if (absolute_time > absolute_end_time) absolute_end_time = absolute_time;

    // Do we have a new track?
    if (!track_numbers.contains(track_number)) {
        // Append the new track to the statistics
        track_numbers.append(track_number);
        track_start_times.append(frame_time);
        track_end_times.append(frame_time);
    } else {
        // Update the end time for the existing track
        int index = track_numbers.indexOf(track_number);
        if (frame_time < track_start_times[index]) track_start_times[index] = frame_time;
        if (frame_time > track_end_times[index]) track_end_times[index] = frame_time;
    }

    // Save the current track and frame time
    current_track = track_number;
    current_frame_time = frame_time;
    current_absolute_time = absolute_time;
}

void SectionToF2Frame::show_statistics() {
    qInfo() << "Section to F2 frame statistics:";
    qInfo() << "  Sections:";
    qInfo() << "    Total sections:" << total_sections;
    qInfo() << "    Corrected sections:" << corrected_sections;
    qInfo() << "    Uncorrectable sections:" << uncorrectable_sections;
    qInfo() << "    Output F2 frames:" << total_f2_frames;

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
    FrameTime total_duration = FrameTime(0,0,0);
    for (int i = 0; i < track_numbers.size(); i++) {
        total_duration = total_duration + (track_end_times[i] - track_start_times[i]);
    }
    if (track_numbers.size() > 1) qInfo().noquote() << "  Total duration of" << track_numbers.size() << "tracks:" << total_duration.to_string();
    else qInfo().noquote() << "  Total duration of" << track_numbers.size() << "track:" << total_duration.to_string();
}