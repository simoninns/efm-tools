/************************************************************************

    dec_f1frametodata24.cpp

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

#include "dec_f1frametodata24.h"

F1FrameToData24::F1FrameToData24() {
    valid_f1_frames_count = 0;
    invalid_f1_frames_count = 0;
    start_time = FrameTime(59, 59, 74);
    end_time = FrameTime(0, 0, 0);
    corrupt_bytes_count = 0;
}

void F1FrameToData24::push_frame(F1Frame data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

Data24 F1FrameToData24::pop_frame() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool F1FrameToData24::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void F1FrameToData24::process_queue() {
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        F1Frame f1_frame = input_buffer.dequeue();
        QVector<uint8_t> data = f1_frame.get_data();
        QVector<uint8_t> error_data = f1_frame.get_error_data();

        // ECMA-130 issue 2 page 16 - Clause 16
        // All byte pairs are swapped by the F1 Frame encoder
        for (int i = 0; i < data.size(); i += 2) {
            if (i + 1 < data.size()) {
                std::swap(data[i], data[i + 1]);
                std::swap(error_data[i], error_data[i + 1]);
            }
        }
        
        // Check the error data (and count any flagged errors)
        uint32_t error_count = f1_frame.count_errors();

        corrupt_bytes_count += error_count;

        if (error_count > 0) invalid_f1_frames_count++;
        else valid_f1_frames_count++;

        // Put the resulting data into a Data24 frame and push it to the output buffer
        Data24 data24;
        data24.set_data(data);
        data24.set_error_data(error_data);
        data24.frame_metadata = f1_frame.frame_metadata;

        if (data24.frame_metadata.get_absolute_frame_time() < start_time) {
            start_time = data24.frame_metadata.get_absolute_frame_time();
        }

        if (data24.frame_metadata.get_absolute_frame_time() >= end_time) {
            end_time = f1_frame.frame_metadata.get_absolute_frame_time();
        }

        // Add the data to the output buffer
        output_buffer.enqueue(data24);
    }
}

void F1FrameToData24::show_statistics() {
    qInfo() << "F1 frame to Data24 statistics:";

    qInfo() << "  Frames:";
    qInfo() << "    Valid F1 frames:" << valid_f1_frames_count;
    qInfo() << "    Invalid F1 frames:" << invalid_f1_frames_count;

    qInfo() << "  Bytes:";
    uint32_t valid_bytes = (valid_f1_frames_count + invalid_f1_frames_count) * 24;
    qInfo().nospace() << "    Valid bytes: " << valid_bytes;
    qInfo().nospace() << "    Corrupt bytes: " << corrupt_bytes_count;
    qInfo().nospace().noquote() << "    Data loss: " << QString::number((corrupt_bytes_count * 100.0) / valid_bytes, 'f', 3) << "%";

    qInfo() << "  Audio Samples (16L+16R):";
    qInfo().nospace() << "    Valid samples: " << valid_bytes / 4;
    qInfo().nospace() << "    Corrupt samples: " << corrupt_bytes_count / 4;    

    qInfo() << "  Q-Channel time information:";
    qInfo().noquote() << "    Start time:" << start_time.to_string();
    qInfo().noquote() << "    End time:" << end_time.to_string();
    qInfo().noquote() << "    Total time:" << (end_time - start_time).to_string();
}