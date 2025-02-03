/************************************************************************

    enc_sectiontof3frame.cpp

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

#include "enc_sectiontof3frame.h"

// SectionToF3Frame class implementation
SectionToF3Frame::SectionToF3Frame() {
    valid_f3_frames_count = 0;
}

void SectionToF3Frame::push_section(Section section) {
    input_buffer.enqueue(section);
    process_queue();
}

QVector<F3Frame> SectionToF3Frame::pop_frames() {
    if (!is_ready()) {
        qFatal("SectionToF3Frame::pop_frames(): No F3 frames are available.");
    }
    return output_buffer.dequeue();
}

void SectionToF3Frame::process_queue() {
    while (input_buffer.size() >= 1) {
        Section section = input_buffer.dequeue();
        QVector<F3Frame> f3_frames;

        // Take the metadata information from the first F2 frame in the section
        Subcode subcode;
        QByteArray subcode_data = subcode.to_data(section.get_f2_frame(10).frame_metadata);

        for (uint32_t symbol_number = 0; symbol_number < 98; ++symbol_number) {
            F2Frame f2_frame = section.get_f2_frame(symbol_number);
            F3Frame f3_frame;

            if (symbol_number == 0) {
                f3_frame.set_frame_type_as_sync0();
            } else if (symbol_number == 1) {
                f3_frame.set_frame_type_as_sync1();
            } else {
                // Generate the subcode byte
                f3_frame.set_frame_type_as_subcode(subcode_data[symbol_number]);
            }

            f3_frame.set_data(f2_frame.get_data());

            valid_f3_frames_count++;
            f3_frames.append(f3_frame);
        }

        output_buffer.enqueue(f3_frames);
    }
}

bool SectionToF3Frame::is_ready() const {
    return !output_buffer.isEmpty();
}
