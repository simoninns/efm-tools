/************************************************************************

    enc_data24_to_f1frame.cpp

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-encoder is free software: you can redistribute it and/or
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

#include "enc_data24tof1frame.h"

// Data24ToF1Frame class implementation
Data24ToF1Frame::Data24ToF1Frame() {
    valid_f1_frames_count = 0;
}

void Data24ToF1Frame::push_frame(Data24 data24) {
    input_buffer.enqueue(data24);
    process_queue();
}

F1Frame Data24ToF1Frame::pop_frame() {
    if (!is_ready()) {
        qFatal("Data24ToF1Frame::pop_frame(): No F1 frames are available.");
    }
    return output_buffer.dequeue();
}

void Data24ToF1Frame::process_queue() {
    while (!input_buffer.isEmpty()) {
        Data24 data24 = input_buffer.dequeue();

        // ECMA-130 issue 2 page 16 - Clause 16
        // All byte pairs are swapped by the F1 Frame encoder
        QVector<uint8_t> data = data24.get_data();
        for (int i = 0; i < data.size(); i += 2) {
            std::swap(data[i], data[i + 1]);
        }

        F1Frame f1_frame;
        f1_frame.set_data(data);
        f1_frame.frame_metadata = data24.frame_metadata;

        valid_f1_frames_count++;
        output_buffer.enqueue(f1_frame);
    }
}

bool Data24ToF1Frame::is_ready() const {
    return !output_buffer.isEmpty();
}
