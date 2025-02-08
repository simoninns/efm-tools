/************************************************************************

    dec_f3frametof2section.cpp

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

#include "dec_f3frametof2section.h"

F3FrameToF2Section::F3FrameToF2Section() {
    // Set the initial state
    current_state = EXPECTING_SYNC0;

    missed_sync0s = 0;
    missed_sync1s = 0;
    missed_subcodes = 0;
    valid_sections = 0;
    invalid_sections = 0;
    input_f3_frames = 0;
}

void F3FrameToF2Section::push_frame(F3Frame data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);
    input_f3_frames++;

    // Process the state machine
    process_state_machine();
}

F2Section F3FrameToF2Section::pop_section() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool F3FrameToF2Section::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void F3FrameToF2Section::process_state_machine() {
    // Keep processing the state machine until the input buffer is empty
    while(input_buffer.size() > 0) {
        switch(current_state) {
            case EXPECTING_SYNC0:
                current_state = expecting_sync0();
                break;

            case EXPECTING_SYNC1:
                current_state = expecting_sync1();
                break;

            case EXPECTING_SUBCODE:
                current_state = expecting_subcode();
                break;

            case PROCESS_SECTION:
                current_state = process_section();
                break;
        }
    }
}

F3FrameToF2Section::State F3FrameToF2Section::expecting_sync0() {
    // Pop the first frame from the input buffer
    F3Frame f3_frame = input_buffer.dequeue();

    State next_state = EXPECTING_SYNC0;

    switch(f3_frame.get_f3_frame_type()) {
        case F3Frame::SYNC0:
            // Add the frame to the section buffer
            section_buffer.clear();
            section_buffer.append(f3_frame);

            // Move to the next state
            next_state = EXPECTING_SYNC1;
            break;

        case F3Frame::SYNC1:
            missed_sync0s++;
            if (show_debug) qDebug() << "F3FrameToF2Section::expecting_sync0 - Sync1 frame received when expecting Sync0";

            // Is there a frame in the section buffer?
            if(section_buffer.size() > 0) {
                // Use the last frame in the section buffer as the sync0
                F3Frame sync0_frame = section_buffer.last();
                sync0_frame.set_frame_type_as_sync0();
                section_buffer.clear();
                section_buffer.append(sync0_frame);

                // Add the sync1 to the section buffer
                section_buffer.append(f3_frame);
                
                // Move to the next state
                next_state =  EXPECTING_SUBCODE;
            } else {
                // No frame available in the buffer - Make a fake sync0 frame
                F3Frame sync0_frame;
                sync0_frame.set_frame_type_as_sync0();
                section_buffer.clear();
                section_buffer.append(sync0_frame);

                // Add the sync1 to the section buffer
                section_buffer.append(f3_frame);

                // Move to the next state
                next_state = EXPECTING_SUBCODE;
            }
            break;

        case F3Frame::SUBCODE:
            missed_sync0s++;
            // If we get a subcode, assume it's a miss-labelled sync0 frame
            if (show_debug) qDebug() << "F3FrameToF2Section::expecting_sync0 - Subcode frame received when expecting Sync0";
            f3_frame.set_frame_type_as_sync0();
            section_buffer.clear();
            section_buffer.append(f3_frame);
            next_state =  EXPECTING_SYNC1;
            break;
    }

    return next_state;
}

F3FrameToF2Section::State F3FrameToF2Section::expecting_sync1() {
    // Pop the first frame from the input buffer
    F3Frame f3_frame = input_buffer.dequeue();

    State next_state = EXPECTING_SYNC1;

    switch(f3_frame.get_f3_frame_type()) {
        case F3Frame::SYNC1:
            // Add the frame to the section buffer
            section_buffer.append(f3_frame);

            // Move to the next state
            next_state = EXPECTING_SUBCODE;
            break;

        case F3Frame::SYNC0:
            missed_sync1s++;
            // If we get a sync0, continue to wait for the sync1
            if (show_debug) qDebug() << "F3FrameToF2Section::expecting_sync1 - Sync0 frame received when expecting Sync1";

            // Add the sync0 to a new section buffer
            section_buffer.clear();
            section_buffer.append(f3_frame);

            // Move to the next state
            next_state = EXPECTING_SYNC1;
            break;

        case F3Frame::SUBCODE:
            missed_sync1s++;
            // If we get a subcode, assume it's a miss-labeled sync1 frame
            if (show_debug) qDebug() << "F3FrameToF2Section::expecting_sync1 - Subcode frame received when expecting Sync1";
            f3_frame.set_frame_type_as_sync1();        

            // Add the sync1 to the section buffer
            section_buffer.append(f3_frame);

            // Move to the next state
            next_state = EXPECTING_SUBCODE;
            break;
    }

    return next_state;
}

F3FrameToF2Section::State F3FrameToF2Section::expecting_subcode() {
    // Pop the first frame from the input buffer
    F3Frame f3_frame = input_buffer.dequeue();

    State next_state = EXPECTING_SUBCODE;

    switch(f3_frame.get_f3_frame_type()) {
        case F3Frame::SUBCODE:
            // Add the frame to the section buffer
            section_buffer.append(f3_frame);

            // Is the section buffer full?
            if(section_buffer.size() == 98) {
                // Process the section buffer
                valid_sections++;
                next_state = PROCESS_SECTION;
            } else {
                // Continue to wait for more subcode frames
                next_state = EXPECTING_SUBCODE;
            }
            break;

        case F3Frame::SYNC0:
            missed_subcodes++;
            if (show_debug) qDebug() << "F3FrameToF2Section::expecting_subcode - Sync0 frame received when expecting Subcode";

            // Assume section is lost - clear the buffer and look for the sync1
            section_buffer.clear();
            invalid_sections++;

            // Add the sync0 frame to a new section buffer
            section_buffer.clear();
            section_buffer.append(f3_frame);

            // Move to the next state
            next_state = EXPECTING_SYNC1;
            break;

        case F3Frame::SYNC1:
            missed_subcodes++;
            if (show_debug) qDebug() << "F3FrameToF2Section::expecting_subcode - Sync1 frame received when expecting Subcode";

            // If we get a sync1, assume it's a miss-labeled subcode frame
            f3_frame.set_frame_type_as_subcode(0);        

            // Add the subcode to the section buffer
            section_buffer.append(f3_frame);

            // Is the section buffer full?
            if(section_buffer.size() == 98) {
                // Process the section buffer
                valid_sections++;
                next_state = PROCESS_SECTION;
            } else {
                // Continue to wait for more subcode frames
                next_state = EXPECTING_SUBCODE;
            }
            break;
    }

    return next_state;
}

F3FrameToF2Section::State F3FrameToF2Section::process_section() {
    // Sanity check - Do we have a full section buffer?
    if(section_buffer.size() != 98) {
        qFatal("F3FrameToF2Section::process_section - Section buffer is not full");
    }

    // Sanity check - Is the first frame a sync0?
    if(section_buffer.first().get_f3_frame_type() != F3Frame::SYNC0) {
        qFatal("F3FrameToF2Section::process_section - First frame in section buffer is not a Sync0");
    }

    // Sanity check - Is the second frame a sync1?
    if(section_buffer.at(1).get_f3_frame_type() != F3Frame::SYNC1) {
        qFatal("F3FrameToF2Section::process_section - Second frame in section buffer is not a Sync1");
    }

    // Sanity check - Are frames 3-98 subcode frames?
    for(int i = 2; i < 98; i++) {
        if(section_buffer.at(i).get_f3_frame_type() != F3Frame::SUBCODE) {
            qFatal("F3FrameToF2Section::process_section - Frame %d in section buffer is not a Subcode", i);
        }
    }

    // Generate the subcode from the 98 bytes of data
    Subcode subcode;
    QByteArray subcode_data;
    for (int i = 0; i < 98; i++) {
        subcode_data[i] = section_buffer[i].get_subcode_byte();
    }
    SectionMetadata section_metadata = subcode.from_data(subcode_data);

    // Now we have 98 F3 Frames and a subcode for metadata, we can create a new section
    F2Section f2_section;

    // Add the 98 F2 frames to the section
    for (uint32_t index = 0; index < 98; index++) {
        F2Frame f2_frame;
        f2_frame.set_data(section_buffer[index].get_data());
        f2_frame.set_error_data(section_buffer[index].get_error_data());
        f2_section.push_frame(f2_frame);
    }

    // Set the section metadata
    f2_section.metadata = section_metadata;

    // Add the section to the output buffer
    output_buffer.enqueue(f2_section);

    // Clear the section buffer and move to the next state
    section_buffer.clear();
    return EXPECTING_SYNC0;
}

void F3FrameToF2Section::show_statistics() {
    qInfo() << "F3 Frame to F2 Section statistics:";
    qInfo() << "  F2 Sections:";
    qInfo() << "    Valid F2 sections:" << valid_sections;
    qInfo() << "    Invalid F2 sections:" << invalid_sections;
    qInfo() << "  Sync tracking:";
    qInfo() << "    Missed sync0s:" << missed_sync0s;
    qInfo() << "    Missed sync1s:" << missed_sync1s;
    qInfo() << "    Missed subcodes:" << missed_subcodes;
    qInfo() << "  F3 Frames:";
    qInfo() << "    Input F3 frames:" << input_f3_frames;
    qInfo() << "    Output F2 frames:" << (valid_sections) * 98;
    qInfo() << "    Discarded F3 frames:" << input_f3_frames - (valid_sections * 98);
}