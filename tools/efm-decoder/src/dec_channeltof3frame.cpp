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

    // Set the initial state
    current_state = WAITING_FOR_INITIAL_SYNC;
    sync_lost_count = 0;
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
    // We shift the new data into the internal buffer
    // as there is usually old unprocessed data in the internal buffer
    // which needs to be processed first
    internal_buffer.append(input_buffer.dequeue());
    process_state_machine();
}

void ChannelToF3Frame::process_state_machine() {
    // Check if the internal buffer is long enough to process
    // "long enough" means 588 bits plus the 24 bits of the next frame's sync header
    while(internal_buffer.size() > 588+24) {
        switch(current_state) {
            case WAITING_FOR_INITIAL_SYNC:
                current_state = state_waiting_for_initial_sync();
                break;

            case WAITING_FOR_SYNC:
                current_state = state_waiting_for_sync();
                break;

            case PROCESS_FRAME:
                current_state = state_processing_frame();
                break;

            case SYNC_LOST:
                current_state = state_sync_lost();
                break;
        }
    }
}

// This function processes the state where we are waiting for the sync header
// after sync is lost (or initial sync has never been found).  As there can be a lot of 
// random data at the start (or in other places if the data source skipped or was
// damaged), this state is used to throw away data until a sync header is found.
ChannelToF3Frame::State ChannelToF3Frame::state_waiting_for_initial_sync() {
    // Does the internal buffer contain a sync header?
    if (internal_buffer.contains(sync_header)) {
        // Remove any data before the sync header
        internal_buffer = internal_buffer.right(internal_buffer.size() - internal_buffer.indexOf(sync_header));
        qDebug() << "ChannelToF3Frame::state_waiting_for_initial_sync - Found initial F3 sync header";
        return WAITING_FOR_SYNC;
    }

    // No initial sync header present in the input data
    // Discard the data (apart from the last 24 bits) and
    // wait for more data
    internal_buffer = internal_buffer.right(sync_header.size());
    qDebug() << "ChannelToF3Frame::state_waiting_for_initial_sync - Initial F3 sync header not found - throwing away data";
    return WAITING_FOR_INITIAL_SYNC;
}

// In this state we have already got a sync header (either the initial header or
// a subsequent header) and we are waiting for the next sync header.  This state
// allows for the fact that, in between sync headers, the data might be corrupted
// or missing due to source conditions or defects.  Although we expect 588 bits
// of data between from the start of one sync header to the start of the next, we
// allow for the possibility that the data might be a little longer or shorter due
// to errors.  If there is too much data before seeing a sync header we will throw
// away the data and start again in the "waiting for initial sync" state.
ChannelToF3Frame::State ChannelToF3Frame::state_waiting_for_sync() {
    // Does the internal buffer contain another sync header?
    if (internal_buffer.count(sync_header) == 2) {
        // Where is the second sync header?
        uint32_t second_sync_header_index = internal_buffer.indexOf(sync_header, sync_header.size());

        // Extract the frame data from the beginning of the internal buffer to the second sync header
        frame_data = internal_buffer.left(second_sync_header_index);

        // Remove the extracted frame data from the internal buffer
        internal_buffer = internal_buffer.right(internal_buffer.size() - second_sync_header_index);

        // Is the frame data a reasonable length?
        if (frame_data.size() > 580 && frame_data.size() < 590+24) {
            return PROCESS_FRAME;
        }

        // The frame data is not a reasonable length
        qDebug() << "ChannelToF3Frame::state_waiting_for_sync - Frame data not a reasonable length: " << frame_data.size();
        return WAITING_FOR_INITIAL_SYNC;
    }

    // Have we lost sync?
    if (internal_buffer.size() > 590) {
        qDebug() << "ChannelToF3Frame::state_waiting_for_sync - no sync after 590 bits";
        // Discard the data (apart from the last 24 bits) and
        // wait for more data
        internal_buffer = internal_buffer.right(sync_header.size());

        return SYNC_LOST;
    }

    // Keep waiting for the sync header
    return WAITING_FOR_SYNC;
}

// In this state we have received a channel frame which is reasonable in length
// and we can now process the channel frame into an F3 frame.
ChannelToF3Frame::State ChannelToF3Frame::state_processing_frame() {
    //qDebug() << "ChannelToF3Frame::state_processing_frame - Processing frame of size: " << frame_data.size();

    // Count the number of valid and invalid channel frames based on length
    if (frame_data.size() == 588) valid_channel_frames_count++;
    else invalid_channel_frames_count++;

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

    // Note: the subcode could be 256 (sync0 symbol) or 257 (sync1 symbol)
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

    return WAITING_FOR_SYNC;
}

// In this state we have lost sync with the data stream.  This can happen if the
// data source is corrupt or if the data stream is not aligned correctly.  The 
// purpose of this state is to allow for statistics to be gathered on the number
// of times sync is lost.
ChannelToF3Frame::State ChannelToF3Frame::state_sync_lost() {
    qDebug() << "ChannelToF3Frame::state_sync_lost - Sync lost";
    sync_lost_count++;
    return WAITING_FOR_INITIAL_SYNC;
}

void ChannelToF3Frame::show_statistics() {
    qInfo() << "Channel to F3 frame statistics:";
    qInfo() << "  Valid channel frames:" << valid_channel_frames_count;
    qInfo() << "  Invalid channel frames:" << invalid_channel_frames_count;
    qInfo() << "  Sync lost count:" << sync_lost_count;
}