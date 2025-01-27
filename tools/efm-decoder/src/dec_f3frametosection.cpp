/************************************************************************

    dec_f3frametosection.cpp

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

#include "dec_f3frametosection.h"

F3FrameToSection::F3FrameToSection() {
    invalid_f3_frames_count = 0;
    valid_f3_frames_count = 0;
}

void F3FrameToSection::push_frame(F3Frame data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

Section F3FrameToSection::pop_section() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool F3FrameToSection::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void F3FrameToSection::process_queue() {
    // Process the input buffer
    while (input_buffer.size() >= 98) {
        // Firstly we have to gather the 98 F3 frames and compute the subcode
        // (since we need the subcode in order to generate the F2 frames)
        QVector<F3Frame> f3_frames;
        Subcode subcode;

        for (uint32_t index = 0; index < 98; index++) {
            F3Frame f3_frame = input_buffer.dequeue();
            f3_frames.append(f3_frame);

            if (index >= 2) {
                // Get the subcode byte from the F3 frame and set it in the subcode object
                subcode.set_subcode_byte(index, f3_frame.get_subcode_byte());
            }

            valid_f3_frames_count++;
        }

        // Now we have 98 F3 Frames and the subcode, we can create a new section
        Section section;
        for (uint32_t index = 0; index < 98; index++) {
            F2Frame f2_frame;
            f2_frame.set_data(f3_frames[index].get_data());
            
            // Now we have to extract the section data and add it to the F2 frame
            // Note: The F2 Frame doesn't support AP time at the moment - to-do
            f2_frame.set_frame_type(subcode.q_channel.get_frame_type());
            f2_frame.set_track_number(subcode.q_channel.get_track_number());
            f2_frame.set_frame_time(subcode.q_channel.get_frame_time());

            section.push_frame(f2_frame);
        }
        
        // Copy the subcode object to the section object
        section.subcode = subcode;

        // Add the section to the output buffer
        output_buffer.enqueue(section);
    }
}