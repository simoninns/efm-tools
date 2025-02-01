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
      delay_line2_err({0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2}),
      delay_lineM_err({108, 104, 100, 96, 92, 88, 84, 80, 76, 72, 68, 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0}),
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

        // Make an error data vector the same size as the data vector
        QVector<uint8_t> error_data;
        error_data.resize(data.size());
        error_data.fill(0);

        data = delay_line1.push(data);
        if (data.isEmpty()) continue;

        // Process the data
        // Note: We will only get valid data if the delay lines are all full
        data = inverter.invert_parity(data);

        circ.c1_decode(data, error_data);

        data = delay_lineM.push(data);
        error_data = delay_lineM_err.push(error_data);
        if (data.isEmpty()) continue;

        // Only perform C2 decode if delay line 1 is full and delay line M is full
        circ.c2_decode(data, error_data);

        data = interleave.deinterleave(data);
        error_data = interleave_err.deinterleave(error_data);

        data = delay_line2.push(data);
        error_data = delay_line2_err.push(error_data);
        if (data.isEmpty()) continue;

        // Check if any value in the F2 error_data is not 0
        bool error_detected = false;
        for (auto byte : error_data) {
            if (byte != 0) {
                error_detected = true;
                break;
            }
        }

        if (!error_detected) valid_f2_frames_count++;
        else invalid_f2_frames_count++;

        // Put the resulting data (and error data) into an F1 frame and push it to the output buffer
        F1Frame f1_frame;
        f1_frame.set_data(data);
        f1_frame.set_error_data(error_data);
        f1_frame.set_frame_type(f2_frame.get_frame_type());
        f1_frame.set_frame_time(f2_frame.get_frame_time());
        f1_frame.set_track_number(f2_frame.get_track_number());
        output_buffer.enqueue(f1_frame);
    }
}

void F2FrameToF1Frame::show_statistics() {
    qInfo() << "F2 frame to F1 frame statistics:";
    qInfo() << "  Frames:";
    qInfo() << "    Valid F2 frames:" << valid_f2_frames_count;
    qInfo() << "    Corrupt F2 frames:" << invalid_f2_frames_count; // 1 or more errors in the frame data
    qInfo() << "  C1 decoder:";
    qInfo() << "    Valid C1s:" << circ.get_valid_c1s();
    qInfo() << "    Fixed C1s:" << circ.get_fixed_c1s();
    qInfo() << "    Error C1s:" << circ.get_error_c1s();
    qInfo() << "  C2 decoder:";
    qInfo() << "    Valid C2s:" << circ.get_valid_c2s();
    qInfo() << "    Fixed C2s:" << circ.get_fixed_c2s();
    qInfo() << "    Error C2s:" << circ.get_error_c2s();
}