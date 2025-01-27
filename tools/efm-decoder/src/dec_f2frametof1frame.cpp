/************************************************************************

    dec_f2frametof1frame.cpp

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

#include "dec_f2frametof1frame.h"

F2FrameToF1Frame::F2FrameToF1Frame()
    : delay_line1({0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1}),
      delay_line2({0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2}),
      delay_lineM({108, 104, 100, 96, 92, 88, 84, 80, 76, 72, 68, 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0}),
      invalid_f2_frames_count(0),
      valid_f2_frames_count(0) 
{}

void F2FrameToF1Frame::push_frame(F2Frame data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

F1Frame F2FrameToF1Frame::pop_frame() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool F2FrameToF1Frame::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

// Note: The F2 frames will not be correct until the delay lines are full
// So lead-in is required to prevent loss of the input date.  For now we will
// just discard the data until the delay lines are full.
void F2FrameToF1Frame::process_queue() {
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        F2Frame f2_frame = input_buffer.dequeue();
        QVector<uint8_t> data = f2_frame.get_data();

        data = delay_line1.push(data);
        if (data.isEmpty()) continue;

        // Process the data
        data = inverter.invert_parity(data);

        data = circ.c1_decode(data);

        data = delay_lineM.push(data);
        if (data.isEmpty()) continue;

        // Only perform C2 decode if delay line 1 is full and delay line M is full
        data = circ.c2_decode(data);

        data = interleave.deinterleave(data);

        data = delay_line2.push(data);
        if (data.isEmpty()) continue;

        // We will only get valid data if the delay lines are all full
        valid_f2_frames_count++;

        // Put the resulting data into an F1 frame and push it to the output buffer
        F1Frame f1_frame;
        f1_frame.set_data(data);
        f1_frame.set_frame_type(f2_frame.get_frame_type());
        f1_frame.set_frame_time(f2_frame.get_frame_time());
        f1_frame.set_track_number(f2_frame.get_track_number());
        valid_f2_frames_count++;
        output_buffer.enqueue(f1_frame);
    }
}

// Get the statistics for the C1 decoder
void F2FrameToF1Frame::get_c1_circ_stats(int32_t &valid_c1s, int32_t &fixed_c1s, int32_t &error_c1s) {
    valid_c1s = circ.get_valid_c1s();
    fixed_c1s = circ.get_fixed_c1s();
    error_c1s = circ.get_error_c1s();
}

// Get the statistics for the C2 decoder
void F2FrameToF1Frame::get_c2_circ_stats(int32_t &valid_c2s, int32_t &fixed_c2s, int32_t &error_c2s) {
    valid_c2s = circ.get_valid_c2s();
    fixed_c2s = circ.get_fixed_c2s();
    error_c2s = circ.get_error_c2s();
}