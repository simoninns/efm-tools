/************************************************************************

    decoders.cpp

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

// Note: perhaps the merging bits can be used to error check a little? Since 
// there are only 4 possible values, it should be easy to check the spacing
// between the data and figure out if a T-value is long or short?

#include <QVector>
#include <QQueue>
#include <QMap>
#include <QtGlobal>
#include <QDebug>

#include "decoders.h"
#include "section.h"

TvaluesToChannel::TvaluesToChannel() {
    invalid_t_values_count = 0;
    valid_t_values_count = 0;
}

void TvaluesToChannel::push_frame(QByteArray data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

QString TvaluesToChannel::pop_frame() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool TvaluesToChannel::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void TvaluesToChannel::process_queue() {
    // Process the input buffer
    QString bit_string;

    while (!input_buffer.isEmpty()) {
        QByteArray t_values = input_buffer.dequeue();

        for (int32_t i = 0; i < t_values.size(); i++) {
            // Convert the T-value to a bit string
            
            // Range check
            if (static_cast<int>(t_values[i]) > 11) {
                invalid_t_values_count++;
                t_values[i] = 11;
            } else if (static_cast<int>(t_values[i]) < 3) {
                invalid_t_values_count++;
                t_values[i] = 3;
            } else {
                valid_t_values_count++;
            }

            // T3 = 100, T4 = 1000, ... , T11 = 10000000000
            bit_string += "1";
            for (int32_t j = 1; j < t_values[i]; j++) {
                bit_string += "0";
            }   
        }
    }

    if (!bit_string.isEmpty()) {
        // Add the bit string to the output buffer
        output_buffer.enqueue(bit_string);
    }
}

ChannelToF3Frame::ChannelToF3Frame() {
    invalid_channel_frames_count = 0;
    valid_channel_frames_count = 0;
    internal_buffer.clear();
}

void ChannelToF3Frame::push_frame(QString data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

F3Frame ChannelToF3Frame::pop_frame() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool ChannelToF3Frame::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void ChannelToF3Frame::process_queue() {
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        // Append the input buffer to the internal object buffer
        internal_buffer.append(input_buffer.dequeue());

        // Check if the internal buffer is long enough to process
        // "long enough" means 588 bits plus the 24 bits of the next frame's sync header
        if (internal_buffer.size() > 588+24) {
            // Does the internal buffer contain at least 2 sync headers?
            if (internal_buffer.count(sync_header) >= 2) {
                // Find the first sync header
                int sync_header_index = internal_buffer.indexOf(sync_header);

                // Find the next sync header
                int next_sync_header_index = internal_buffer.indexOf(sync_header, sync_header_index+1);

                // Did we find the next sync header?
                if (next_sync_header_index != -1) {
                    // Extract the frame data
                    QString frame_data = internal_buffer.mid(sync_header_index, next_sync_header_index - sync_header_index);

                    if (frame_data.size() == 588) {
                        valid_channel_frames_count++;

                        // The channel frame data is:
                        //   Sync Header: 24 bits
                        //   Merging bits: 3 bits
                        //   Subcode: 14 bits
                        //   Merging bits: 3 bits
                        //   Then 32x
                        //     Data: 14 bits
                        //     Merging bits: 3 bits
                        //
                        // Giving a total of 588 bits

                        // Note: the subcode could be 256 or 257
                        uint16_t subcode = convert_efm_to_8bit(frame_data.mid(24+3, 14));

                        QVector<uint8_t> frame_data_bytes;
                        for (int i = 0; i < 32; i++) {
                            frame_data_bytes.append(convert_efm_to_8bit(frame_data.mid(24+3+14+3+(17*i), 14)));
                        }

                        // Create a new F3 frame
                        F3Frame f3_frame;
                        f3_frame.set_data(frame_data_bytes);
                        if (subcode == 256) {
                            f3_frame.set_frame_type_as_sync0();
                        } else if (subcode == 257) {
                            f3_frame.set_frame_type_as_sync1();
                        } else {
                            f3_frame.set_frame_type_as_subcode(subcode);
                        }

                        // Add the frame to the output buffer
                        output_buffer.enqueue(f3_frame);
                    } else {
                        qDebug() << "next_sync_header_index: " << next_sync_header_index;
                        qDebug() << "frame data: " << frame_data;
                        qFatal("ChannelToF3Frame::process_queue - Invalid frame size: %d", frame_data.size());
                        invalid_channel_frames_count++;
                    }

                    // Remove the frame data from the internal buffer
                    internal_buffer.remove(0, next_sync_header_index);
                } else {
                    // Couldn't find the second sync header... wait for more data
                    qDebug() << "ChannelToF3Frame::process_queue - Couldn't find the second sync header... waiting for more data";
                    break;
                }
            } else {
                // No initial sync header found, throw away the data (apart from the last 24 bits)
                qDebug() << "ChannelToF3Frame::process_queue - 2 sync headers not found... discarding 24 bytes of data";
                internal_buffer = internal_buffer.right(24);
                break;
            }
        }
    }
}

uint16_t ChannelToF3Frame::convert_efm_to_8bit(QString efm) {
    // Convert the EFM symbol to an 8-bit value
    return efm_lut.indexOf(efm);
}

// Define the 24-bit sync header for the F3 frame
const QString ChannelToF3Frame::sync_header = "100000000001000000000010";

// Note: The EFM LUT is a list of 257 EFM symbols, each represented as a 14-bit string
// The EFM symbols are indexed from 0 to 257 with the first 256 symbols representing
// the 8-bit data values from 0 to 255 and the last symbols representing the sync header
// sync0 and sync1 respectively.
const QStringList ChannelToF3Frame::efm_lut = {
    "01001000100000", "10000100000000", "10010000100000", "10001000100000",
    "01000100000000", "00000100010000", "00010000100000", "00100100000000",
    "01001001000000", "10000001000000", "10010001000000", "10001001000000",
    "01000001000000", "00000001000000", "00010001000000", "00100001000000",
    "10000000100000", "10000010000000", "10010010000000", "00100000100000",
    "01000010000000", "00000010000000", "00010010000000", "00100010000000",
    "01001000010000", "10000000010000", "10010000010000", "10001000010000",
    "01000000010000", "00001000010000", "00010000010000", "00100000010000",
    "00000000100000", "10000100001000", "00001000100000", "00100100100000",
    "01000100001000", "00000100001000", "01000000100000", "00100100001000",
    "01001001001000", "10000001001000", "10010001001000", "10001001001000",
    "01000001001000", "00000001001000", "00010001001000", "00100001001000",
    "00000100000000", "10000010001000", "10010010001000", "10000100010000",
    "01000010001000", "00000010001000", "00010010001000", "00100010001000",
    "01001000001000", "10000000001000", "10010000001000", "10001000001000",
    "01000000001000", "00001000001000", "00010000001000", "00100000001000",
    "01001000100100", "10000100100100", "10010000100100", "10001000100100",
    "01000100100100", "00000000100100", "00010000100100", "00100100100100",
    "01001001000100", "10000001000100", "10010001000100", "10001001000100",
    "01000001000100", "00000001000100", "00010001000100", "00100001000100",
    "10000000100100", "10000010000100", "10010010000100", "00100000100100",
    "01000010000100", "00000010000100", "00010010000100", "00100010000100",
    "01001000000100", "10000000000100", "10010000000100", "10001000000100",
    "01000000000100", "00001000000100", "00010000000100", "00100000000100",
    "01001000100010", "10000100100010", "10010000100010", "10001000100010",
    "01000100100010", "00000000100010", "01000000100100", "00100100100010",
    "01001001000010", "10000001000010", "10010001000010", "10001001000010",
    "01000001000010", "00000001000010", "00010001000010", "00100001000010",
    "10000000100010", "10000010000010", "10010010000010", "00100000100010",
    "01000010000010", "00000010000010", "00010010000010", "00100010000010",
    "01001000000010", "00001001001000", "10010000000010", "10001000000010",
    "01000000000010", "00001000000010", "00010000000010", "00100000000010",
    "01001000100001", "10000100100001", "10010000100001", "10001000100001",
    "01000100100001", "00000000100001", "00010000100001", "00100100100001",
    "01001001000001", "10000001000001", "10010001000001", "10001001000001",
    "01000001000001", "00000001000001", "00010001000001", "00100001000001",
    "10000000100001", "10000010000001", "10010010000001", "00100000100001",
    "01000010000001", "00000010000001", "00010010000001", "00100010000001",
    "01001000000001", "10000010010000", "10010000000001", "10001000000001",
    "01000010010000", "00001000000001", "00010000000001", "00100010010000",
    "00001000100001", "10000100001001", "01000100010000", "00000100100001",
    "01000100001001", "00000100001001", "01000000100001", "00100100001001",
    "01001001001001", "10000001001001", "10010001001001", "10001001001001",
    "01000001001001", "00000001001001", "00010001001001", "00100001001001",
    "00000100100000", "10000010001001", "10010010001001", "00100100010000",
    "01000010001001", "00000010001001", "00010010001001", "00100010001001",
    "01001000001001", "10000000001001", "10010000001001", "10001000001001",
    "01000000001001", "00001000001001", "00010000001001", "00100000001001",
    "01000100100000", "10000100010001", "10010010010000", "00001000100100",
    "01000100010001", "00000100010001", "00010010010000", "00100100010001",
    "00001001000001", "10000100000001", "00001001000100", "00001001000000",
    "01000100000001", "00000100000001", "00000010010000", "00100100000001",
    "00000100100100", "10000010010001", "10010010010001", "10000100100000",
    "01000010010001", "00000010010001", "00010010010001", "00100010010001",
    "01001000010001", "10000000010001", "10010000010001", "10001000010001",
    "01000000010001", "00001000010001", "00010000010001", "00100000010001",
    "01000100000010", "00000100000010", "10000100010010", "00100100000010",
    "01000100010010", "00000100010010", "01000000100010", "00100100010010",
    "10000100000010", "10000100000100", "00001001001001", "00001001000010",
    "01000100000100", "00000100000100", "00010000100010", "00100100000100",
    "00000100100010", "10000010010010", "10010010010010", "00001000100010",
    "01000010010010", "00000010010010", "00010010010010", "00100010010010",
    "01001000010010", "10000000010010", "10010000010010", "10001000010010",
    "01000000010010", "00001000010010", "00010000010010", "00100000010010",
    "00100000000001", "00000000010010"
};

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

SectionToF2Frame::SectionToF2Frame() {
    invalid_sections_count = 0;
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

F2FrameToF1Frame::F2FrameToF1Frame()
    : delay_line1({0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1}),
      delay_line2({0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2}),
      delay_lineM({108, 104, 100, 96, 92, 88, 84, 80, 76, 72, 68, 64, 60, 56, 52, 48, 44, 40, 36, 32, 28, 24, 20, 16, 12, 8, 4, 0}),
      invalid_f2_frames_count(0),
      valid_f2_frames_count(0) 
{}

void F2FrameToF1Frame::push_frame(F2Frame data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the queue
    process_queue();
}

F1Frame F2FrameToF1Frame::pop_frame() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool F2FrameToF1Frame::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

// Note: The F2 frames will not be correct until the delay lines are full
// So lead-in is required to prevent loss of the input date.  For now we will
// just discard the data until the delay lines are full.
void F2FrameToF1Frame::process_queue() {
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        F2Frame f2_frame = input_buffer.dequeue();
        QVector<uint8_t> data = f2_frame.get_data();

        data = delay_line1.push(data);
        if (data.isEmpty()) continue;

        // Process the data
        data = inverter.invert_parity(data);

        data = circ.c1_decode(data);

        data = delay_lineM.push(data);
        if (data.isEmpty()) continue;

        // Only perform C2 decode if delay line 1 is full and delay line M is full
        data = circ.c2_decode(data);

        data = interleave.deinterleave(data);

        data = delay_line2.push(data);
        if (data.isEmpty()) continue;

        // We will only get valid data if the delay lines are all full
        valid_f2_frames_count++;

        // Put the resulting data into an F1 frame and push it to the output buffer
        F1Frame f1_frame;
        f1_frame.set_data(data);
        f1_frame.set_frame_type(f2_frame.get_frame_type());
        f1_frame.set_frame_time(f2_frame.get_frame_time());
        f1_frame.set_track_number(f2_frame.get_track_number());
        valid_f2_frames_count++;
        output_buffer.enqueue(f1_frame);
    }
}

// Get the statistics for the C1 decoder
void F2FrameToF1Frame::get_c1_circ_stats(int32_t &valid_c1s, int32_t &fixed_c1s, int32_t &error_c1s) {
    valid_c1s = circ.get_valid_c1s();
    fixed_c1s = circ.get_fixed_c1s();
    error_c1s = circ.get_error_c1s();
}

// Get the statistics for the C2 decoder
void F2FrameToF1Frame::get_c2_circ_stats(int32_t &valid_c2s, int32_t &fixed_c2s, int32_t &error_c2s) {
    valid_c2s = circ.get_valid_c2s();
    fixed_c2s = circ.get_fixed_c2s();
    error_c2s = circ.get_error_c2s();
}

F1FrameToData24::F1FrameToData24() {
    invalid_f1_frames_count = 0;
    valid_f1_frames_count = 0;
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

        // ECMA-130 issue 2 page 16 - Clause 16
        // All byte pairs are swapped by the F1 Frame encoder
        for (int i = 0; i < data.size(); i += 2) {
            if (i + 1 < data.size()) {
                std::swap(data[i], data[i + 1]);
            }
        }

        // Put the resulting data into a Data24 frame and push it to the output buffer
        Data24 data24;
        data24.set_data(data);
        data24.set_frame_type(f1_frame.get_frame_type());
        data24.set_frame_time(f1_frame.get_frame_time());
        data24.set_track_number(f1_frame.get_track_number());

        // Add the data to the output buffer
        output_buffer.enqueue(data24);
        
        valid_f1_frames_count++;
    }
}