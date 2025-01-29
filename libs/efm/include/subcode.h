/************************************************************************

    subcode.h

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

#ifndef SUBCODE_H
#define SUBCODE_H

#include <cstdint>
#include <QByteArray>
#include <QString>
#include "frame.h"

class Pchannel {
public:
    Pchannel();
    void set_flag(bool _flag);
    bool get_bit(uint8_t index) const;

private:
    bool flag;
};

class Qchannel {
public:
    enum QModes {
        QMODE_1,
        QMODE_2,
        QMODE_3,
        QMODE_4
    };
    enum Control {
        AUDIO_4CH_NO_PREEMPHASIS_COPY_PROHIBITED,
        AUDIO_4CH_PREEMPHASIS_COPY_PROHIBITED,
        AUDIO_4CH_NO_PREEMPHASIS_COPY_PERMITTED,
        AUDIO_4CH_PREEMPHASIS_COPY_PERMITTED,
        AUDIO_2CH_NO_PREEMPHASIS_COPY_PROHIBITED,
        AUDIO_2CH_PREEMPHASIS_COPY_PROHIBITED,
        AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED,
        AUDIO_2CH_PREEMPHASIS_COPY_PERMITTED,
        DIGITAL_COPY_PROHIBITED,
        DIGITAL_COPY_PERMITTED,
    };

    Qchannel();
    bool get_bit(uint8_t index) const;
    void set_bit(uint8_t index, bool value);

    void set_q_mode_1(Control _control, uint8_t _track_number, FrameTime _f_time, FrameTime _ap_time, FrameType _frame_type);
    void set_q_mode_4(Control _control, uint8_t _track_number, FrameTime _f_time, FrameTime _ap_time, FrameType _frame_type);

    QModes get_q_mode() const { return q_mode; }
    Control get_control() const { return control; }

    FrameType get_frame_type() const { return frame_type; }
    FrameTime get_frame_time() const { return f_time; }
    FrameTime get_ap_time() const { return ap_time; }
    uint8_t get_track_number() const { return track_number; }

    bool is_valid();

private:
    QModes q_mode;
    Control control;
    QByteArray q_channel_data; // 12 bytes (96 bits)
    FrameType frame_type;
    uint8_t track_number;
    FrameTime f_time;
    FrameTime ap_time;
    bool q_channel_data_valid;

    void set_conmode();
    void set_control(Control _control);
    void repair_data();
    QByteArray generate_crc(QByteArray data);
    bool is_crc_valid(QByteArray data);
    uint16_t crc16(const QByteArray &data);
    void set_q_mode_1or4(uint8_t track_number, FrameTime f_time, FrameTime ap_time, FrameType frame_type);
    uint16_t int_to_bcd2(uint16_t value);
    uint16_t bcd2_to_int(uint16_t bcd);
    bool refresh_q_channel_from_data();
};

class Subcode {
public:
    Subcode();
    uint8_t get_subcode_byte(uint8_t index) const;
    void set_subcode_byte(uint8_t index, uint8_t value);

    Pchannel p_channel;
    Qchannel q_channel;
};

#endif // SUBCODE_H