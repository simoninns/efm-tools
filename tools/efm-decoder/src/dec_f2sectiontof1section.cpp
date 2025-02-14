/************************************************************************

    dec_f2sectiontof1section.cpp

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

#include "dec_f2sectiontof1section.h"

F2SectionToF1Section::F2SectionToF1Section()
    : delay_line1({0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1}),
      delay_line2({0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2}),
      delay_lineM({108, 104, 100, 96, 92, 88, 84, 80, 76, 72, 68, 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0}),
      delay_line1_err({0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1}),
      delay_line2_err({0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2}),
      delay_lineM_err({108, 104, 100, 96, 92, 88, 84, 80, 76, 72, 68, 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0}),
      valid_input_f2_frames_count(0),
      invalid_input_f2_frames_count(0),
      invalid_output_f1_frames_count(0),
      valid_output_f1_frames_count(0),
      input_byte_errors(0),
      output_byte_errors(0),
      dl_lost_frames_count(0),
      last_frame_number(-1),
      continuity_error_count(0)
{}

void F2SectionToF1Section::push_section(F2Section f2_section) {
    // Add the data to the input buffer
    input_buffer.enqueue(f2_section);

    // Process the queue
    process_queue();
}

F1Section F2SectionToF1Section::pop_section() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool F2SectionToF1Section::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

// Note: The F2 frames will not be correct until the delay lines are full
// So lead-in is required to prevent loss of the input date.  For now we will
// just discard the data until the delay lines are full.
void F2SectionToF1Section::process_queue() {
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        F2Section f2_section = input_buffer.dequeue();
        F1Section f1_section;

        // Sanity check the F2 section
        if (f2_section.is_complete() == false) {
            qFatal("F2SectionToF1Section::process_queue - F2 Section is not complete");
        }

        // Check section continuity
        if (last_frame_number != -1) {
            if (f2_section.metadata.get_absolute_section_time().get_frames() != last_frame_number + 1) {
                qWarning() << "F2 Section continuity error last frame:" << last_frame_number << "current frame:" << f2_section.metadata.get_absolute_section_time().get_frames();
                qWarning() << "Last section time:" << f2_section.metadata.get_absolute_section_time().to_string();
                qWarning() << "This is a bug in the F2 Metadata correction and should be reported";
                continuity_error_count++;
            }
            last_frame_number = f2_section.metadata.get_absolute_section_time().get_frames();
        } else {
            last_frame_number = f2_section.metadata.get_absolute_section_time().get_frames();
        }

        for (int index = 0; index < 98; index++) {
            QVector<uint8_t> data = f2_section.get_frame(index).get_data();
            QVector<uint8_t> error_data = f2_section.get_frame(index).get_error_data();

            //if (show_debug) show_data(" F2 Input", index, f2_section.metadata.get_absolute_section_time().to_string(), data, error_data);

            // Check F2 frame for errors
            uint32_t in_frame_errors = f2_section.get_frame(index).count_errors();
            if (in_frame_errors == 0) valid_input_f2_frames_count++;
            else {
                invalid_input_f2_frames_count++;
                input_byte_errors += in_frame_errors;
            }

            data = delay_line1.push(data);
            error_data = delay_line1_err.push(error_data);
            if (data.isEmpty()) {
                // Output an empty F1 frame (ensures the section is complete)
                // Note: This isn't an error frame, it's just an empty frame
                F1Frame f1_frame;
                f1_frame.set_data(QVector<uint8_t>(24, 0));
                f1_section.push_frame(f1_frame);
                dl_lost_frames_count++;
                continue;
            }

            // Process the data
            // Note: We will only get valid data if the delay lines are all full
            data = inverter.invert_parity(data);

            //if (show_debug) show_data(" C1 Input", index, f2_section.metadata.get_absolute_section_time().to_string(), data, error_data);

            circ.c1_decode(data, error_data, show_debug);

            data = delay_lineM.push(data);
            error_data = delay_lineM_err.push(error_data);
            if (data.isEmpty()) {
                // Output an empty F1 frame (ensures the section is complete)
                // Note: This isn't an error frame, it's just an empty frame
                F1Frame f1_frame;
                f1_frame.set_data(QVector<uint8_t>(24, 0));
                f1_section.push_frame(f1_frame);
                dl_lost_frames_count++;
                continue;
            }

            if (show_debug) show_data(" C2 Input", index, f2_section.metadata.get_absolute_section_time().to_string(), data, error_data);

            // Only perform C2 decode if delay line 1 is full and delay line M is full
            circ.c2_decode(data, error_data, show_debug);

            if (show_debug) show_data("C2 Output", index, f2_section.metadata.get_absolute_section_time().to_string(), data, error_data);

            data = interleave.deinterleave(data);
            error_data = interleave_err.deinterleave(error_data);

            data = delay_line2.push(data);
            error_data = delay_line2_err.push(error_data);
            if (data.isEmpty()) {
                // Output an empty F1 frame (ensures the section is complete)
                // Note: This isn't an error frame, it's just an empty frame
                F1Frame f1_frame;
                f1_frame.set_data(QVector<uint8_t>(24, 0));
                f1_section.push_frame(f1_frame);
                dl_lost_frames_count++;
                continue;
            }

            //if (show_debug) show_data("F2 Output", index, f2_section.metadata.get_absolute_section_time().to_string(), data, error_data);

            // Put the resulting data (and error data) into an F1 frame and push it to the output buffer
            F1Frame f1_frame;
            f1_frame.set_data(data);
            f1_frame.set_error_data(error_data);
            
            // Check F1 frame for errors
            uint32_t out_frame_errors = f1_frame.count_errors();
            if (out_frame_errors == 0) valid_output_f1_frames_count++;
            else {
                invalid_output_f1_frames_count++;
                output_byte_errors += out_frame_errors;
            }

            f1_section.push_frame(f1_frame);
        }

        // All frames in the section are processed
        f1_section.metadata = f2_section.metadata;

        // Add the section to the output buffer
        output_buffer.enqueue(f1_section);
    }
}

void F2SectionToF1Section::show_data(QString description, int32_t index, QString time_string, QVector<uint8_t>& data, QVector<uint8_t>& data_error) {
    QString data_string;
    bool has_error = false;
    for (int i = 0; i < data.size(); ++i) {
        if (data_error[i] == 0) {
            data_string.append(QString("%1 ").arg(data[i], 2, 16, QChar('0')));
        } else {
            data_string.append(QString("XX "));
            has_error = true;
        }
    }

    // Display the data if there are errors
    if (has_error) {
        qDebug().nospace().noquote() << "F2SectionToF1Section - " << description
                           << "[" << QString("%1").arg(index, 2, 10, QChar('0'))
                           << "]: (" << time_string << ") " << data_string << "XX=ERROR";
    }
}

void F2SectionToF1Section::show_statistics() {
    qInfo() << "F2 Section to F1 Section statistics:";
    qInfo() << "  Input F2 Frames:";
    qInfo() << "    Valid frames:" << valid_input_f2_frames_count;
    qInfo() << "    Corrupt frames:" << invalid_input_f2_frames_count << "frames containing" << input_byte_errors << "byte errors";
    qInfo() << "    Delay line lost frames:" << dl_lost_frames_count;
    qInfo() << "    Continuity errors:" << continuity_error_count;
    
    qInfo() << "  Output F1 Frames (after CIRC):";
    qInfo() << "    Valid frames:" << valid_output_f1_frames_count;
    qInfo() << "    Corrupt frames:" << invalid_output_f1_frames_count;
    qInfo() << "    Output byte errors:" << output_byte_errors;

    qInfo() << "  C1 decoder:";
    qInfo() << "    Valid C1s:" << circ.get_valid_c1s();
    qInfo() << "    Fixed C1s:" << circ.get_fixed_c1s();
    qInfo() << "    Error C1s:" << circ.get_error_c1s();
    
    qInfo() << "  C2 decoder:";
    qInfo() << "    Valid C2s:" << circ.get_valid_c2s();
    qInfo() << "    Fixed C2s:" << circ.get_fixed_c2s();
    qInfo() << "    Error C2s:" << circ.get_error_c2s();
}