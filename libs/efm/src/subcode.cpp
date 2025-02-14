/************************************************************************

    subcode.cpp

    EFM-library - Convert subcode data to FrameMetadata and back
    Copyright (C) 2025 Simon Inns

    This file is part of EFM-Tools.

    This is free software: you can redistribute it and/or
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

#include "subcode.h"

// Takes 98 bytes of subcode data and returns a FrameMetadata object
SectionMetadata Subcode::from_data(const QByteArray& data) {
    // Ensure the data is 98 bytes long
    if (data.size() != 98) {
        qFatal("Subcode::from_data(): Data size of %d does not match 98 bytes", data.size());
    }

    // Extract the p-channel data and q-channel data
    QByteArray p_channel;
    QByteArray q_channel;
    p_channel.resize(12);
    q_channel.resize(12);

    // Note: index 0 and 1 are sync0 and sync1 bytes
    // so we get 96 bits of data per channel from 98 bytes input
    for (uint32_t index = 2; index < data.size(); index++) {
        set_bit(p_channel, index-2, data[index] & 0x80);
        set_bit(q_channel, index-2, data[index] & 0x40);
    }

    // Create the SectionMetadata object
    SectionMetadata section_metadata;

    // Set the p-channel (p-channel is just repeating flag)
    if (static_cast<char>(p_channel[0]) == 0) section_metadata.set_p_flag(false);
    else section_metadata.set_p_flag(true);

    // Set the q-channel
    // If the q-channel CRC is not valid, attempt to repair the data
    bool repaired = false;
    if (!is_crc_valid(q_channel)) repaired = repair_data(q_channel);

    if (is_crc_valid(q_channel)) {
        // Set the q-channel data from the subcode data

        // Get the address and control nybbles
        uint8_t control_nybble = q_channel[0] >> 4;
        uint8_t mode_nybble = q_channel[0] & 0x0F;

        // Set the q-channel mode
        switch (mode_nybble) {
            case 0x1:
                section_metadata.set_q_mode(SectionMetadata::QMODE_1);
                break;
            case 0x2:
                section_metadata.set_q_mode(SectionMetadata::QMODE_2);
                break;
            case 0x3:
                section_metadata.set_q_mode(SectionMetadata::QMODE_3);
                break;
            case 0x4:
                section_metadata.set_q_mode(SectionMetadata::QMODE_4);
                break;
            default:
                if (show_debug) qDebug() << "Subcode::from_data(): Q channel data is:" << q_channel.toHex();
                qFatal("Subcode::from_data(): Invalid Q-mode nybble! Must be 1, 2, 3 or 4 not %d", mode_nybble);
        }

        // Set the q-channel control settings
        switch (control_nybble) {
            case 0x0:
                // AUDIO_2CH_NO_PREEMPHASIS_COPY_PROHIBITED
                section_metadata.set_audio(true);
                section_metadata.set_copy_prohibited(true);
                section_metadata.set_preemphasis(false);
                section_metadata.set_2_channel(true);
                break;
            case 0x1:
                // AUDIO_2CH_PREEMPHASIS_COPY_PROHIBITED
                section_metadata.set_audio(true);
                section_metadata.set_copy_prohibited(true);
                section_metadata.set_preemphasis(true);
                section_metadata.set_2_channel(true);
                break;
            case 0x2:
                // AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED
                section_metadata.set_audio(true);
                section_metadata.set_copy_prohibited(false);
                section_metadata.set_preemphasis(false);
                section_metadata.set_2_channel(true);
                break;
            case 0x3:
                // AUDIO_2CH_PREEMPHASIS_COPY_PERMITTED
                section_metadata.set_audio(true);
                section_metadata.set_copy_prohibited(false);
                section_metadata.set_preemphasis(true);
                section_metadata.set_2_channel(true);
                break;
            case 0x4:
                // DIGITAL_COPY_PROHIBITED
                section_metadata.set_audio(false);
                section_metadata.set_copy_prohibited(true);
                section_metadata.set_preemphasis(false);
                section_metadata.set_2_channel(true);
                break;
            case 0x6:
                // DIGITAL_COPY_PERMITTED
                section_metadata.set_audio(false);
                section_metadata.set_copy_prohibited(false);
                section_metadata.set_preemphasis(false);
                section_metadata.set_2_channel(true);
                break;
            case 0x8:
                // AUDIO_4CH_NO_PREEMPHASIS_COPY_PROHIBITED
                section_metadata.set_audio(true);
                section_metadata.set_copy_prohibited(true);
                section_metadata.set_preemphasis(false);
                section_metadata.set_2_channel(false);
                break;
            case 0x9:
                // AUDIO_4CH_PREEMPHASIS_COPY_PROHIBITED
                section_metadata.set_audio(true);
                section_metadata.set_copy_prohibited(true);
                section_metadata.set_preemphasis(true);
                section_metadata.set_2_channel(false);
                break;
            case 0xA:
                // AUDIO_4CH_NO_PREEMPHASIS_COPY_PERMITTED
                section_metadata.set_audio(true);
                section_metadata.set_copy_prohibited(false);
                section_metadata.set_preemphasis(false);
                section_metadata.set_2_channel(false);
                break;
            case 0xB:
                // AUDIO_4CH_PREEMPHASIS_COPY_PERMITTED
                section_metadata.set_audio(true);
                section_metadata.set_copy_prohibited(false);
                section_metadata.set_preemphasis(true);
                section_metadata.set_2_channel(false);
                break;
            default:
                if (show_debug) qDebug() << "Subcode::from_data(): Q channel data is:" << q_channel.toHex();
                qFatal("Subcode::from_data(): Invalid control nybble! Must be 0-3, 4-7 or 8-11 not %d", control_nybble);
        }

         // Get the track number
        uint8_t track_number = bcd2_to_int(q_channel[1]);

        // If the track number is 0, then this is a lead-in frame
        // If the track number is 0xAA, then this is a lead-out frame
        // If the track number is 1-99, then this is a user data frame
        if (track_number == 0) {
            section_metadata.set_section_type(SectionType::LEAD_IN);
        } else if (track_number == 0xAA) {
            section_metadata.set_section_type(SectionType::LEAD_OUT);
        } else {
            section_metadata.set_section_type(SectionType::USER_DATA);
        }

        // Now set the track number
        section_metadata.set_track_number(track_number);

        // Set the frame time q_data_channel[3-5]
        section_metadata.set_section_time(SectionTime(bcd2_to_int(q_channel[3]), bcd2_to_int(q_channel[4]), bcd2_to_int(q_channel[5])));

        // Set the zero byte q_data_channel[6] - Not used at the moment

        // Set the ap time q_data_channel[7-9]
        section_metadata.set_absolute_section_time(SectionTime(bcd2_to_int(q_channel[7]), bcd2_to_int(q_channel[8]), bcd2_to_int(q_channel[9])));

        section_metadata.set_valid(true);
    } else {
        // Set the q-channel data to invalid leaving the rest of 
        // the metadata as default values
        if (show_debug) qDebug() << "Subcode::from_data(): Invalid CRC in Q-channel data - expected:" << QString::number(get_q_channel_crc(q_channel), 16) <<
            "calculated:" << QString::number(calculate_q_channel_crc16(q_channel), 16);

        SectionTime bad_abs_time = SectionTime(bcd2_to_int(q_channel[7]), bcd2_to_int(q_channel[8]), bcd2_to_int(q_channel[9]));
        if (show_debug) qDebug().noquote() << "Subcode::from_data(): Q channel data is:" << q_channel.toHex() << "potentially corrupt absolute time is:" << bad_abs_time.to_string();
        section_metadata.set_valid(false);
    }

    // Sanity check the track number and frame type
    //
    // If the track number is 0, then this is a lead-in frame
    // If the track number is 0xAA, then this is a lead-out frame
    // If the track number is 1-99, then this is a user data frame
    if (section_metadata.get_track_number() == 0 && section_metadata.get_section_type() != SectionType::LEAD_IN) {
        if (show_debug) qDebug("Subcode::from_data(): Track number 0 is only valid for lead-in frames");
    } else if (section_metadata.get_track_number() == 0xAA && section_metadata.get_section_type() != SectionType::LEAD_OUT) {
        if (show_debug) qDebug("Subcode::from_data(): Track number 0xAA is only valid for lead-out frames");
    } else if (section_metadata.get_track_number() > 99) {
        if (show_debug) qDebug("Subcode::from_data(): Track number %d is out of range", section_metadata.get_track_number());
    }

    if (repaired) {
        if (show_debug) qDebug().noquote() << "Subcode::from_data(): Q-channel repaired for section with absolute time:" << section_metadata.get_absolute_section_time().to_string() <<
            "track number:" << section_metadata.get_track_number() << "and section time:" << section_metadata.get_section_time().to_string();
    }

    // All done!
    return section_metadata;
}

// Takes a FrameMetadata object and returns 98 bytes of subcode data
QByteArray Subcode::to_data(const SectionMetadata& section_metadata) {
    QByteArray p_channel_data(12,0);
    QByteArray q_channel_data(12,0);

    // Set the p-channel data
    for (int i = 0; i < 12; i++) {
        if (section_metadata.is_p_flag()) p_channel_data[i] = 0xFF;
        else p_channel_data[i] = 0x00;
    }

    // Create the control and address nybbles
    uint8_t control_nybble = 0;
    uint8_t mode_nybble = 0;

    switch (section_metadata.get_q_mode()) {
        case SectionMetadata::QMODE_1:
            mode_nybble = 0x1; // 0b0001
            break;
        case SectionMetadata::QMODE_2:
            mode_nybble = 0x2; // 0b0010
            break;
        case SectionMetadata::QMODE_3:
            mode_nybble = 0x3; // 0b0011
            break;
        case SectionMetadata::QMODE_4:
            mode_nybble = 0x4; // 0b0100
            break;
        default:
            qFatal("Subcode::to_data(): Invalid Q-mode %d", section_metadata.get_q_mode());
    }

    bool audio = section_metadata.is_audio();
    bool copy_prohibited = section_metadata.is_copy_prohibited();
    bool preemphasis = section_metadata.is_preemphasis();
    bool channels2 = section_metadata.is_2_channel();

    // These are the valid combinations of control nybble flags
    if (audio && channels2 && !preemphasis && copy_prohibited) control_nybble = 0x0; // 0b0000 = AUDIO_2CH_NO_PREEMPHASIS_COPY_PROHIBITED
    else if (audio && channels2 && preemphasis && copy_prohibited) control_nybble = 0x1; // 0b0001 = AUDIO_2CH_PREEMPHASIS_COPY_PROHIBITED
    else if (audio && channels2 && !preemphasis && !copy_prohibited) control_nybble = 0x2; // 0b0010 = AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED
    else if (audio && channels2 && preemphasis && !copy_prohibited) control_nybble = 0x3; // 0b0011 = AUDIO_2CH_PREEMPHASIS_COPY_PERMITTED
    else if (!audio && copy_prohibited) control_nybble = 0x4; // 0b0100 = DIGITAL_COPY_PROHIBITED
    else if (!audio && !copy_prohibited) control_nybble = 0x6; // 0b0110 = DIGITAL_COPY_PERMITTED
    else if (audio && !channels2 && !preemphasis && copy_prohibited) control_nybble = 0x8; // 0b1000 = AUDIO_4CH_NO_PREEMPHASIS_COPY_PROHIBITED
    else if (audio && !channels2 && preemphasis && copy_prohibited) control_nybble = 0x9; // 0b1001 = AUDIO_4CH_PREEMPHASIS_COPY_PROHIBITED
    else if (audio && !channels2 && !preemphasis && !copy_prohibited) control_nybble = 0xA; // 0b1010 = AUDIO_4CH_NO_PREEMPHASIS_COPY_PERMITTED
    else if (audio && !channels2 && preemphasis && !copy_prohibited) control_nybble = 0xB; // 0b1011 = AUDIO_4CH_PREEMPHASIS_COPY_PERMITTED
    else {
        qFatal("Subcode::to_data(): Invalid control nybble! Must be 0-3, 4-7 or 8-11");
    }

    // The Q-channel data is constructed from the Q-mode (4 bits) and control bits (4 bits)
    // Q-mode is 0-3 and control is 4-7
    q_channel_data[0] = control_nybble << 4 | mode_nybble;    

    // Get the frame metadata
    SectionType frame_type = section_metadata.get_section_type();
    SectionTime f_time = section_metadata.get_section_time();
    SectionTime ap_time = section_metadata.get_absolute_section_time();
    uint8_t track_number = section_metadata.get_track_number();

    // Sanity check the track number and frame type
    //
    // If the track number is 0, then this is a lead-in frame
    // If the track number is 0xAA, then this is a lead-out frame
    // If the track number is 1-99, then this is a user data frame
    if (track_number == 0 && frame_type != SectionType::LEAD_IN) {
        qFatal("Subcode::to_data(): Track number 0 is only valid for lead-in frames");
    } else if (track_number == 0xAA && frame_type != SectionType::LEAD_OUT) {
        qFatal("Subcode::to_data(): Track number 0xAA is only valid for lead-out frames");
    } else if (track_number > 99) {
        qFatal("Subcode::to_data(): Track number %d is out of range", track_number);
    }

    // Set the Q-channel data
    if (frame_type == SectionType::LEAD_IN) {
        uint16_t tno = 0x00;
        uint16_t pointer = 0x00;
        uint8_t zero = 0;

        q_channel_data[1] = tno;
        q_channel_data[2] = pointer;
        q_channel_data[3] = f_time.to_bcd()[0];
        q_channel_data[4] = f_time.to_bcd()[1];
        q_channel_data[5] = f_time.to_bcd()[2];
        q_channel_data[6] = zero;
        q_channel_data[7] = ap_time.to_bcd()[0];
        q_channel_data[8] = ap_time.to_bcd()[1];
        q_channel_data[9] = ap_time.to_bcd()[2];
    }
 
    if (frame_type == SectionType::USER_DATA) {
        uint8_t tno = int_to_bcd2(track_number);
        uint8_t index = 01; // Not correct?
        uint8_t zero = 0;

        q_channel_data[1] = tno;
        q_channel_data[2] = index;
        q_channel_data[3] = f_time.to_bcd()[0];
        q_channel_data[4] = f_time.to_bcd()[1];
        q_channel_data[5] = f_time.to_bcd()[2];
        q_channel_data[6] = zero;
        q_channel_data[7] = ap_time.to_bcd()[0];
        q_channel_data[8] = ap_time.to_bcd()[1];
        q_channel_data[9] = ap_time.to_bcd()[2];
    }

    if (frame_type == SectionType::LEAD_OUT) {
        uint16_t tno = 0xAA; // Hexidecimal AA for lead-out
        uint16_t index = 01; // Must be 01 for lead-out
        uint8_t zero = 0;

        q_channel_data[1] = tno;
        q_channel_data[2] = index;
        q_channel_data[3] = f_time.to_bcd()[0];
        q_channel_data[4] = f_time.to_bcd()[1];
        q_channel_data[5] = f_time.to_bcd()[2];
        q_channel_data[6] = zero;
        q_channel_data[7] = ap_time.to_bcd()[0];
        q_channel_data[8] = ap_time.to_bcd()[1];
        q_channel_data[9] = ap_time.to_bcd()[2];
    }

    // Set the CRC
    set_q_channel_crc(q_channel_data); // Sets data[10] and data[11]

    // Now we need to convert the p-channel and q-channel data into a 98 byte array
    QByteArray data;
    data.resize(98);
    data[0] = 0x00; // Sync0
    data[1] = 0x00; // Sync1

    for (int index = 2; index < 98; index++) {
        uint8_t subcode_byte = 0x00;
        if (get_bit(p_channel_data, index-2)) subcode_byte |= 0x80;
        if (get_bit(q_channel_data, index-2)) subcode_byte |= 0x40;
        data[index] = subcode_byte;
    }

    return data;
}

// Set a bit in a byte array
void Subcode::set_bit(QByteArray& data, uint8_t bit_position, bool value) {
    // Check to ensure the bit position is valid
    if (bit_position >= data.size() * 8) {
        qFatal("Subcode::set_bit(): Bit position %d is out of range for data size %d", bit_position, data.size());
    }

    // We need to convert this to a byte number and bit number within that byte
    uint8_t byte_number = bit_position / 8;
    uint8_t bit_number = 7 - (bit_position % 8);

    // Set the bit
    if (value) {
        data[byte_number] = static_cast<uchar>(data[byte_number] | (1 << bit_number)); // Set bit
    } else {
        data[byte_number] = static_cast<uchar>(data[byte_number] & ~(1 << bit_number)); // Clear bit
    }
}

// Get a bit from a byte array
bool Subcode::get_bit(const QByteArray& data, uint8_t bit_position) {
    // Check to ensure we don't overflow the data array
    if (bit_position >= data.size() * 8) {
        qFatal("Subcode::get_bit(): Bit position %d is out of range for data size %d", bit_position, data.size());
    }

    // We need to convert this to a byte number and bit number within that byte
    uint8_t byte_number = bit_position / 8;
    uint8_t bit_number = 7 - (bit_position % 8);

    // Get the bit
    return (data[byte_number] & (1 << bit_number)) != 0;
}

bool Subcode::is_crc_valid(QByteArray q_channel_data) {
    // Get the CRC from the data
    uint16_t data_crc = get_q_channel_crc(q_channel_data);

    // Calculate the CRC
    uint16_t calculated_crc = calculate_q_channel_crc16(q_channel_data);

    // Check if the CRC is valid
    return data_crc == calculated_crc;
}

uint16_t Subcode::get_q_channel_crc(QByteArray q_channel_data) {
    // Get the CRC from the data
    return static_cast<uint16_t>(static_cast<uchar>(q_channel_data[10]) << 8 | static_cast<uchar>(q_channel_data[11]));
}

void Subcode::set_q_channel_crc(QByteArray& q_channel_data) {
    // Calculate the CRC
    uint16_t calculated_crc = calculate_q_channel_crc16(q_channel_data);

    // Set the CRC in the data
    q_channel_data[10] = static_cast<uint8_t>(calculated_crc >> 8);
    q_channel_data[11] = static_cast<uint8_t>(calculated_crc & 0xFF);
}

// Generate a 16-bit CRC for the subcode data
// Adapted from http://mdfs.net/Info/Comp/Comms/CRC16.htm
uint16_t Subcode::calculate_q_channel_crc16(const QByteArray &q_channel_data)
{
    int32_t i;
    uint32_t crc = 0;

    // Remove the last 2 bytes
    QByteArray data = q_channel_data.left(q_channel_data.size() - 2);

    for (int pos = 0; pos < data.size(); pos++) {
        crc = crc ^ static_cast<uint32_t>(static_cast<uchar>(data[pos]) << 8);
        for (i = 0; i < 8; i++) {
            crc = crc << 1;
            if (crc & 0x10000) crc = (crc ^ 0x1021) & 0xFFFF;
        }
    }

    // Invert the CRC
    crc = ~crc & 0xFFFF;

    return static_cast<uint16_t>(crc);
}

// Because of the way Q-channel data is spread over many frames, the most
// likely cause of a CRC error is a single bit error in the data. We can
// attempt to repair the data by flipping each bit in turn and checking
// the CRC.
//
// Perhaps there is some more effective way to repair the data, but this
// will do for now.
bool Subcode::repair_data(QByteArray &q_channel_data) {
    QByteArray data_copy = q_channel_data;

    // 96-16 = Don't repair CRC bits
    for (int i = 0; i < 96-16; i++) {
        data_copy = q_channel_data;
        data_copy[i / 8] = static_cast<uchar>(data_copy[i / 8] ^ (1 << (7 - (i % 8))));

        if (is_crc_valid(data_copy)) {
            q_channel_data = data_copy;
            return true;
        }
    }

    return false;
}

// Convert integer to BCD (Binary Coded Decimal)
// Output is always 2 nybbles (00-99)
uint8_t Subcode::int_to_bcd2(uint8_t value) {
    if (value > 99) {
        qFatal("Subcode::int_to_bcd2(): Value must be in the range 0 to 99. Got %d", value);
    }

    uint16_t bcd = 0;
    uint16_t factor = 1;

    while (value > 0) {
        bcd += (value % 10) * factor;
        value /= 10;
        factor *= 16;
    }

    // Ensure the result is always 2 bytes (00-99)
    return bcd & 0xFF;
}

// Convert BCD (Binary Coded Decimal) to integer
uint8_t Subcode::bcd2_to_int(uint8_t bcd) {
    uint16_t value = 0;
    uint16_t factor = 1;

    while (bcd > 0) {
        value += (bcd & 0x0F) * factor;
        bcd >>= 4;
        factor *= 10;
    }

    return value;
}