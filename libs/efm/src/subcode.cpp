/************************************************************************

    subcode.cpp

    EFM-library - Subcode channel functions
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

#include <QDebug>
#include "subcode.h"

// P-channel class -----------------------------------------------------------------------------------------------------
Pchannel::Pchannel() {
    flag = false;
}

void Pchannel::set_flag(bool _flag) {
    flag = _flag;
}

bool Pchannel::get_bit(uint8_t index) const {
    // Since the p-channel has the same value for
    // all bits, we can just return the flag value here
    return flag;
}

// Q-channel class -----------------------------------------------------------------------------------------------------
Qchannel::Qchannel() {
    // The Q-channel data is 12 bytes long (96 bits)
    q_channel_data.resize(12);
    q_channel_data.fill(0);
    
    // q Channel defaults
    q_mode = QMODE_1;
    set_control(AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED);
    frame_type = SubcodeFrameType::USER_DATA;
    track_number = 1;
}

void Qchannel::set_q_mode_1(Control _control, uint8_t _track_number, FrameTime _f_time, FrameTime _ap_time, SubcodeFrameType _frame_type) {
    q_mode = QMODE_1;
    set_control(_control);
    set_q_mode_1or4(track_number, f_time, ap_time, frame_type);
}

// To-Do: Implement the set_q_mode_2, set_q_mode_3 functions

void Qchannel::set_q_mode_4(Control _control, uint8_t _track_number, FrameTime _f_time, FrameTime _ap_time, SubcodeFrameType _frame_type) {
    q_mode = QMODE_4;
    set_control(_control);
    set_q_mode_1or4(_track_number, _f_time, _ap_time, _frame_type);
}

// The only difference between Q-mode 1 and 4 is the control bits, so we can use the same function for both
// to save code duplication
void Qchannel::set_q_mode_1or4(uint8_t _track_number, FrameTime _f_time, FrameTime _ap_time, SubcodeFrameType _frame_type) {
    if (frame_type == SubcodeFrameType::LEAD_IN) track_number = 0;
    if (frame_type == SubcodeFrameType::LEAD_OUT) track_number = 0;
    if ((frame_type == SubcodeFrameType::USER_DATA) && (track_number < 1 || track_number > 98)) {
        qFatal("Qchannel::generate_frame(): Track number must be in the range 1 to 98.");
    }
    if ((frame_type == SubcodeFrameType::USER_DATA) && (track_number > 0 || track_number < 99)) {
        track_number = _track_number;
    }

    frame_type = _frame_type;
    f_time = _f_time;
    ap_time = _ap_time;

    // Set the Q-channel data
    if (frame_type == SubcodeFrameType::LEAD_IN) {
        uint16_t tno = 0x00;
        uint16_t pointer = 0x00;
        uint8_t zero = 0;

        set_conmode(); // Set the control and mode bits q_data_channel[0]
        q_channel_data[1] = tno;
        q_channel_data[2] = pointer;
        q_channel_data[3] = int_to_bcd2(f_time.get_min());
        q_channel_data[4] = int_to_bcd2(f_time.get_sec());
        q_channel_data[5] = int_to_bcd2(f_time.get_frame());
        q_channel_data[6] = zero;
        q_channel_data[7] = int_to_bcd2(ap_time.get_min());
        q_channel_data[8] = int_to_bcd2(ap_time.get_sec());
        q_channel_data[9] = int_to_bcd2(ap_time.get_frame());
        generate_crc();
    }
 
    if (frame_type == SubcodeFrameType::USER_DATA) {
        uint8_t tno = int_to_bcd2(track_number);
        uint8_t index = 01; // Not correct
        uint8_t zero = 0;

        set_conmode(); // Set the control and mode bits q_data_channel[0]
        q_channel_data[1] = tno;
        q_channel_data[2] = index;
        q_channel_data[3] = int_to_bcd2(f_time.get_min());
        q_channel_data[4] = int_to_bcd2(f_time.get_sec());
        q_channel_data[5] = int_to_bcd2(f_time.get_frame());
        q_channel_data[6] = zero;
        q_channel_data[7] = int_to_bcd2(ap_time.get_min());
        q_channel_data[8] = int_to_bcd2(ap_time.get_sec());
        q_channel_data[9] = int_to_bcd2(ap_time.get_frame());
        generate_crc();
    }

    if (frame_type == SubcodeFrameType::LEAD_OUT) {
        uint16_t tno = 0xAA; // Hexidecimal AA for lead-out
        uint16_t index = 01; // Must be 01 for lead-out
        uint8_t zero = 0;

        set_conmode(); // Set the control and mode bits q_data_channel[0]
        q_channel_data[1] = tno;
        q_channel_data[2] = index;
        q_channel_data[3] = int_to_bcd2(f_time.get_min());
        q_channel_data[4] = int_to_bcd2(f_time.get_sec());
        q_channel_data[5] = int_to_bcd2(f_time.get_frame());
        q_channel_data[6] = zero;
        q_channel_data[7] = int_to_bcd2(ap_time.get_min());
        q_channel_data[8] = int_to_bcd2(ap_time.get_sec());
        q_channel_data[9] = int_to_bcd2(ap_time.get_frame());
        generate_crc();
    }
}
    
void Qchannel::set_control(Control _control) {
    control = _control;
    set_conmode();
}

void Qchannel::set_conmode() {
    uint8_t mode_nybble = 0;
    uint8_t control_nybble = 0;

    switch (q_mode) {
        case QMODE_1:
            mode_nybble = 0x1; // 0b0001
            break;
        case QMODE_2:
            mode_nybble = 0x2; // 0b0010
            break;
        case QMODE_3:
            mode_nybble = 0x3; // 0b0011
            break;
        case QMODE_4:
            mode_nybble = 0x4; // 0b0100
            break;
    }

    switch (control) {
        case AUDIO_2CH_NO_PREEMPHASIS_COPY_PROHIBITED:
            control_nybble = 0x0; // 0b0000 = 0x0
            break;
        case AUDIO_2CH_PREEMPHASIS_COPY_PROHIBITED:
            control_nybble = 0x1; // 0b0001 = 0x1
            break;
        case AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED:
            control_nybble = 0x2; // 0b0010 = 0x2
            break;
        case AUDIO_2CH_PREEMPHASIS_COPY_PERMITTED:
            control_nybble = 0x3; // 0b0011 = 0x3
            break;
        case DIGITAL_COPY_PROHIBITED:
            control_nybble = 0x4; // 0b0100 = 0x4
            break;
        case DIGITAL_COPY_PERMITTED:
            control_nybble = 0x6; // 0b0110 = 0x6
            break;
    }

    // The Q-channel data is constructed from the Q-mode (4 bits) and control bits (4 bits)
    q_channel_data[0] = control_nybble << 4 | mode_nybble;
}

bool Qchannel::get_bit(uint8_t index) const {
    if (index < 2) qFatal("Qchannel::get_bit(): Invalid index (must be greater than 2 due to sync0 and sync1 bytes)");
    index -= 2; // This is due to the sync0 and sync1 bytes at the start of the subcode

    // The symbol number is the bit position in the 96-bit subcode data
    // We need to convert this to a byte number and bit number within that byte
    uint8_t byte_number = index / 8;
    uint8_t bit_number = 7 - (index % 8); // Change to make MSB at the other end

    uint8_t sc_byte = q_channel_data[byte_number];

    // Return true or false based on the required bit
    return (sc_byte & (1 << bit_number)) != 0;   
}

void Qchannel::set_bit(uint8_t index, bool value) {
    if (index < 2) qFatal("Qchannel::set_bit(): Invalid index (must be greater than 2 due to sync0 and sync1 bytes)");
    index -= 2; // This is due to the sync0 and sync1 bytes at the start of the subcode

    // The symbol number is the bit position in the 96-bit subcode data
    // We need to convert this to a byte number and bit number within that byte
    uint8_t byte_number = index / 8;
    uint8_t bit_number = 7 - (index % 8); // Change to make MSB at the other end

    if (value) {
        q_channel_data[byte_number] = static_cast<uchar>(q_channel_data[byte_number] | (1 << bit_number)); // Set bit
    } else {
        q_channel_data[byte_number] = static_cast<uchar>(q_channel_data[byte_number] & ~(1 << bit_number)); // Clear bit
    }

    // If the index is 95 (0-97, -2), then we assume that we have all the required q-channel data
    // and we have to refresh the q-channel parameters based on the data.
    if (index == 95) {
        refresh_q_channel_from_data();
    }
}

void Qchannel::refresh_q_channel_from_data() {
    qDebug() << "Qchannel::refresh_q_channel_from_data()" << q_channel_data.toHex();

    // Firstly compute the CRC and check the q-channel data is valid
    if (!is_crc_valid()) {
        qFatal("Qchannel::refresh_q_channel_from_data(): CRC is invalid");
    }

    // Set q-mode, control, track number, frame time, ap time, frame type by reading the q-channel data
    // This is the reverse of the set_q_mode_1or4 function

    // Set the control and mode bits q_data_channel[0]
    uint8_t control_nybble = q_channel_data[0] >> 4;
    uint8_t mode_nybble = q_channel_data[0] & 0x0F;

    switch (mode_nybble) {
        case 0x1:
            q_mode = QMODE_1;
            break;
        case 0x2:
            q_mode = QMODE_2;
            break;
        case 0x3:
            q_mode = QMODE_3;
            break;
        case 0x4:
            q_mode = QMODE_4;
            break;
    }

    switch (control_nybble) {
        case 0x0:
            control = AUDIO_2CH_NO_PREEMPHASIS_COPY_PROHIBITED;
            break;
        case 0x1:
            control = AUDIO_2CH_PREEMPHASIS_COPY_PROHIBITED;
            break;
        case 0x2:
            control = AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED;
            break;
        case 0x3:
            control = AUDIO_2CH_PREEMPHASIS_COPY_PERMITTED;
            break;
        case 0x4:
            control = DIGITAL_COPY_PROHIBITED;
            break;
        case 0x6:
            control = DIGITAL_COPY_PERMITTED;
            break;
    }

    // Set the track number q_data_channel[1]
    track_number = bcd2_to_int(q_channel_data[1]);

    // If the track number is 0, then this is a lead-in frame
    // If the track number is 0xAA, then this is a lead-out frame
    // If the track number is 1-99, then this is a user data frame
    if (track_number == 0) {
        frame_type = SubcodeFrameType::LEAD_IN;
    } else if (track_number == 0xAA) {
        frame_type = SubcodeFrameType::LEAD_OUT;
    } else {
        frame_type = SubcodeFrameType::USER_DATA;
    }

    // Set the index/pointer q_data_channel[2] - Not used at the moment

    // Set the frame time q_data_channel[3-5]
    f_time.set_min(bcd2_to_int(q_channel_data[3]));
    f_time.set_sec(bcd2_to_int(q_channel_data[4]));
    f_time.set_frame(bcd2_to_int(q_channel_data[5]));

    // Set the zero byte q_data_channel[6] - Not used at the moment

    // Set the ap time q_data_channel[7-9]
    ap_time.set_min(bcd2_to_int(q_channel_data[7]));
    ap_time.set_sec(bcd2_to_int(q_channel_data[8]));
    ap_time.set_frame(bcd2_to_int(q_channel_data[9]));
}

void Qchannel::generate_crc() {
    // Generate the CRC for the channel data
    // CRC is on control+mode+data 4+4+72 = 80 bits with 16-bit CRC (96 bits total)
    uint16_t crc = crc16(q_channel_data.mid(0, 10));

    // Invert the CRC
    crc = ~crc & 0xFFFF;

    // CRC is 2 bytes
    q_channel_data[10] = crc >> 8;
    q_channel_data[11] = crc & 0xFF;
}

bool Qchannel::is_crc_valid() {
    // Check the CRC for the channel data
    // CRC is on control+mode+data 4+4+72 = 80 bits with 16-bit CRC (96 bits total)
    uint16_t crc = crc16(q_channel_data.mid(0, 10));

    // Invert the CRC
    crc = ~crc & 0xFFFF;

    // Get the CRC from the data
    uint16_t data_crc = static_cast<uchar>(q_channel_data[10]) << 8 | static_cast<uchar>(q_channel_data[11]);

    // Compare the CRCs...
    if (crc != data_crc) {
        return false;
    }
    return true;
}

// Generate a 16-bit CRC for the subcode data
// Adapted from http://mdfs.net/Info/Comp/Comms/CRC16.htm
uint16_t Qchannel::crc16(const QByteArray &data)
{
    int32_t i;
    uint32_t crc = 0;

    for (int pos = 0; pos < data.size(); pos++) {
        crc = crc ^ static_cast<uint32_t>(static_cast<uchar>(data[pos]) << 8);
        for (i = 0; i < 8; i++) {
            crc = crc << 1;
            if (crc & 0x10000) crc = (crc ^ 0x1021) & 0xFFFF;
        }
    }

    return static_cast<uint16_t>(crc);
}

// Convert integer to BCD (Binary Coded Decimal)
// Output is always 2 bytes (00-99)
uint16_t Qchannel::int_to_bcd2(uint16_t value) {
    if (value > 99) {
        qFatal("Qchannel::int_to_bcd2(): Value must be in the range 0 to 99.");
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

uint16_t Qchannel::bcd2_to_int(uint16_t bcd) {
    uint16_t value = 0;
    uint16_t factor = 1;

    while (bcd > 0) {
        value += (bcd & 0x0F) * factor;
        bcd >>= 4;
        factor *= 10;
    }

    return value;
}

// Subcode class -------------------------------------------------------------------------------------------------------
Subcode::Subcode() {
    // Set the Q-channel to default (Q-mode 1, audio 2-channel, no preemphasis, copy permitted)
    p_channel.set_flag(false);
    q_channel.set_q_mode_1(Qchannel::Control::AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED, 1,
        FrameTime(0, 0, 0), FrameTime(0, 0, 0), Qchannel::SubcodeFrameType::USER_DATA);
}

uint8_t Subcode::get_subcode_byte(uint8_t index) const {
    if (index < 2) return 0; // The first two subcode bytes are always zero (sync0 and sync1)

    uint8_t subcode_byte = 0;
    if (p_channel.get_bit(index)) subcode_byte |= 0x80;
    if (q_channel.get_bit(index)) subcode_byte |= 0x40;

    // Note: channels R through W are reserved and always zero
    
    return subcode_byte;
}

void Subcode::set_subcode_byte(uint8_t index, uint8_t value) {
    if (index < 2) qFatal("Subcode::set_subcode_byte(): Invalid index (must be greater than 2 due to sync0 and sync1 bytes)");

    // The P-channel is the MSB of the subcode byte
    p_channel.set_flag(value & 0x80);

    // The Q-channel is the next bit
    q_channel.set_bit(index, value & 0x40);

    // Note: channels R through W are reserved and always zero
}