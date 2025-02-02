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
    // Tracking variables
    missed_subcodes = 0;

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
        Section section = input_buffer.dequeue();

        Subcode subcode = section.subcode;

        if (subcode.is_valid()) {
            uint32_t track_number = subcode.q_channel.get_track_number();
            FrameTime frame_time = subcode.q_channel.get_frame_time();
            FrameTime absolute_time = subcode.q_channel.get_ap_time();

            // qDebug() << "Processing section with track number:" << track_number;
            // qDebug() << "  Section time:" << frame_time.to_string();
            // qDebug() << "  Section absolute time:" << absolute_time.to_string();

            // Set the absolute start and end times
            if (absolute_time < absolute_start_time) absolute_start_time = absolute_time;
            if (absolute_time > absolute_end_time) absolute_end_time = absolute_time;

            // Do we have a new track?
            if (!track_numbers.contains(track_number)) {
                // Append a new track
                track_numbers.append(track_number);
                track_start_times.append(frame_time);
                track_end_times.append(frame_time);
            } else {
                // Update the end time for the existing track
                int index = track_numbers.indexOf(track_number);
                if (frame_time < track_start_times[index]) track_start_times[index] = frame_time;
                if (frame_time > track_end_times[index]) track_end_times[index] = frame_time;
            }
        } else {
            // Subcode is invalid...
            qWarning() << "SectionToF2Frame::process_queue - Invalid subcode";
        } 

        QVector<F2Frame> f2_frames;
        uint32_t track_number = subcode.q_channel.get_track_number();
        FrameTime frame_time = subcode.q_channel.get_frame_time();
        FrameTime absolute_time = subcode.q_channel.get_ap_time();

        for (int i = 0; i < 98; i++) {
            F2Frame f2_frame = section.get_f2_frame(i);

            f2_frame.set_absolute_frame_time(absolute_time);
            f2_frame.set_frame_time(frame_time);
            f2_frame.set_track_number(track_number);

            f2_frames.append(section.get_f2_frame(i));
        }

        // Add the F2 frames to the output buffer
        output_buffer.enqueue(f2_frames);
    }
}

void SectionToF2Frame::show_statistics() {
    qInfo() << "Section to F2 frame statistics:";
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