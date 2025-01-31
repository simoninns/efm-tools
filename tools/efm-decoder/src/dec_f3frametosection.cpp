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
    // Set the initial state
    current_state = WAITING_FOR_SYNC;

    valid_sections = 0;
    invalid_sections = 0;
    discarded_f3_frames = 0;
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
            case WAITING_FOR_SYNC:
                current_state = state_waiting_for_sync();
                break;

            case GATHER_SECTION_FRAMES:
                current_state = gather_section_frames();
                break;

            case PROCESS_SECTION:
                current_state = process_section();
                break;
        }
    }
}

// In this state we are waiting for the sync frame indicating the start
// of a new section.  We can accept either a sync0 or sync1 frame.
F3FrameToSection::State F3FrameToSection::state_waiting_for_sync() {
    // Clear the internal F3 frame buffer
    section_buffer.clear();

    // Pop a frame from the input buffer
    F3Frame f3_frame = input_buffer.dequeue();

    // Is the frame a sync0 frame?
    if (f3_frame.get_f3_frame_type() == F3Frame::F3FrameType::SYNC0) {
        //qDebug() << "F3FrameToSection::state_waiting_for_initial_sync(): Got initial sync0 frame";
        section_buffer.append(f3_frame);
        return GATHER_SECTION_FRAMES;
    }

    // If the frame a sync1 frame?
    if (f3_frame.get_f3_frame_type() == F3Frame::F3FrameType::SYNC1) {
        qDebug() << "F3FrameToSection::state_waiting_for_initial_sync(): Got initial sync1 frame, but sync0 was missing";

        if (section_buffer.size() == 0) {
            // Add a dummy sync0 frame
            F3Frame sync0_frame;
            sync0_frame.set_frame_type_as_sync0();

            section_buffer.append(sync0_frame); // Add a dummy sync0 frame
        } else {
            // Mark the previous frame as sync0
            section_buffer[section_buffer.size()-1].set_frame_type_as_sync0();
        }

        // Now append the sync1 frame
        section_buffer.append(f3_frame);
        return GATHER_SECTION_FRAMES;
    }

    // Keep waiting for the sync frame
    qDebug() << "F3FrameToSection::state_waiting_for_initial_sync(): Discarding frame as not sync0 or sync1";
    discarded_f3_frames++; // Number of leading frames before we get a first sync frame
    return WAITING_FOR_SYNC;
}

F3FrameToSection::State F3FrameToSection::gather_section_frames() {
    // Pop a frame from the input buffer
    F3Frame f3_frame = input_buffer.dequeue();

    if (f3_frame.get_f3_frame_type() == F3Frame::F3FrameType::SUBCODE) {
        //qDebug() << "F3FrameToSection::gather_section_frames(): Got subcode frame number" << section_buffer.size();
        section_buffer.append(f3_frame);
        return GATHER_SECTION_FRAMES;
    }

    if (f3_frame.get_f3_frame_type() == F3Frame::F3FrameType::SYNC0) {
        //qDebug() << "F3FrameToSection::gather_section_frames(): Got sync0 frame (end of section) with" << section_buffer.size() << "frames";
        // Push the F3 frame back into the input buffer
        input_buffer.append(f3_frame); // Push it back onto the section buffer
        return PROCESS_SECTION;
    }

    if (f3_frame.get_f3_frame_type() == F3Frame::F3FrameType::SYNC1 && section_buffer.size() > 0) {
        if (section_buffer[section_buffer.size()-1].get_f3_frame_type() != F3Frame::F3FrameType::SYNC0) {
            qDebug() << "F3FrameToSection::gather_section_frames(): Got sync1 frame with no preceeding sync0 frame (end of section) with" << section_buffer.size() << "frames";

            // Mark the preceding flag as sync0
            if (section_buffer.size() > 0) {
                section_buffer[section_buffer.size()-1].set_frame_type_as_sync0();
            } else {
                qFatal("F3FrameToSection::gather_section_frames(): Got sync1 frame with no preceeding sync0 frame, but buffer was empty?");
            }

            // Push the sync0 F3 frame back into the input buffer
            input_buffer.append(section_buffer[section_buffer.size()-1]);

            // Push the sync1 F3 frame back into the input buffer
            input_buffer.append(f3_frame);

            // Remove the sync0 frame from the section buffer
            section_buffer.remove(section_buffer.size()-1);

            // Sanity check
            if (section_buffer.size() != 98) {
                qFatal("F3FrameToSection::gather_section_frames(): After a sync1 with no sync0, the section buffer should contain 98 frames - could be a bug?");
            }

            return PROCESS_SECTION;
        }

        // Valid sync1 frame - carry on gathering frames
        //qDebug() << "F3FrameToSection::gather_section_frames(): Got sync1 frame";
        section_buffer.append(f3_frame);
        return GATHER_SECTION_FRAMES;
    }

    // Check for overflow
    if (section_buffer.size() > 98) {
        // Overflow - discard the section
        discarded_f3_frames += section_buffer.size();
        qDebug() << "F3FrameToSection::gather_section_frames(): Section buffer overflow - discarding" << section_buffer.size() << "F3 frames";
        section_buffer.clear();
        return WAITING_FOR_SYNC;
    }

    qFatal("F3FrameToSection::gather_section_frames(): Frame is not sync0, sync1, or subcode - this is a bug");
    return WAITING_FOR_SYNC;
}

F3FrameToSection::State F3FrameToSection::process_section() {
    // We must have at least 98 frames to process a section
    if (section_buffer.size() >= 98) {
        //qDebug() << "F3FrameToSection::process_section(): Processing section buffer containing" << section_buffer.size() << "F3 frames";

        while (section_buffer.size() >= 98) {
            // Process each section
            QVector<F3Frame> section_frames = section_buffer.mid(0, 98); // Get the first 98 frames
            section_buffer = section_buffer.mid(98); // Remove the first 98 frames from the section buffer

            // Verify first two frames are sync0 and sync1
            if (section_frames[0].get_f3_frame_type() != F3Frame::F3FrameType::SYNC0) {
                qDebug() << "F3FrameToSection::process_section(): First frame is not sync0";
                section_frames[0].set_frame_type_as_sync0();
            }

            if (section_frames[1].get_f3_frame_type() != F3Frame::F3FrameType::SYNC1) {
                qDebug() << "F3FrameToSection::process_section(): Second frame is not sync1";
                section_frames[1].set_frame_type_as_sync1();
            }

            // Sanity check - the remaining frames should be subcode frames
            for (int i = 2; i < 98; i++) {
                if (section_frames[i].get_f3_frame_type() != F3Frame::F3FrameType::SUBCODE) {
                    qDebug() << "F3FrameToSection::process_section(): Frame" << i << "is not subcode";
                    //section_frames[i].set_frame_type_as_subcode(0);
                    qFatal("F3FrameToSection::process_section(): Frame is not subcode - this is a bug");
                }
            }

            // Generate the subcode
            Subcode subcode;
            for (int i = 2; i < 98; i++) {
                subcode.set_subcode_byte(i, section_frames[i].get_subcode_byte());
            }

            // Check the validity of the subcode
            if (!subcode.is_valid()) {
                
            }

            // Now we have 98 F3 Frames and a valid subcode, we can create a new section
            Section section;
            for (uint32_t index = 0; index < 98; index++) {
                F2Frame f2_frame;
                f2_frame.set_data(section_frames[index].get_data());
                
                // Now we have to extract the section data and add it to the F2 frame
                // Note: The F2 Frame doesn't support AP time at the moment - to-do
                f2_frame.set_frame_type(subcode.q_channel.get_frame_type());
                f2_frame.set_track_number(subcode.q_channel.get_track_number());
                f2_frame.set_frame_time(subcode.q_channel.get_frame_time());
                
                // Check the section's subcode data is valid
                if (subcode.q_channel.is_valid()) {
                    valid_sections++;
                    f2_frame.set_subcode_data_valid(true);
                } else {
                    invalid_sections++;
                    qDebug() << "F3FrameToSection::process_section(): Subcode data is invalid";
                    f2_frame.set_subcode_data_valid(false);
                }

                section.push_frame(f2_frame);
            }

            // Copy the subcode object to the section
            section.subcode = subcode;

            // Add the section to the output buffer
            output_buffer.enqueue(section);
            
            //qDebug() << "F3FrameToSection::process_section(): Section processed - section added to output buffer";
        }

        // All sections are processed, do we have any remaining frames?
        if (section_buffer.size() > 0) {
            qDebug() << "F3FrameToSection::process_section(): Remaining frames in section buffer - discarding" << section_buffer.size() << "frames";
            discarded_f3_frames += section_buffer.size();
            section_buffer.clear();
        }
    }

    // Wait for the next sync
    return WAITING_FOR_SYNC;
}

void F3FrameToSection::show_statistics() {
    qInfo() << "F3 frame to section statistics:";
    qInfo() << "  Sections:";
    qInfo() << "    Valid sections:" << valid_sections;
    qInfo() << "    Invalid sections:" << invalid_sections;
    qInfo() << "  F3 Frames:";
    qInfo() << "    Discarded F3 frames:" << discarded_f3_frames;
}