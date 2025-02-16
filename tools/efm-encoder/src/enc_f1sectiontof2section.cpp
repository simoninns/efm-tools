/************************************************************************

    enc_f1sectiontof2section.cpp

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

#include "enc_f1sectiontof2section.h"

// F1SectionToF2Section class implementation
F1SectionToF2Section::F1SectionToF2Section()
    : delay_line1({ 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
                    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 }),
      delay_line2({ 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0 }),
      delay_lineM({ 0,  4,  8,  12, 16, 20, 24, 28, 32, 36, 40, 44,  48,  52,
                    56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108 })
{
    valid_f2_sections_count = 0;
}

void F1SectionToF2Section::push_section(F1Section f1_section)
{
    input_buffer.enqueue(f1_section);
    process_queue();
}

F2Section F1SectionToF2Section::pop_section()
{
    if (!is_ready()) {
        qFatal("F1SectionToF2Section::pop_section(): No F2 sections are available.");
    }
    return output_buffer.dequeue();
}

// Note: The F2 frames will not be correct until the delay lines are full
// So lead-in is required to prevent loss of the input date.  For now we will
// just discard the data until the delay lines are full.
//
// This will drop 108+2+1 = 111 F2 frames of data - The decoder will also have
// the same issue and will loose another 111 frames of data (so you need at least
// 222 frames of lead-in data to ensure the decoder has enough data to start decoding)
void F1SectionToF2Section::process_queue()
{
    while (!input_buffer.isEmpty()) {
        F1Section f1_section = input_buffer.dequeue();
        F2Section f2_section;
        f2_section.metadata = f1_section.metadata;

        for (int index = 0; index < 98; ++index) {
            // Pop the F1 frame and copy the data
            F1Frame f1_frame = f1_section.get_frame(index);
            QVector<uint8_t> data = f1_frame.get_data();

            // Process the data
            data = delay_line2.push(data);
            if (data.isEmpty()) {
                // Generate a blank F2 frame (to keep the section in sync)
                F2Frame f2_frame;
                QVector<uint8_t> blank_data(32, 0);
                f2_frame.set_data(blank_data);
                f2_section.push_frame(f2_frame);
                continue;
            }

            data = interleave.interleave(data); // 24
            circ.c2_encode(data); // 24 + 4 = 28

            data = delay_lineM.push(data); // 28
            if (data.isEmpty()) {
                // Generate a blank F2 frame (to keep the section in sync)
                F2Frame f2_frame;
                QVector<uint8_t> blank_data(32, 0);
                f2_frame.set_data(blank_data);
                f2_section.push_frame(f2_frame);
                continue;
            }

            circ.c1_encode(data); // 28 + 4 = 32

            data = delay_line1.push(data); // 32
            if (data.isEmpty()) {
                // Generate a blank F2 frame (to keep the section in sync)
                F2Frame f2_frame;
                QVector<uint8_t> blank_data(32, 0);
                f2_frame.set_data(blank_data);
                f2_section.push_frame(f2_frame);
                continue;
            }

            data = inverter.invert_parity(data); // 32

            // Put the resulting data into an F2 frame and push it to the output buffer
            F2Frame f2_frame;
            f2_frame.set_data(data);

            f2_section.push_frame(f2_frame);
        }

        valid_f2_sections_count++;
        output_buffer.enqueue(f2_section);
    }
}

bool F1SectionToF2Section::is_ready() const
{
    return !output_buffer.isEmpty();
}
