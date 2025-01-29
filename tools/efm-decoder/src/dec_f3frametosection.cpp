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
    // Reset statistics
    valid_sync0_frames = 0;
    valid_sync1_frames = 0;
    valid_subcode_frames = 0;
    valid_sections = 0;
    invalid_sync0_frames = 0;
    invalid_sync1_frames = 0;
    invalid_subcode_frames = 0;
    invalid_sections = 0;
    sync_lost_count = 0;

    current_state = WAITING_FOR_INITIAL_SYNC0;
}

void F3FrameToSection::push_frame(F3Frame data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the state machine
    process_state_machine();
}

Section F3FrameToSection::pop_section() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool F3FrameToSection::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void F3FrameToSection::process_state_machine() {
    // Keep processing the state machine until the input buffer is empty
    while(input_buffer.size() > 0) {
        switch(current_state) {
            case WAITING_FOR_INITIAL_SYNC0:
                current_state = state_waiting_for_initial_sync0();
                break;

            case WAITING_FOR_SYNC0:
                current_state = state_waiting_for_sync0();
                break;

            case WAITING_FOR_SYNC1:
                current_state = state_waiting_for_sync1();
                break;

            case WAITING_FOR_SUBCODE:
                current_state = state_waiting_for_subcode();
                break;

            case SECTION_COMPLETE:
                current_state = state_section_complete();
                break;

            case SYNC_LOST:
                current_state = state_sync_lost();
                break;
        }
    }
}

F3FrameToSection::State F3FrameToSection::state_waiting_for_initial_sync0() {
    // Clear the internal F3 frame buffer
    internal_buffer.clear();

    // Pop a frame from the input buffer
    F3Frame f3_frame = input_buffer.dequeue();

    // Is the first frame a sync0 frame?
    if (f3_frame.get_f3_frame_type() != F3Frame::F3FrameType::SYNC0) {
        qDebug() << "F3FrameToSection::state_waiting_for_initial_sync0(): F3 Frame was not a sync0 frame - discarding";
        invalid_sync0_frames++;
        return WAITING_FOR_INITIAL_SYNC0;
    }

    // Add the frame to the current section
    internal_buffer.append(f3_frame);
    valid_sync0_frames++;

    qDebug() << "F3FrameToSection::state_waiting_for_initial_sync0 - Got sync0 frame";
    return WAITING_FOR_SYNC1;
}

F3FrameToSection::State F3FrameToSection::state_waiting_for_sync0() {
    // Pop a frame from the input buffer
    F3Frame f3_frame = input_buffer.dequeue();

    // Is the first frame a sync0 frame?
    if (f3_frame.get_f3_frame_type() != F3Frame::F3FrameType::SYNC0) {
        qDebug() << "F3FrameToSection::state_waiting_for_sync0(): F3 Frame was not a sync0 frame - discarding";
        invalid_sync0_frames++;
        return SYNC_LOST;
    }

    // Add the frame to the current section
    internal_buffer.append(f3_frame);
    valid_sync0_frames++;

    qDebug() << "F3FrameToSection::state_waiting_for_sync0 - Got sync0 frame";
    // Now wait for sync1
    return WAITING_FOR_SYNC1;
}

F3FrameToSection::State F3FrameToSection::state_waiting_for_sync1() {
    // Pop a frame from the input buffer
    F3Frame f3_frame = input_buffer.dequeue();

    // Is the frame a sync1 frame?
    if (f3_frame.get_f3_frame_type() != F3Frame::F3FrameType::SYNC1) {
        qDebug() << "F3FrameToSection::state_waiting_for_sync1(): F3 Frame was not a sync1 frame - discarding";
        invalid_sync1_frames++;
        return SYNC_LOST;
    }

    // Add the frame to the current section
    internal_buffer.append(f3_frame);
    valid_sync1_frames++;

    // Keep waiting for the sync header
    qDebug() << "F3FrameToSection::state_waiting_for_sync1 - Got sync1 frame";
    return WAITING_FOR_SUBCODE;
}

// In this state we have received a channel frame which is reasonable in length
// and we can now process the channel frame into an F3 frame.
F3FrameToSection::State F3FrameToSection::state_waiting_for_subcode() {
    // Pop a frame from the input buffer
    F3Frame f3_frame = input_buffer.dequeue();

    // Is the frame a subcode frame?
    if (f3_frame.get_f3_frame_type() != F3Frame::F3FrameType::SUBCODE) {
        qDebug() << "F3FrameToSection::state_waiting_for_subcode(): F3 Frame was not a subcode frame - discarding";
        invalid_subcode_frames++;
        invalid_sections++;
        return SYNC_LOST;
    }

    // Add the frame to the current section
    internal_buffer.append(f3_frame);
    valid_subcode_frames++;

    // Do we have enough frames to create a section?
    if (internal_buffer.size() == 98) {
        return SECTION_COMPLETE;
    }

    // Keep waiting for more subcode frames
    qDebug() << "F3FrameToSection::state_waiting_for_subcode - Got subcode frame" << internal_buffer.size() << "/98";
    return WAITING_FOR_SUBCODE;
}

F3FrameToSection::State F3FrameToSection::state_section_complete() {
    // Firstly we have to generate the subcode from data in the 98 F3 frames
    Subcode subcode;
    for (uint32_t index = 0; index < 98; index++) {
        // Sanity checks
        if (index == 0 && internal_buffer[index].get_f3_frame_type() != F3Frame::F3FrameType::SYNC0) {
            qFatal("F3FrameToSection::state_section_complete(): Expected subcode sync0 frame at index 0, got something else!");
        }
        if (index == 1 && internal_buffer[index].get_f3_frame_type() != F3Frame::F3FrameType::SYNC1) {
            qFatal("F3FrameToSection::state_section_complete(): Expected subcode sync1 frame at index 1, got something else!");
        }

        if (index >= 2) {
            // Get the subcode byte from the F3 frame and set it in the subcode object
            subcode.set_subcode_byte(index, internal_buffer[index].get_subcode_byte());
        }
    }

    // Now we have 98 F3 Frames and the subcode, we can create a new section
    Section section;
    for (uint32_t index = 0; index < 98; index++) {
        F2Frame f2_frame;
        f2_frame.set_data(internal_buffer[index].get_data());
        
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
    valid_sections++;

    // Clean up for next section
    internal_buffer.clear();

    // All done, wait for the next sync0 frame
    return WAITING_FOR_SYNC0;
}

// In this state we have lost sync with the data stream.  This can happen if the
// data source is corrupt or if the data stream is not aligned correctly.  The 
// purpose of this state is to allow for statistics to be gathered on the number
// of times sync is lost.
F3FrameToSection::State F3FrameToSection::state_sync_lost() {
    qDebug() << "F3FrameToSection::state_sync_lost - Sync lost";
    sync_lost_count++;

    // Clean up
    internal_buffer.clear();

    return WAITING_FOR_INITIAL_SYNC0;
}

void F3FrameToSection::show_statistics() {
    qInfo() << "F3 frame to section statistics:";
    qInfo() << "  Sections:";
    qInfo() << "    Valid sections:" << valid_sections;
    qInfo() << "    Invalid sections:" << invalid_sections;
    qInfo() << "    Sync lost count:" << sync_lost_count;
    qInfo() << "  Section Frames:";
    qInfo() << "    Valid sync0 frames:" << valid_sync0_frames;
    qInfo() << "    Valid sync1 frames:" << valid_sync1_frames;
    qInfo() << "    Valid subcode frames:" << valid_subcode_frames;
    qInfo() << "    Invalid sync0 frames:" << invalid_sync0_frames;
    qInfo() << "    Invalid sync1 frames:" << invalid_sync1_frames;
    qInfo() << "    Invalid subcode frames:" << invalid_subcode_frames;
}