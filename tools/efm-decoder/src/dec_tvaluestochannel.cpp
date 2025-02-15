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

QByteArray TvaluesToChannel::pop_frame() {
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
        if (show_debug) qDebug() << "TvaluesToChannel::expecting_initial_sync() - Initial sync header found at index:" << initial_sync_index;
        next_state = EXPECTING_SYNC;
    } else {
        if (show_debug) qDebug() << "TvaluesToChannel::expecting_initial_sync() - Initial sync header not found, dropping" << internal_buffer.size() - 1 << "T-values";
        // Drop all but the last T-value in the buffer
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
        int bit_count = count_bits(frame_data);

        // If the frame data is 550 to 600 bits, we have a valid frame
        if (bit_count > 550 && bit_count < 600) {
            if (bit_count != 588 && show_debug) qDebug() << "TvaluesToChannel::expecting_sync() - Got frame with" << bit_count << "bits - Treating as valid";

            // We have a valid frame
            // Place the frame data into the output buffer
            output_buffer.enqueue(frame_data);
            if (show_debug && count_bits(frame_data) != 588) qDebug() << "TvaluesToChannel::expecting_sync() - Queuing frame of" << count_bits(frame_data) << "bits";
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
        int ftt_bit_count = count_bits(internal_buffer, 0, third_sync_index);
        
        // Value of the Ts between the second and third sync header
        int stt_bit_count = count_bits(internal_buffer, second_sync_index, third_sync_index);
        
        if (ftt_bit_count > 550 && ftt_bit_count < 600) {
            if (show_debug) qDebug() << "TvaluesToChannel::handle_undershoot() - Undershoot frame - Value from first to third sync_header =" << ftt_bit_count << "bits - treating as valid";
            // Valid frame between the first and third sync headers
            QByteArray frame_data = internal_buffer.left(third_sync_index);
            output_buffer.enqueue(frame_data);
            if (show_debug && count_bits(frame_data) != 588) qDebug() << "TvaluesToChannel::handle_undershoot1() - Queuing frame of" << count_bits(frame_data) << "bits";
            consumed_t_values += frame_data.size();
            channel_frame_count++;

            if (ftt_bit_count == 588) perfect_frames++;
            if (ftt_bit_count < 588) long_frames++;
            if (ftt_bit_count > 588) short_frames++;

            // Remove the frame data from the internal buffer
            internal_buffer = internal_buffer.right(internal_buffer.size() - third_sync_index);
            next_state = EXPECTING_SYNC;
        } else if (stt_bit_count > 550 && stt_bit_count < 600) {
            if (show_debug) qDebug() << "TvaluesToChannel::handle_undershoot() - Undershoot frame - Value from second to third sync_header =" << stt_bit_count << "bits - treating as valid";
            // Valid frame between the second and third sync headers
            QByteArray frame_data = internal_buffer.mid(second_sync_index, third_sync_index - second_sync_index);
            output_buffer.enqueue(frame_data);
            if (show_debug && count_bits(frame_data) != 588) qDebug() << "TvaluesToChannel::handle_undershoot2() - Queuing frame of" << count_bits(frame_data) << "bits";
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
            if (show_debug) qDebug() << "TvaluesToChannel::handle_undershoot() - First to third sync is" << ftt_bit_count << "bits, second to third sync is" << stt_bit_count <<
                ". Dropping (what might be a) frame.";
            next_state = EXPECTING_SYNC;

            // Remove the frame data from the internal buffer
            discarded_t_values += second_sync_index;
            internal_buffer = internal_buffer.right(internal_buffer.size() - third_sync_index);
        }
    } else {
        if (show_debug) qDebug() << "TvaluesToChannel::handle_undershoot() - No third sync header found.  Staying in undershoot state waiting for more data";
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

        // Remove the frame data from the internal buffer
        internal_buffer = internal_buffer.right(internal_buffer.size() - sync_index);

        // How many bits of data do we have?  Count the T-values
        int bit_count = count_bits(frame_data);

        // If the frame data is 588*2 bits, or within 11 bits of 588*2, we have a two frames
        // separated by a corrupt sync header
        if (bit_count > 588*2-11 && bit_count < 588*2+11) {
            // Count the number of T-values to 588 bits
            int first_frame_bits = 0;
            int end_of_frame_index = 0;
            for (int i = 0; i < frame_data.size(); i++) {
                first_frame_bits += frame_data.at(i);
                if (first_frame_bits >= 588) {
                    end_of_frame_index = i;
                    break;
                }
            }

            QByteArray first_frame_data = frame_data.left(end_of_frame_index + 1);
            QByteArray second_frame_data = frame_data.right(frame_data.size() - end_of_frame_index - 1);

            uint32_t first_frame_bit_count = count_bits(first_frame_data);
            uint32_t second_frame_bit_count = count_bits(second_frame_data);

            // Place the frames into the output buffer
            output_buffer.enqueue(first_frame_data);
            output_buffer.enqueue(second_frame_data);

            if (show_debug) qDebug() << "TvaluesToChannel::handle_overshoot() - Overshoot frame split -" << first_frame_bit_count << "/" << second_frame_bit_count << "bits";

            consumed_t_values += frame_data.size();
            channel_frame_count += 2;

            if (first_frame_bit_count == 588) perfect_frames++;
            if (first_frame_bit_count < 588) long_frames++;
            if (first_frame_bit_count > 588) short_frames++;

            if (second_frame_bit_count == 588) perfect_frames++;
            if (second_frame_bit_count < 588) long_frames++;
            if (second_frame_bit_count > 588) short_frames++;

            next_state = EXPECTING_SYNC;
        } else {
            if (show_debug) qDebug() << "TvaluesToChannel::handle_overshoot() - Overshoot frame -" << bit_count << "bits, but no sync header found, dropping" << internal_buffer.size() - 1 << "T-values";
            internal_buffer = internal_buffer.right(1);
            next_state = EXPECTING_INITIAL_SYNC;
        }
    } else {
        qFatal("TvaluesToChannel::handle_overshoot() - Overshoot frame detected but no second sync header found, even though it should have been there.");
    }

    return next_state;
}

// Count the number of bits in the array of T-values
uint32_t TvaluesToChannel::count_bits(QByteArray data, int32_t start_position, int32_t end_position) {
    if (end_position == -1) end_position = data.size();

    uint32_t bit_count = 0;
    for (int i = start_position; i < end_position; i++) {
        bit_count += data.at(i);
    }
    return bit_count;
}

void TvaluesToChannel::show_statistics() {
    qInfo() << "T-values to Channel Frame statistics:";
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