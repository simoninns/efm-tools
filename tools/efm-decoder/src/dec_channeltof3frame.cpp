/************************************************************************

    dec_channeltof3frame.cpp

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

#include "dec_channeltof3frame.h"

ChannelToF3Frame::ChannelToF3Frame() {
    // Statistics
    good_frames = 0;
    undershoot_frames = 0;
    overshoot_frames = 0;

    valid_efm_symbols = 0;
    invalid_efm_symbols = 0;

    valid_subcode_symbols = 0;
    invalid_subcode_symbols = 0;
}

void ChannelToF3Frame::push_frame(QByteArray data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process queue
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
    while(input_buffer.size() > 0) {
        // Extract the first item in the input buffer
        QByteArray frame_data = input_buffer.dequeue();

        // Count the number of bits in the frame
        int bit_count = 0;
        for (int i = 0; i < frame_data.size(); i++) {
            bit_count += frame_data.at(i);
        }

        // Generate statistics
        if (bit_count != 588 && show_debug) qDebug() << "ChannelToF3Frame::process_queue() - Frame data is" << bit_count << "bits (should be 588)";
        if (bit_count == 588) good_frames++;
        if (bit_count < 588) undershoot_frames++;
        if (bit_count > 588) overshoot_frames++;

        // Create an F3 frame
        F3Frame f3_frame = create_f3_frame(frame_data);

        // Place the frame into the output buffer
        output_buffer.enqueue(f3_frame);
    }
}

F3Frame ChannelToF3Frame::create_f3_frame(QByteArray t_values) {
    F3Frame f3_frame;

    // The channel frame data is:
    //   Sync Header: 24 bits (bits 0-23)
    //   Merging bits: 3 bits (bits 24-26)
    //   Subcode: 14 bits (bits 27-40)
    //   Merging bits: 3 bits (bits 41-43)
    //   Then 32x 17-bit data values (bits 44-587)
    //     Data: 14 bits
    //     Merging bits: 3 bits
    //
    // Giving a total of 588 bits

    // Convert the T-values to data
    QByteArray frame_data = tvalues_to_data(t_values);

    // Extract the subcode in bits 27-40
    uint16_t subcode = efm.fourteen_to_eight(get_bits(frame_data, 27, 40));
    if (subcode == 300) {
        subcode = 0;
        invalid_subcode_symbols++;
    } else valid_subcode_symbols++;

    // Extract the data values in bits 44-587 ignoring the merging bits
    QVector<uint8_t> data_values;
    QVector<uint8_t> error_values;
    for (int i = 44; i < 588; i += 17) {
        uint16_t data_value = efm.fourteen_to_eight(get_bits(frame_data, i, i + 13));

        if (data_value < 256) {
            data_values.append(data_value);
            error_values.append(0);
            valid_efm_symbols++;
        } else {
            data_values.append(0);
            error_values.append(1);
            invalid_efm_symbols++;
        }
    }

    // Create an F3 frame...
    
    // Determine the frame type
    if (subcode == 256) f3_frame.set_frame_type_as_sync0();
    else if (subcode == 257) f3_frame.set_frame_type_as_sync1();
    else f3_frame.set_frame_type_as_subcode(subcode);

    // Set the frame data
    f3_frame.set_data(data_values);
    f3_frame.set_error_data(error_values);

    return f3_frame;
}

QByteArray ChannelToF3Frame::tvalues_to_data(QByteArray t_values) {
    QByteArray output_data;
    uint8_t currentByte = 0; // Will accumulate bits until we have 8.
    uint32_t bitsFilled = 0;     // How many bits are currently in currentByte.

    // Iterate through each T-value in the input.
    for (uint8_t c : t_values) {
        // Convert char to int (assuming the T-value is in the range 3..11)
        int tValue = static_cast<uint8_t>(c);
        if (tValue < 3 || tValue > 11) {
            qFatal("ChannelToF3Frame::tvalues_to_data(): T-value must be in the range 3 to 11.");
        }
        
        // First, append the leading 1 bit.
        currentByte = (currentByte << 1) | 1;
        ++bitsFilled;
        if (bitsFilled == 8) {
            output_data.append(currentByte);
            currentByte = 0;
            bitsFilled = 0;
        }
        
        // Then, append (t_value - 1) zeros.
        for (int i = 1; i < tValue; ++i) {
            currentByte = (currentByte << 1); // Append a 0 bit.
            ++bitsFilled;
            if (bitsFilled == 8) {
                output_data.append(currentByte);
                currentByte = 0;
                bitsFilled = 0;
            }
        }
    }
    
    // If there are remaining bits, pad the final byte with zeros on the right.
    if (bitsFilled > 0) {
        currentByte <<= (8 - bitsFilled);
        output_data.append(currentByte);
    }
    
    return output_data;
}

uint16_t ChannelToF3Frame::get_bits(QByteArray data, int start_bit, int end_bit) {
    uint16_t value = 0;

    // Make sure start and end bits are within a 588 bit range
    if (start_bit < 0 || start_bit > 587) {
        qFatal("ChannelToF3Frame::get_bits(): Start bit must be in the range 0 to 587.");
    }

    if (end_bit < 0 || end_bit > 587) {
        qFatal("ChannelToF3Frame::get_bits(): End bit must be in the range 0 to 587.");
    }

    if (start_bit > end_bit) {
        qFatal("ChannelToF3Frame::get_bits(): Start bit must be less than or equal to the end bit.");
    }

    // Extract the bits from the data
    for (int bit = start_bit; bit <= end_bit; ++bit) {
        int byte_index = bit / 8;
        int bit_index = bit % 8;
        if (data[byte_index] & (1 << (7 - bit_index))) {
            value |= (1 << (end_bit - bit));
        }
    }

    return value;
}

void ChannelToF3Frame::show_statistics() {
    qInfo() << "Channel to F3 Frame statistics:";
    qInfo() << "  Channel Frames:";
    qInfo() << "    Total:" << good_frames + undershoot_frames + overshoot_frames;
    qInfo() << "    Good:" << good_frames;
    qInfo() << "    Undershoot:" << undershoot_frames;
    qInfo() << "    Overshoot:" << overshoot_frames;
    qInfo() << "  EFM symbols:";
    qInfo() << "    Valid:" << valid_efm_symbols;
    qInfo() << "    Invalid:" << invalid_efm_symbols;
    qInfo() << "  Subcode symbols:";
    qInfo() << "    Valid:" << valid_subcode_symbols;
    qInfo() << "    Invalid:" << invalid_subcode_symbols;
}