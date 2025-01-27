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