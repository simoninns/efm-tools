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
    valid_sections_count = 0;
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
        QVector<F2Frame> f2_frames;

        for (int i = 0; i < 98; i++) {
            f2_frames.append(section.get_f2_frame(i));
        }

        // Add the F2 frames to the output buffer
        output_buffer.enqueue(f2_frames);
        valid_sections_count++;
    }
}

void SectionToF2Frame::show_statistics() {
    qInfo() << "Section to F2 frame statistics:";
    qInfo() << "  Processed sections:" << valid_sections_count;
    qInfo() << "  Total F2 frames:" << valid_sections_count * 98;
}