/************************************************************************

    dec_tvaluestochannel.cpp

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

#include "dec_tvaluestochannel.h"

TvaluesToChannel::TvaluesToChannel() {
    // Statistics
    consumed_t_values = 0;
    discarded_t_values = 0;
    channel_frame_count = 0;

    perfect_frames = 0;
    long_frames = 0;
    short_frames = 0;

    overshoot_syncs = 0;
    undershoot_syncs = 0;
    perfect_syncs = 0;

    // Set the initial state
    current_state = EXPECTING_INITIAL_SYNC;
}

void TvaluesToChannel::push_frame(QByteArray data) {
    // Add the data to the input buffer
    input_buffer.enqueue(data);

    // Process the state machine
    process_state_machine();
}

QString TvaluesToChannel::pop_frame() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool TvaluesToChannel::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void TvaluesToChannel::process_state_machine() {
    // Add the input data to the internal t-value buffer
    internal_buffer.append(input_buffer.dequeue());

    // We need 588 bits to make a frame.  Every frame starts with T11+T11.
    // So the minimum number of t-values we need is 54 and 
    // the maximum number of t-values we can have is 191.  This upper limit
    // is where we need to maintain the buffer size (at 382 for 2 frames).

    while(internal_buffer.size() > 382) {
        switch(current_state) {
            case EXPECTING_INITIAL_SYNC:
                current_state = expecting_initial_sync();
                break;
            case EXPECTING_SYNC:
                current_state = expecting_sync();
                break;

            case HANDLE_OVERSHOOT:
                current_state = handle_overshoot();
                break;

            case HANDLE_UNDERSHOOT:
                current_state = handle_undershoot();
                break;
        }
    }
}

TvaluesToChannel::State TvaluesToChannel::expecting_initial_sync() {
    State next_state = EXPECTING_INITIAL_SYNC;

    // Expected sync header
    QByteArray t11_t11 = QByteArray::fromHex("0B0B");

    // Does the buffer contain a T11+T11 sequence?
    int initial_sync_index = internal_buffer.indexOf(t11_t11);

    if (initial_sync_index != -1) {
        qDebug() << "TvaluesToChannel::expecting_initial_sync() - Initial sync header found at index:" << initial_sync_index;
        next_state = EXPECTING_SYNC;
    } else {
        // Drop all but the last T-value in the buffer
        qDebug() << "TvaluesToChannel::expecting_initial_sync() - Initial sync header not found, dropping" << internal_buffer.size() - 1 << "T-values";
        discarded_t_values += internal_buffer.size() - 1;
        internal_buffer = internal_buffer.right(1);
    }

    return next_state;
}

TvaluesToChannel::State TvaluesToChannel::expecting_sync() {
    State next_state = EXPECTING_SYNC;

    // The internal buffer contains a valid sync at the start
    // Find the next sync header after it
    QByteArray t11_t11 = QByteArray::fromHex("0B0B");
    int sync_index = internal_buffer.indexOf(t11_t11, 2);

    // Do we have a valid second sync header?
    if (sync_index != -1) {
        // Extract the frame data from (and including) the first sync header until
        // (but not including) the second sync header
        QByteArray frame_data = internal_buffer.left(sync_index);

        // Do we have exactly 588 bits of data?  Count the T-values
        int bit_count = 0;
        for (int i = 0; i < frame_data.size(); i++) {
            bit_count += frame_data.at(i);
        }

        // If the frame data is 550 to 600 bits, we have a valid frame
        if (bit_count > 550 && bit_count < 600) {
            // We have a valid frame
            if (bit_count != 588) qDebug() << "TvaluesToChannel::expecting_sync() - Got frame with" << bit_count << "bits - Treating as valid";

            // Place the frame data into the output buffer
            output_buffer.enqueue(frame_data);
            consumed_t_values += frame_data.size();
            channel_frame_count++;
            perfect_syncs++;

            if (bit_count == 588) perfect_frames++;
            if (bit_count < 588) long_frames++;
            if (bit_count > 588) short_frames++;
            
            // Remove the frame data from the internal buffer
            internal_buffer = internal_buffer.right(internal_buffer.size() - sync_index);
            next_state = EXPECTING_SYNC;
        } else {
            // This is most likely a missing sync header issue rather than
            // one or more T-values being incorrect. So we'll handle that 
            // separately.
            if (bit_count > 588) {
                //qDebug() << "TvaluesToF3Frame::expecting_sync() - Got frame with" << bit_count << "bits - Overshoot";
                next_state = HANDLE_OVERSHOOT;
            } else {
                //qDebug() << "TvaluesToF3Frame::expecting_sync() - Got frame with" << bit_count << "bits - Undershoot";
                next_state = HANDLE_UNDERSHOOT;
            }
        }
    }

    return next_state;
}

TvaluesToChannel::State TvaluesToChannel::handle_undershoot() {
    State next_state = EXPECTING_SYNC;

    // The frame data is too short
    undershoot_syncs++;

    // Find the second sync header
    QByteArray t11_t11 = QByteArray::fromHex("0B0B");
    int second_sync_index = internal_buffer.indexOf(t11_t11, 2);

    // Find the third sync header
    int third_sync_index = internal_buffer.indexOf(t11_t11, second_sync_index + 2);

    // So, unless the data is completely corrupt we should have 588 bits between
    // the first and third sync headers (i.e. the second was a corrupt sync header) or
    // 588 bits between the second and third sync headers (i.e. the first was a corrupt sync header)
    //
    // If neither of these conditions are met, we have a corrupt frame data and we have to drop it
   
    if (third_sync_index != -1) {
        // Value of the Ts between the first and third sync header
        int ftt_bit_count = 0;
        for (int i = 0; i < third_sync_index; i++) {
            ftt_bit_count += internal_buffer.at(i);
        }
        
        // Value of the Ts between the second and third sync header
        int stt_bit_count = 0;
        for (int i = second_sync_index; i < third_sync_index; i++) {
            stt_bit_count += internal_buffer.at(i);
        }
        
        if (ftt_bit_count > 550 && ftt_bit_count < 600) {
            // Valid frame between the first and third sync headers
            qDebug() << "TvaluesToChannel::handle_undershoot() - Undershoot frame - Value from first to third sync_header =" << ftt_bit_count << "bits - treating as valid";
            QByteArray frame_data = internal_buffer.left(third_sync_index);
            output_buffer.enqueue(frame_data);
            consumed_t_values += frame_data.size();
            channel_frame_count++;

            if (ftt_bit_count == 588) perfect_frames++;
            if (ftt_bit_count < 588) long_frames++;
            if (ftt_bit_count > 588) short_frames++;

            // Remove the frame data from the internal buffer
            internal_buffer = internal_buffer.right(internal_buffer.size() - third_sync_index);
            next_state = EXPECTING_SYNC;
        } else if (stt_bit_count > 550 && stt_bit_count < 600) {
            // Valid frame between the second and third sync headers
            qDebug() << "TvaluesToChannel::handle_undershoot() - Undershoot frame - Value from second to third sync_header =" << stt_bit_count << "bits - treating as valid";
            QByteArray frame_data = internal_buffer.mid(second_sync_index, third_sync_index - second_sync_index);
            output_buffer.enqueue(frame_data);
            consumed_t_values += frame_data.size();
            channel_frame_count++;

            if (stt_bit_count == 588) perfect_frames++;
            if (stt_bit_count < 588) long_frames++;
            if (stt_bit_count > 588) short_frames++;

            // Remove the frame data from the internal buffer
            discarded_t_values += second_sync_index;
            internal_buffer = internal_buffer.right(internal_buffer.size() - third_sync_index);
            next_state = EXPECTING_SYNC;
        } else {
            // Corrupt frame data - can't recover
            qDebug() << "TvaluesToChannel::handle_undershoot() - Undershoot -2";
            qFatal("TvaluesToChannel::handle_undershoot() - Undershoot frame detected but no valid frame found between first and third sync headers - what to do?");
        }
    } else {
        qDebug() << "TvaluesToChannel::handle_undershoot() - No third sync header found.  Staying in undershoot state waiting for more data";
        next_state = HANDLE_UNDERSHOOT;
    }

    return next_state;
}

TvaluesToChannel::State TvaluesToChannel::handle_overshoot() {
    State next_state = EXPECTING_SYNC;

    // The frame data is too long
    overshoot_syncs++;
    
    // Is the overshoot due to a missing/corrupt sync header?
    // Count the bits between the first and second sync headers, if they are 588*2, split
    // the frame data into two frames
    QByteArray t11_t11 = QByteArray::fromHex("0B0B");

    // Find the second sync header
    int sync_index = internal_buffer.indexOf(t11_t11, 2);

    // Do we have a valid second sync header?
    if (sync_index != -1) {
        // Extract the frame data from (and including) the first sync header until
        // (but not including) the second sync header
        QByteArray frame_data = internal_buffer.left(sync_index);

        // How many bits of data do we have?  Count the T-values
        int bit_count = 0;
        for (int i = 0; i < frame_data.size(); i++) {
            bit_count += frame_data.at(i);
        }

        // If the frame data is 588*2 bits, or within 11 bits of 588*2, we have a two frames
        // separated by a corrupt sync header
        if (bit_count > 588*2-11 && bit_count < 588*2+11) {
            qDebug() << "TvaluesToChannel::handle_overshoot() - Overshoot frame -" << bit_count << "bits - splitting into two frames of" << bit_count/2 << "bits";

            // Place the frame data into the output buffer
            output_buffer.enqueue(frame_data.left(frame_data.size()/2));
            output_buffer.enqueue(frame_data.right(frame_data.size()/2));
            consumed_t_values += frame_data.size();
            channel_frame_count += 2;

            if (bit_count/2 == 588) perfect_frames+=2;
            if (bit_count/2 < 588) long_frames+=2;
            if (bit_count/2 > 588) short_frames+=2;

            // Remove the frame data from the internal buffer
            internal_buffer = internal_buffer.right(internal_buffer.size() - sync_index);

            next_state = EXPECTING_SYNC;
        } else {
            qDebug() << "TvaluesToChannel::handle_overshoot() - Overshoot frame -" << bit_count << "bits, but no sync header found, dropping" << internal_buffer.size() - 1 << "T-values";
            internal_buffer = internal_buffer.right(1);
            next_state = EXPECTING_INITIAL_SYNC;
        }
    } else {
        qFatal("TvaluesToChannel::handle_overshoot() - Overshoot frame detected but no second sync header found, even though it should have been there.");
    }

    return next_state;
}

void TvaluesToChannel::show_statistics() {
    qInfo() << "T-values to Channel frame statistics:";
    qInfo() << "  T-Values:";
    qInfo() << "    Consumed:" << consumed_t_values;
    qInfo() << "    Discarded:" << discarded_t_values;
    qInfo() << "  Channel frames:";
    qInfo() << "    Total:" << channel_frame_count;
    qInfo() << "    588 bits:" << perfect_frames;
    qInfo() << "    >588 bits:" << long_frames;
    qInfo() << "    <588 bits:" << short_frames;
    qInfo() << "  Sync headers:";
    qInfo() << "    Good syncs:" << perfect_syncs;
    qInfo() << "    Overshoots:" << overshoot_syncs;
    qInfo() << "    Undershoots:" << undershoot_syncs;

    // When we overshoot and split the frame, we are guessing the sync header...
    qInfo() << "    Guessed:" << channel_frame_count - perfect_syncs - overshoot_syncs - undershoot_syncs;
}