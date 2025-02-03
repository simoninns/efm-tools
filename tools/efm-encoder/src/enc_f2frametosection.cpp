/************************************************************************

    enc_f2frametosection.cpp

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

#include "enc_f2frametosection.h"

// F2FrameToSection class implementation
F2FrameToSection::F2FrameToSection() {
    valid_sections_count = 0;
}

// Input is 32 bytes of data from the F2 frame payload
void F2FrameToSection::push_frame(F2Frame f2_frame) {
    input_buffer.enqueue(f2_frame);
    process_queue();
}

Section F2FrameToSection::pop_section() {
    if (!is_ready()) {
        qFatal("F2FrameToF3Frame::pop_section(): No sections are available.");
    }
    return output_buffer.dequeue();
}

// Process the input queue of F2 frames into F3 frame sections
// Each section consists of 98 F2 frames, the first two of which are sync frames
// The remaining 96 frames are subcode frames.
void F2FrameToSection::process_queue() {
    while (input_buffer.size() >= 98) {
        Section section;
        
        for (uint32_t symbol = 0; symbol < 98; ++symbol) {
            F2Frame f2_frame = input_buffer.dequeue();
            section.push_frame(f2_frame);
        }

        // Queue the section
        valid_sections_count++;
        output_buffer.enqueue(section);
    }
}

bool F2FrameToSection::is_ready() const {
    return !output_buffer.isEmpty();
}