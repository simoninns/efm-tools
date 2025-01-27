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
                        uint16_t subcode = efm.fourteen_to_eight(frame_data.mid(24+3, 14));

                        QVector<uint8_t> frame_data_bytes;
                        for (int i = 0; i < 32; i++) {
                            frame_data_bytes.append(efm.fourteen_to_eight(frame_data.mid(24+3+14+3+(17*i), 14)));
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