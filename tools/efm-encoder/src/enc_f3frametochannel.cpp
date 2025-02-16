/************************************************************************

    enc_f3frametochannel.cpp

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

#include "enc_f3frametochannel.h"

// F3FrameToChannel class implementation
F3FrameToChannel::F3FrameToChannel()
{
    // Output data is a string that represents bits
    // Since we are working with 14-bit EFM symbols, 3 bit merging symbols and
    // a 24 bit frame sync header - the output length of a channel frame is 588 bit (73.5 bytes)
    // so we can't use a QByteArray directly here without introducing unwanted padding
    dsv = 0;
    dsv_direction = true;
    total_t_values = 0;
    total_sections = 0;

    previous_channel_frame.clear();
    valid_channel_frames_count = 0;

    // Set corruption to false by default
    corrupt_f3sync = false;
    corrupt_f3sync_frequency = 0;
    corrupt_subcode_sync = false;
    corrupt_subcode_sync_frequency = 0;

    subcode_corruption_type = 0;
}

void F3FrameToChannel::push_frame(F3Frame f3_frame)
{
    input_buffer.enqueue(f3_frame);

    process_queue();
}

QVector<uint8_t> F3FrameToChannel::pop_frame()
{
    if (!is_ready()) {
        qFatal("F3FrameToChannel::pop_frame(): No bytes are available.");
    }
    return output_buffer.dequeue();
}

void F3FrameToChannel::process_queue()
{
    // Seed a random number generator
    QRandomGenerator random_generator(
            static_cast<uint32_t>(QTime::currentTime().msecsSinceStartOfDay()));

    while (!input_buffer.isEmpty()) {
        // Pop the F3 frame data from the processing queue
        F3Frame f3_frame = input_buffer.dequeue();
        QVector<uint8_t> f3_frame_data = f3_frame.data();

        // Ensure the F3 frame data is 32 bytes long
        if (f3_frame_data.size() != 32) {
            qFatal("F3FrameToChannel::process_queue(): F3 frame data must be 32 bytes long.");
        }

        QString channel_frame;
        QString merging_bits = "xxx";

        // F3 Sync header
        if (corrupt_f3sync) {
            if (valid_channel_frames_count % corrupt_f3sync_frequency == 0) {
                // Generate a random sync header
                // Must be 24 bits long and have a minimum of 2 and a maximum of 10 zeros between
                // each 1
                channel_frame += generate_random_sync_value();
                qDebug() << "F3FrameToChannel::process_queue(): Corrupting F3 sync header:"
                         << channel_frame;
            } else {
                channel_frame += sync_header;
            }
        } else {
            channel_frame += sync_header;
        }
        channel_frame += merging_bits;

        QString subcode_value;

        // Subcode sync0 and sync1 headers (based on the F3 frame type)
        if (f3_frame.f3FrameType() == F3Frame::F3FrameType::Subcode) {
            subcode_value = efm.eightToFourteen(f3_frame.subcodeByte());
        } else if (f3_frame.f3FrameType() == F3Frame::F3FrameType::Sync0) {
            subcode_value += efm.eightToFourteen(256);
            total_sections++;

            // Note: This is 0-8 because we want it to be unlikely that both sync0 and sync1 are
            // corrupted at the same time.  So 0 = corrupt both, 1-4 = corrupt sync0 and 5-8 =
            // corrupt sync1
            subcode_corruption_type = random_generator.bounded(9); // 0-8
        } else {
            // SYNC1
            subcode_value += efm.eightToFourteen(257);
        }

        // Corrupt the subcode sync0 and sync1 patterns?
        if (corrupt_subcode_sync) {
            if (total_sections % corrupt_subcode_sync_frequency == 0) {
                if (f3_frame.f3FrameType() == F3Frame::F3FrameType::Sync0) {
                    if (subcode_corruption_type == 0
                        || (subcode_corruption_type >= 1 && subcode_corruption_type <= 4)) {
                        // Corrupt the sync0 pattern
                        subcode_value = efm.eightToFourteen(random_generator.bounded(256));
                        qDebug() << "F3FrameToChannel::process_queue(): Corrupting subcode sync0 "
                                    "value:"
                                 << subcode_value;
                    }
                }

                if (f3_frame.f3FrameType() == F3Frame::F3FrameType::Sync1) {
                    if (subcode_corruption_type == 0
                        || (subcode_corruption_type >= 5 && subcode_corruption_type <= 8)) {
                        // Corrupt the sync1 pattern
                        subcode_value = efm.eightToFourteen(random_generator.bounded(256));
                        qDebug() << "F3FrameToChannel::process_queue(): Corrupting subcode sync1 "
                                    "value:"
                                 << subcode_value;
                    }
                }
            }
        }

        channel_frame += subcode_value;
        channel_frame += merging_bits;

        // Now we output the actual F3 frame data
        for (uint32_t index = 0; index < f3_frame_data.size(); index++) {
            channel_frame += efm.eightToFourteen(f3_frame_data[index]);
            channel_frame += merging_bits;
        }

        // Now we add the merging bits to the frame
        channel_frame = add_merging_bits(channel_frame);

        // Sanity check - Frame size should be 588 bits
        if (channel_frame.size() != 588) {
            qDebug() << "F3FrameToChannel::process_queue(): Channel frame:" << channel_frame;
            qFatal("F3FrameToChannel::process_queue(): BUG - Merged channel frame size is not 588 "
                   "bits. It is %d bits",
                   channel_frame.size());
        }

        // Sanity check - The frame should only contain one sync header (unless we are corrupting
        // it)
        if (channel_frame.count(sync_header) != 1 && !corrupt_f3sync) {
            qDebug() << "F3FrameToChannel::process_queue(): Channel frame:" << channel_frame;
            qFatal("F3FrameToChannel::process_queue(): BUG - Channel frame contains %d sync "
                   "headers.",
                   channel_frame.count(sync_header));
        }

        // Sanity check - This and the previous frame combined should contain only two sync headers
        // (edge case detection)
        if (!previous_channel_frame.isEmpty()) {
            QString combined_frames = previous_channel_frame + channel_frame;
            if (combined_frames.count(sync_header) != 2) {
                qDebug() << "F3FrameToChannel::process_queue(): Previous frame:"
                         << previous_channel_frame;
                qDebug() << "F3FrameToChannel::process_queue():  Current frame:" << channel_frame;
                qFatal("F3FrameToChannel::process_queue(): BUG - Previous and current channel "
                       "frames combined contains more than two sync headers.");
            }
        }

        // Flush the output data to the output buffer
        valid_channel_frames_count++;
        write_frame(channel_frame);
    }
}

void F3FrameToChannel::write_frame(QString channel_frame)
{
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
                if ((j + i) < channel_frame.size() && channel_frame[j + i] == '0') {
                    zero_count++;
                } else {
                    break;
                }
            }

            // The number of zeros is not between 2 and 10 - something is wrong in the input data
            if (zero_count < 2 || zero_count > 10) {
                qInfo() << "F3FrameToChannel::write_frame(): Channel frame:" << channel_frame;
                qInfo() << "F3FrameToChannel::write_frame(): Zero count:" << zero_count;
                qFatal("F3FrameToChannel::write_frame(): Number of zeros between ones is not "
                       "between 2 and 10.");
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

bool F3FrameToChannel::is_ready() const
{
    return !output_buffer.isEmpty();
}

QString F3FrameToChannel::add_merging_bits(QString channel_frame)
{
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

        // Set start to the next set of merging bits, the first merging bits are 24 bits in and then
        // every 14 bits
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
            // If it's the last set of merging bits, the next EFM symbol is the sync header of the
            // next frame
            next_efm = sync_header;
        } else {
            next_efm = merged_frame.mid(start + 3, 14);
        }

        if (next_efm.isEmpty())
            qFatal("F3FrameToChannel::add_merging_bits(): Next EFM symbol is empty.");

        // Get a list of legal merging bit patterns
        QStringList merging_patterns = get_legal_merging_bit_patterns(current_efm, next_efm);

        // Order the patterns by the resulting DSV delta
        merging_patterns = order_patterns_by_dsv_delta(merging_patterns, current_efm, next_efm);

        // Pick the first pattern from the list that doesn't form a spurious sync_header pattern
        QString merging_bits;
        for (const QString &pattern : merging_patterns) {
            QString temp_frame = merged_frame;

            // Count the current number of sync headers (might be 0 due to corruption settings)
            int sync_header_count = temp_frame.count(sync_header);

            // Add our possible pattern to the frame
            temp_frame.replace(start, 3, pattern);

            // Any spurious sync headers generated?
            if (temp_frame.count(sync_header) == sync_header_count) {
                merging_bits = pattern;
                break;
            }
        }

        // This shouldn't happen, but it it does, let's get debug information :)
        if (merging_bits.isEmpty()) {
            qDebug() << "F3FrameToChannel::add_merging_bits(): No legal merging bit pattern found.";
            qDebug() << "F3FrameToChannel::add_merging_bits(): Possible patterns:"
                     << merging_patterns;
            qDebug() << "F3FrameToChannel::add_merging_bits(): Merging bits start at:" << start;
            qDebug() << "F3FrameToChannel::add_merging_bits():" << merged_frame.mid(start - 24, 24)
                     << "xxx" << merged_frame.mid(start + 3, 24);

            qFatal("F3FrameToChannel::add_merging_bits(): No legal merging bit pattern found - "
                   "encode failed");
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
QStringList F3FrameToChannel::get_legal_merging_bit_patterns(const QString &current_efm,
                                                             const QString &next_efm)
{
    // Check that current_efm is at least 14 bits long
    if (current_efm.size() < 14) {
        qFatal("F3FrameToChannel::get_legal_merging_bit_patterns(): Current EFM symbol is too "
               "short.");
    }

    // Check that next_efm is at least 14 bits long
    if (next_efm.size() < 14) {
        qFatal("F3FrameToChannel::get_legal_merging_bit_patterns(): Next EFM symbol is too short.");
    }

    QStringList patterns;
    QStringList possible_patterns = { "000", "001", "010", "100" };
    for (const QString &pattern : possible_patterns) {
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
QStringList F3FrameToChannel::order_patterns_by_dsv_delta(const QStringList &merging_patterns,
                                                          const QString &current_efm,
                                                          const QString &next_efm)
{
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
int32_t F3FrameToChannel::calculate_dsv_delta(const QString data)
{
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

int32_t F3FrameToChannel::get_total_t_values() const
{
    return total_t_values;
}

void F3FrameToChannel::set_corruption(bool _corrupt_f3sync, uint32_t _corrupt_f3sync_frequency,
                                      bool _corrupt_subcode_sync,
                                      uint32_t _corrupt_subcode_sync_frequency)
{
    corrupt_f3sync = _corrupt_f3sync;
    corrupt_f3sync_frequency = _corrupt_f3sync_frequency;
    corrupt_subcode_sync = _corrupt_subcode_sync;
    corrupt_subcode_sync_frequency = _corrupt_subcode_sync_frequency;
}

// Function to generate a "random" 24-bit sync
QString F3FrameToChannel::generate_random_sync_value()
{

    // Seed the random number generator
    QRandomGenerator *generator = QRandomGenerator::global();

    QStringList possible_replacements{
        "100100000001000000000010", "100000100010000010000010", "100000000001001000000010",
        "100000010000100001000010", "100100000001001000000010", "100000100000010001000010",
        "100100000001000010000010", "100000100010001000100010", "100001000001001000000010",
        "100100010000010001000010", "100100000010001001000010", "100000100001001000100010",
    };

    // Pick a random value between 0 and the length of the possible replacements
    uint32_t random_index = generator->bounded(possible_replacements.size());

    return possible_replacements[random_index];
}