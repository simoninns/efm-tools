/************************************************************************

    encoders.cpp

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

#include <QVector>
#include <QQueue>
#include <QMap>
#include <QtGlobal>
#include <QDebug>

#include "encoders.h"

// Data24ToF1Frame class implementation
Data24ToF1Frame::Data24ToF1Frame() {
    valid_f1_frames_count = 0;
}

void Data24ToF1Frame::push_frame(Data24 data24) {
    input_buffer.enqueue(data24);
    process_queue();
}

F1Frame Data24ToF1Frame::pop_frame() {
    if (!is_ready()) {
        qFatal("Data24ToF1Frame::pop_frame(): No F1 frames are available.");
    }
    return output_buffer.dequeue();
}

void Data24ToF1Frame::process_queue() {
    while (!input_buffer.isEmpty()) {
        Data24 data24 = input_buffer.dequeue();

        // ECMA-130 issue 2 page 16 - Clause 16
        // All byte pairs are swapped by the F1 Frame encoder
        QVector<uint8_t> data = data24.get_data();
        for (int i = 0; i < data.size(); i += 2) {
            std::swap(data[i], data[i + 1]);
        }

        F1Frame f1_frame;
        f1_frame.set_data(data);
        f1_frame.set_frame_type(data24.get_frame_type());
        f1_frame.set_frame_time(data24.get_frame_time());
        f1_frame.set_track_number(data24.get_track_number());

        valid_f1_frames_count++;
        output_buffer.enqueue(f1_frame);
    }
}

bool Data24ToF1Frame::is_ready() const {
    return !output_buffer.isEmpty();
}

// F1FrameToF2Frame class implementation
F1FrameToF2Frame::F1FrameToF2Frame() 
    : delay_line1({1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0}),
      delay_line2({2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0}),
      delay_lineM({0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108})
{
    valid_f2_frames_count = 0;
}

void F1FrameToF2Frame::push_frame(F1Frame f1_frame) {
    input_buffer.enqueue(f1_frame);
    process_queue();
}

F2Frame F1FrameToF2Frame::pop_frame() {
    if (!is_ready()) {
        qFatal("F1FrameToF2Frame::pop_frame(): No F2 frames are available.");
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
void F1FrameToF2Frame::process_queue() {
    while (!input_buffer.isEmpty()) {
        // Pop the F1 frame and copy the data
        F1Frame f1_frame = input_buffer.dequeue();
        QVector<uint8_t> data = f1_frame.get_data();

        // Process the data
        data = delay_line2.push(data);
        if (data.isEmpty()) continue;

        data = interleave.interleave(data); // 24
        data = circ.c2_encode(data); // 24 + 4 = 28

        data = delay_lineM.push(data); // 28
        if (data.isEmpty()) continue;

        data = circ.c1_encode(data); // 28 + 4 = 32

        data = delay_line1.push(data); // 32     
        if (data.isEmpty()) continue;

        data = inverter.invert_parity(data); // 32

        // Put the resulting data into an F2 frame and push it to the output buffer
        F2Frame f2_frame;
        f2_frame.set_data(data);
        f2_frame.set_frame_type(f1_frame.get_frame_type());
        f2_frame.set_frame_time(f1_frame.get_frame_time());
        f2_frame.set_track_number(f1_frame.get_track_number());
        
        valid_f2_frames_count++;
        output_buffer.enqueue(f2_frame);
    }
}

bool F1FrameToF2Frame::is_ready() const {
    return !output_buffer.isEmpty();
}

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

            // Set the overall section information based on the first F2 frame of the section
            if (symbol == 0) {
                section.subcode.p_channel.set_flag(false);
                section.subcode.q_channel.set_q_mode_1(Qchannel::Control::AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED, f2_frame.get_track_number(),
                    f2_frame.get_frame_time(),
                    f2_frame.get_frame_time(), // Note: This sets absolute time to time, which isn't correct
                    f2_frame.get_frame_type());
            }

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

        for (uint32_t symbol_number = 0; symbol_number < 98; ++symbol_number) {
            F2Frame f2_frame = section.get_f2_frame(symbol_number);
            F3Frame f3_frame;

            if (symbol_number == 0) {
                f3_frame.set_frame_type_as_sync0();
            } else if (symbol_number == 1) {
                f3_frame.set_frame_type_as_sync1();
            } else {
                // Generate the subcode byte
                f3_frame.set_frame_type_as_subcode(section.get_subcode_byte(symbol_number));
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

// F3FrameToChannel class implementation
F3FrameToChannel::F3FrameToChannel(){
    // Output data is a string that represents bits
    // Since we are working with 14-bit EFM symbols, 3 bit merging symbols and
    // a 24 bit frame sync header - the output length of a channel frame is 588 bit (73.5 bytes)
    // so we can't use a QByteArray directly here without introducing unwanted padding
    dsv = 0;
    dsv_direction = true;
    total_t_values = 0;

    previous_channel_frame.clear();

    valid_channel_frames_count = 0;
}

void F3FrameToChannel::push_frame(F3Frame f3_frame) {
    input_buffer.enqueue(f3_frame);

    process_queue();
}

QVector<uint8_t> F3FrameToChannel::pop_frame() {
    if (!is_ready()) {
        qFatal("F3FrameToChannel::pop_frame(): No bytes are available.");
    }
    return output_buffer.dequeue();
}

void F3FrameToChannel::process_queue() {
    while (!input_buffer.isEmpty()) {
        // Pop the F3 frame data from the processing queue
        F3Frame f3_frame = input_buffer.dequeue();
        QVector<uint8_t> f3_frame_data = f3_frame.get_data();

        // Ensure the F3 frame data is 32 bytes long
        if (f3_frame_data.size() != 32) {
            qFatal("F3FrameToChannel::process_queue(): F3 frame data must be 24 bytes long.");
        }

        QString channel_frame;
        QString merging_bits = "xxx";
     
        // Sync header
        channel_frame += sync_header;
        channel_frame += merging_bits;

        // Pick the subcode value or a sync0/1 symbol based on the inputF3 frame type    
        if (f3_frame.get_f3_frame_type() == F3Frame::F3FrameType::SUBCODE) {
            channel_frame += convert_8bit_to_efm(f3_frame.get_subcode_byte());
        } else if (f3_frame.get_f3_frame_type() == F3Frame::F3FrameType::SYNC0) {
            channel_frame += convert_8bit_to_efm(256);
        } else {
            channel_frame += convert_8bit_to_efm(257);
        }
        channel_frame += merging_bits;

        // Now we output the actual F3 frame data
        for (uint32_t index = 0; index < f3_frame_data.size(); index++) {
            channel_frame += convert_8bit_to_efm(f3_frame_data[index]);
            channel_frame += merging_bits;
        }

        // Now we add the merging bits to the frame
        channel_frame = add_merging_bits(channel_frame);

        // Sanity check - Frame size should be 588 bits
        if (channel_frame.size() != 588) {
            qDebug() << "F3FrameToChannel::process_queue(): Channel frame:" << channel_frame;
            qFatal("F3FrameToChannel::process_queue(): BUG - Merged channel frame size is not 588 bits. It is %d bits", channel_frame.size());
        }

        // Sanity check - The frame should only contain one sync header
        if (channel_frame.count(sync_header) != 1) {
            qDebug() << "F3FrameToChannel::process_queue(): Channel frame:" << channel_frame;
            qFatal("F3FrameToChannel::process_queue(): BUG - Channel frame contains %d sync headers.", channel_frame.count(sync_header));
        }

        // Sanity check - This and the previous frame combined should contain only two sync headers (edge case detection)
        if (!previous_channel_frame.isEmpty()) {
            QString combined_frames = previous_channel_frame + channel_frame;
            if (combined_frames.count(sync_header) != 2) {
                qDebug() << "F3FrameToChannel::process_queue(): Previous frame:" << previous_channel_frame;
                qDebug() << "F3FrameToChannel::process_queue():  Current frame:" << channel_frame;
                qFatal("F3FrameToChannel::process_queue(): BUG - Previous and current channel frames combined contains more than two sync headers.");
            }
        }   

        // Flush the output data to the output buffer
        valid_channel_frames_count++;
        write_frame(channel_frame);
    }
}

void F3FrameToChannel::write_frame(QString channel_frame) {
    // Since the base class uses QVector<uint8_t> for the output buffer we have to as well
    QVector<uint8_t> output_bytes;

    // Check the input data size
    if (channel_frame.size() != 588) {
        qFatal("F3FrameToChannel::write_frame(): Channel frame size is not 588 bits.");
    }

    for (int i = 0; i < channel_frame.size(); ++i) {
        // Is the first bit a 1?
        if (channel_frame[i] == '1') {
            // Yes, so count the number of 0s until the next 1
            uint32_t zero_count = 0;
            for (uint32_t j = 1; i < channel_frame.size(); j++) {
                if (channel_frame[j + i] == '0') {
                    zero_count++;
                } else {
                    break;
                }
            }

            // The number of zeros is not between 2 and 10 - something is wrong in the input data
            if (zero_count < 2 || zero_count > 10) {
                qInfo() << "F3FrameToChannel::write_frame(): Channel frame:" << channel_frame;
                qInfo() << "F3FrameToChannel::write_frame(): Zero count:" << zero_count;  
                qFatal("F3FrameToChannel::write_frame(): Number of zeros between ones is not between 2 and 10.");
            }

            i += zero_count;

            // Append the T-value to the output bytes (the number of zeros plus 1)
            output_bytes.append(zero_count + 1);
            total_t_values++;
        } else {
            // First bit is zero... input data is invalid!
            qFatal("F3FrameToChannel::write_frame(): First bit should not be zero!");
        }
    }

    output_buffer.enqueue(output_bytes);
}

bool F3FrameToChannel::is_ready() const {
    return !output_buffer.isEmpty();
}

// Note: There are 257 EFM symbols: 0 to 255 and two additional sync0 and sync1 symbols
QString F3FrameToChannel::convert_8bit_to_efm(uint16_t value) {
    if (value < 258) {
        return efm_lut[value];
    } else {
        qFatal("F3FrameToChannel::convert_8bit_to_efm(): Value must be in the range 0 to 257.");
    }
}

QString F3FrameToChannel::add_merging_bits(QString channel_frame) {
    // Ensure the channel frame is 588 bits long
    if (channel_frame.size() != 588) {
        qFatal("F3FrameToChannel::add_merging_bits(): Channel frame is not 588 bits long.");
    }

    // We need to add another sync header to the channel frame
    // to make sure the last merging bit is correct
    QString merged_frame = channel_frame + sync_header;

    // The merging bits are represented by "xxx".  Each merging bit is preceded by and
    // followed by an EFM symbol.  Firstly we have to extract the preceeding and following
    // EFM symbols.

    // There are 34 sets of merging bits in the 588 bit frame
    for (int i = 0; i < 34; ++i) {
        QString current_efm;
        QString next_efm;

        // Set start to the next set of merging bits, the first merging bits are 24 bits in and then every 14 bits
        uint32_t start = 0;
        if (i == 0) {
            // The first EFM symbol is a sync header
            current_efm = sync_header;
            start = 24;
        } else {
            start = 24 + i * 17;

            // Extract the preceding EFM symbol
            current_efm = merged_frame.mid(start - 14, 14);
        }

        // Extract the following EFM symbol
        if (i == 33) {
            // If it's the last set of merging bits, the next EFM symbol is the sync header of the next frame
            next_efm = sync_header;
        } else {
            next_efm = merged_frame.mid(start + 3, 14);
        }

        if (next_efm.isEmpty()) qFatal("F3FrameToChannel::add_merging_bits(): Next EFM symbol is empty.");

        // Get a list of legal merging bit patterns
        QStringList merging_patterns = get_legal_merging_bit_patterns(current_efm, next_efm);

        // Order the patterns by the resulting DSV delta
        merging_patterns = order_patterns_by_dsv_delta(merging_patterns, current_efm, next_efm);

        // Pick the first pattern from the list that doesn't form a spurious sync_header pattern
        QString merging_bits;
        for (const QString& pattern : merging_patterns) {
            QString temp_frame = merged_frame;

            // Add our possible pattern to the frame
            temp_frame.replace(start, 3, pattern);

            // Any spurious sync headers generated?
            if (temp_frame.count(sync_header) == 2) {
                merging_bits = pattern;
                break;
            }
        }

        // This shouldn't happen, but it it does, let's get debug information :)
        if (merging_bits.isEmpty()) {
            qDebug() << "F3FrameToChannel::add_merging_bits(): No legal merging bit pattern found.";
            qDebug() << "F3FrameToChannel::add_merging_bits(): Possible patterns:" << merging_patterns;
            qDebug() << "F3FrameToChannel::add_merging_bits(): Merging bits start at:" << start;
            qDebug() << "F3FrameToChannel::add_merging_bits():" << merged_frame.mid(start - 24, 24) << "xxx" << merged_frame.mid(start + 3, 24);

            qFatal("F3FrameToChannel::add_merging_bits(): No legal merging bit pattern found - encode failed");
        }

        // Replace the "xxx" with the chosen merging bits
        merged_frame.replace(start, 3, merging_bits);
    }

    // Remove the last sync header
    merged_frame.chop(sync_header.size());

    // Verify the merged frame is 588 bits long
    if (merged_frame.size() != 588) {
        qFatal("F3FrameToChannel::add_merging_bits(): Merged frame is not 588 bits long.");
    }

    return merged_frame;
}

// Returns a list of merging bit patterns that don't violate the ECMA-130 rules
// of at least 2 zeros and no more than 10 zeros between ones
QStringList F3FrameToChannel::get_legal_merging_bit_patterns(const QString& current_efm, const QString& next_efm) {
    // Check that current_efm is at least 14 bits long
    if (current_efm.size() < 14) {
        qFatal("F3FrameToChannel::get_legal_merging_bit_patterns(): Current EFM symbol is too short.");
    }

    // Check that next_efm is at least 14 bits long
    if (next_efm.size() < 14) {
        qFatal("F3FrameToChannel::get_legal_merging_bit_patterns(): Next EFM symbol is too short.");
    }

    QStringList patterns;
    QStringList possible_patterns = {"000", "001", "010", "100"};
    for (const QString& pattern : possible_patterns) {
        QString combined = current_efm + pattern + next_efm;

        int max_zeros = 0;
        int min_zeros = std::numeric_limits<int>::max();
        int current_zeros = 0;
        bool inside_zeros = false;

        // Remove leading zeros from the combined string
        while (combined[0] == '0') {
            combined.remove(0, 1);
        }

        for (QChar ch : combined) {
            if (ch == '1') {
                if (inside_zeros) {
                    if (current_zeros > 0) {
                        max_zeros = qMax(max_zeros, current_zeros);
                        min_zeros = qMin(min_zeros, current_zeros);
                    }
                    current_zeros = 0;
                    inside_zeros = false;
                }
            } else if (ch == '0') {
                inside_zeros = true;
                current_zeros++;
            }
        }

        // If no zeros were found between ones
        if (min_zeros == std::numeric_limits<int>::max()) {
            min_zeros = 0;
        }

        // Check for illegal 11 pattern
        if (combined.contains("11")) {
            min_zeros = 0;
        }

        // The pattern is only considered if the combined string conforms to the "at least 2 zeros
        // and no more than 10 zeros between ones" rule
        if (min_zeros >= 2 && max_zeros <= 10) {
            patterns.append(pattern);
        }
    }

    return patterns;
}

// Accepts a list of patterns and orders them by DSV delta (lowest to highest)
QStringList F3FrameToChannel::order_patterns_by_dsv_delta(const QStringList& merging_patterns, const QString& current_efm, const QString& next_efm) {
    QStringList ordered_patterns;
    
    QMap<int32_t, QString> dsv_map;
    for (int i = 0; i < merging_patterns.size(); ++i) {
        QString combined = current_efm + merging_patterns[i] + next_efm;
        int32_t dsv_delta = calculate_dsv_delta(combined);
        dsv_map.insert(dsv_delta, merging_patterns[i]);
    }

    for (auto it = dsv_map.begin(); it != dsv_map.end(); ++it) {
        ordered_patterns.append(it.value());
    }

    return ordered_patterns;
}

// This function calculates the DSV delta for the input data i.e. the change in DSV
// if the data is used to generate a channel frame.
int32_t F3FrameToChannel::calculate_dsv_delta(const QString data) {
    // The DSV is based on transitions between pits and lands in the EFM data
    // rather than the number of 1s and 0s.

    // If the first value isn't a 1 - count the zeros until the next 1
    // If dsv_direction is true, we increment dsv by the number of zeros
    // If dsv_direction is false, we decrement dsv by the number of zeros
    // Then we flip dsv_direction and repeat until we run out of data

    int32_t dsv_delta = 0;

    for (int i = 0; i < data.size(); ++i) {
        if (data[i] == '1') {
            dsv_direction = !dsv_direction;
        } else {
            int zero_count = 0;
            while (i < data.size() && data[i] == '0') {
                zero_count++;
                i++;
            }
            if (dsv_direction) {
                dsv_delta += zero_count;
            } else {
                dsv_delta -= zero_count;
            }
            dsv_direction = !dsv_direction;
        }
    }

    return dsv_delta;
}

int32_t F3FrameToChannel::get_total_t_values() const {
    return total_t_values;
}

// Define the 24-bit sync header for the F3 frame
const QString F3FrameToChannel::sync_header = "100000000001000000000010";

// Note: The EFM LUT is a list of 257 EFM symbols, each represented as a 14-bit string
// The EFM symbols are indexed from 0 to 257 with the first 256 symbols representing
// the 8-bit data values from 0 to 255 and the last symbols representing the sync header
// sync0 and sync1 respectively.
const QStringList F3FrameToChannel::efm_lut = {
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
