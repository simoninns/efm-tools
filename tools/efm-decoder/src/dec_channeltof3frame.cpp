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
    current_state = EXPECTING_SYNC;

    // Reset statistics
    valid_channel_frames_count = 0;
    overshoot_channel_frames_count = 0;
    undershoot_channel_frames_count = 0;
    discarded_bits_count = 0;

    valid_efm_symbols_count = 0;
    invalid_efm_symbols_count = 0;

    // State machine tracking variables
    missed_sync = 0;
}

void ChannelToF3Frame::push_frame(QString data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the state machine
    process_state_machine();
}

F3Frame ChannelToF3Frame::pop_frame() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool ChannelToF3Frame::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void ChannelToF3Frame::process_state_machine() {
    // Add the input data to the internal frame buffer
    internal_buffer.append(input_buffer.dequeue());

    // Keep processing the state machine until the frame buffer
    // doesn't have enough data to process (we always want a full frame
    // plus the next sync header's worth of data, so 588+24 bits)
    while(internal_buffer.size() > 588+24) {
        switch(current_state) {
            case EXPECTING_SYNC:
                current_state = expecting_sync();
                break;

            case EXPECTING_DATA:
                current_state = expecting_data();
                break;

            case PROCESS_FRAME:
                current_state = process_frame();
                break;
        }
    }
}

ChannelToF3Frame::State ChannelToF3Frame::expecting_sync() {
    State next_state = EXPECTING_SYNC;

    // Does the internal buffer contain a sync header?
    if (internal_buffer.contains(sync_header)) {
        //qDebug() << "ChannelToF3Frame::expecting_sync - Sync header found";

        // Remove any data before the sync header (including the actual sync header itself)
        uint32_t original_frame_buffer_size = internal_buffer.size();
        internal_buffer = internal_buffer.mid(internal_buffer.indexOf(sync_header) + sync_header.size());
        uint32_t preceeding_bits_count = original_frame_buffer_size - internal_buffer.size() - 24;
        discarded_bits_count += preceeding_bits_count;
        if (preceeding_bits_count > 0) {
            qDebug() << "ChannelToF3Frame::expecting_sync - Discarding" << preceeding_bits_count << "bits before sync header";
        }
        
        next_state = EXPECTING_DATA;
    } else {
        // We are out of sync, discard all but the last 24 bits of the internal buffer
        uint32_t original_frame_buffer_size = internal_buffer.size();
        internal_buffer = internal_buffer.right(24);
        discarded_bits_count += original_frame_buffer_size - internal_buffer.size();
        qDebug() << "ChannelToF3Frame::expecting_sync - Sync header not found - throwing away" << original_frame_buffer_size - internal_buffer.size() << "bits";

        next_state = EXPECTING_SYNC;
    }

    return next_state;
}

ChannelToF3Frame::State ChannelToF3Frame::expecting_data() {
    State next_state = EXPECTING_DATA;

    // Does the internal buffer contain enough data for a frame?
    if (internal_buffer.size() < 564+24) {
        // Wait for more data
        next_state = EXPECTING_DATA;
    } else {
        // There is enough data for a frame, but is there a terminating sync header?
        // Note: Here we look at the sync header position once, as it takes quite a bit of processing
        // we also use CaseSensitive to avoid the overhead of converting the data to lower case
        uint32_t sync_header_index = internal_buffer.indexOf(sync_header, 0, Qt::CaseSensitive);
        if (sync_header_index != -1) {
            //qDebug() << "ChannelToF3Frame::expecting_data - Terminating sync header found at position" << internal_buffer.indexOf(sync_header);
            // There is a terminating sync header
            frame_data = internal_buffer.left(sync_header_index);

            // Remove the data from the internal buffer
            internal_buffer = internal_buffer.right(internal_buffer.size() - sync_header_index);

            // Clear the missing sync counter
            missed_sync = 0;

            // Process the frame
            next_state = PROCESS_FRAME;
        } else {
            qDebug() << "ChannelToF3Frame::expecting_data - Sync header not found - using 564 bits of data";
            // There is no terminating sync header, but there's enough data for a frame
            // Assume the sync header is missing or corrupt and take 564 bits of data
            frame_data = internal_buffer.left(564); // 588 bits - 24 bits = 564 bits

            // Remove the first 546 bits of the data from the internal buffer
            internal_buffer = internal_buffer.right(internal_buffer.size() - 564);

            // Increment the missing sync counter
            missed_sync++;

            // Process the frame
            next_state = PROCESS_FRAME;
        }
    }

    return next_state;
}

ChannelToF3Frame::State ChannelToF3Frame::process_frame() {
    State next_state = PROCESS_FRAME;

    // Do we have 564 bits of data?
    if (frame_data.size() == 564) {
        //qDebug() << "ChannelToF3Frame::process_frame - Processing 564 bit frame";

        // Count the number of valid channel frames
        valid_channel_frames_count++;

        // Create an F3 frame
        F3Frame f3_frame = convert_frame_data_to_f3_frame(frame_data);
        
        // Add the frame to the output buffer
        output_buffer.enqueue(f3_frame);
    } else {
        // The frame data is not 564 bits...
        if (frame_data.size() > 564) {
            overshoot_channel_frames_count++;
            // The frame data is too long, discard the extra bits
            discarded_bits_count += frame_data.size() - 564;
            qDebug() << "ChannelToF3Frame::process_frame - Frame data is too long - expected 564 bits, got" << frame_data.size() << "bits";

            // Create an F3 frame from the first 564 bits
            F3Frame f3_frame = convert_frame_data_to_f3_frame(frame_data.left(564));
            output_buffer.enqueue(f3_frame); // Add the frame to the output buffer

            // Remove the left-most 564+24 bits from frame_data
            frame_data = frame_data.mid(564 + 24);

            // If it's a reasonable size, treat it as a second frame
            if (frame_data.size() == 564) {
                F3Frame f3_frame = convert_frame_data_to_f3_frame(frame_data);
                output_buffer.enqueue(f3_frame); // Add the frame to the output buffer
                qDebug() << "ChannelToF3Frame::process_frame - Split too long data - Processing 564 bit frame from remaining data";
            } else {
                if (frame_data.size() > 0) {
                    // The remaining data is not a reasonable size, discard it
                    discarded_bits_count += frame_data.size();
                    qDebug() << "ChannelToF3Frame::process_frame - Discarding" << frame_data.size() << "bits of data (too long)";
                } else {
                    qDebug() << "ChannelToF3Frame::process_frame - No data left";
                }
            }

        } else {
            undershoot_channel_frames_count++;
            // The frame data is too short, pad with zeros to 564 bits
            uint8_t padding_bits = 564 - frame_data.size();
            frame_data.append(QString(564 - frame_data.size(), '0'));
            qDebug() << "ChannelToF3Frame::process_frame - Frame data is too short - padding with" << padding_bits << "zero bits";

            // Create an F3 frame
            F3Frame f3_frame = convert_frame_data_to_f3_frame(frame_data);
            frame_data.clear();

            // Add the frame to the output buffer
            output_buffer.enqueue(f3_frame);
        }
    }

    // Clear the internal buffer
    frame_data.clear();

    if (missed_sync > 0 && missed_sync < 3) {
        // Terminating sync header was missing, start collecting data again
        qDebug() << "ChannelToF3Frame::process_frame - Terminating sync header missing - resyncing (missed syncs:" << missed_sync << ")";

        // Remove the first 24 bits of the internal buffer (the quite-possibly-corrupted sync header)
        internal_buffer = internal_buffer.right(internal_buffer.size() - sync_header.size());

        next_state = EXPECTING_DATA;
    } else {
        // Wait for the next sync
        missed_sync = 0;
        next_state = EXPECTING_SYNC;
    }

    return next_state;
}

F3Frame ChannelToF3Frame::convert_frame_data_to_f3_frame(const QString frame_data) {
    // The channel frame data is:
    //   Sync Header: 24 bits (removed already)
    //   Merging bits: 3 bits
    //   Subcode: 14 bits
    //   Merging bits: 3 bits
    //   Then 32x
    //     Data: 14 bits
    //     Merging bits: 3 bits
    //
    // Giving a total of 588 bits

    // Note: the subcode could be 256 (sync0 symbol) or 257 (sync1 symbol)
    uint16_t subcode = efm.fourteen_to_eight(frame_data.mid(3, 14));
    if (subcode == 300) {
        subcode = 0; // Invalid subcode, treat as 0
        invalid_efm_symbols_count++;
    } else valid_efm_symbols_count++;

    QVector<uint8_t> frame_data_bytes;
    for (int i = 0; i < 32; i++) {
        uint16_t data = efm.fourteen_to_eight(frame_data.mid(3+14+3+(17*i), 14));

        if (data == 300) {
            data = 0; // Invalid subcode, treat as 0
            invalid_efm_symbols_count++;
        } else valid_efm_symbols_count++;

        frame_data_bytes.append(data);
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

    return f3_frame;
}

void ChannelToF3Frame::show_statistics() {
    qInfo() << "Channel to F3 frame statistics:";
    qInfo() << "  Channel Frames:";
    qInfo() << "    Valid:" << valid_channel_frames_count;
    qInfo() << "    Overshoot:" << overshoot_channel_frames_count;
    qInfo() << "    Undershoot:" << undershoot_channel_frames_count;
    qInfo() << "    Discarded bits:" << discarded_bits_count;
    qInfo() << "  EFM symbols:";
    qInfo() << "    Valid EFM symbols:" << valid_efm_symbols_count;
    qInfo() << "    Invalid EFM symbols:" << invalid_efm_symbols_count;
}