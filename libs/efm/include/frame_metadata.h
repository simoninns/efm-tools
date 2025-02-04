/************************************************************************

    frame_metadata.h

    EFM-library - Frame metadata classes
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

#ifndef FRAME_METADATA_H
#define FRAME_METADATA_H

#include <QVector>
#include <cstdint>

// Frame time class - stores ECMA-130 frame time as minutes, seconds, and frames
class FrameTime {
public:
    FrameTime() : min(0), sec(0), frame(0) {}
    FrameTime(uint8_t _min, uint8_t _sec, uint8_t _frame) : min(_min), sec(_sec), frame(_frame) {}
    virtual ~FrameTime() = default;

    uint8_t get_min() const { return min; }
    uint8_t get_sec() const { return sec; }
    uint8_t get_frame() const { return frame; }

    void set_min(uint8_t _min);
    void set_sec(uint8_t _sec);
    void set_frame(uint8_t _frame);

    QByteArray to_bcd() const;
    void increment_frame();
    QString to_string() const;

    int32_t get_time_in_frames() const { return (min * 60 + sec) * 75 + frame; }
    void set_time_in_frames(int32_t time_in_frames);

    bool operator==(const FrameTime& other) const;
    bool operator!=(const FrameTime& other) const;
    bool operator<(const FrameTime& other) const;
    bool operator>(const FrameTime& other) const;
    FrameTime operator+(const FrameTime& other) const;
    FrameTime operator-(const FrameTime& other) const;

private:
    uint8_t min;
    uint8_t sec;
    uint8_t frame;    
};

// Frame type class - stores the type of frame (LEAD_IN, LEAD_OUT, USER_DATA)
class FrameType {
public:
    enum Type { LEAD_IN, LEAD_OUT, USER_DATA };

    FrameType() : type(USER_DATA) {}
    FrameType(Type _type) : type(_type) {}

    Type get_type() const { return type; }
    void set_type(Type _type) { type = _type; }

    QString to_string() const {
        switch (type) {
            case LEAD_IN: return "LEAD_IN";
            case LEAD_OUT: return "LEAD_OUT";
            case USER_DATA: return "USER_DATA";
            default: return "UNKNOWN";
        }
    }

    bool operator==(const FrameType& other) const {
        return type == other.type;
    }

    bool operator!=(const FrameType& other) const {
        return type != other.type;
    }

private:
    Type type;
};

// Frame metadata class - stores the frame type, frame time, absolute frame time, and track number
// This data is common for Data24, F1 and F2 frames
// The section index is used to identify the section number when translated from F2 frames (0-97)
class FrameMetadata {
public:
    enum QModes {
        QMODE_1,
        QMODE_2,
        QMODE_3,
        QMODE_4
    };

    FrameMetadata() : frame_type(FrameType::USER_DATA), frame_time(FrameTime()), absolute_frame_time(FrameTime()), track_number(0), valid_data(false),
        is_audio_flag(true), is_copy_prohibited_flag(true), is_preemphasis_flag(false), is_2_channel_flag(true), p_flag(true), q_mode(QModes::QMODE_1) {}

    FrameType get_frame_type() const { return frame_type; }
    void set_frame_type(FrameType _frame_type);

    FrameTime get_frame_time() const { return frame_time; }
    void set_frame_time(const FrameTime& _frame_time) { frame_time = _frame_time; }

    FrameTime get_absolute_frame_time() const { return absolute_frame_time; }
    void set_absolute_frame_time(const FrameTime& _frame_time) { absolute_frame_time = _frame_time; }

    uint8_t get_track_number() const { return track_number; }
    void set_track_number(uint8_t _track_number);

    QModes get_q_mode() const { return q_mode; }
    void set_q_mode(QModes _q_mode) { q_mode = _q_mode; }

    bool is_audio() const { return is_audio_flag; }
    void set_audio(bool audio) { is_audio_flag = audio; }
    bool is_copy_prohibited() const { return is_copy_prohibited_flag; }
    void set_copy_prohibited(bool copy_prohibited) { is_copy_prohibited_flag = copy_prohibited; }
    bool is_preemphasis() const { return is_preemphasis_flag; }
    void set_preemphasis(bool preemphasis) { is_preemphasis_flag = preemphasis; }
    bool is_2_channel() const { return is_2_channel_flag; }
    void set_2_channel(bool _2_channel) { is_2_channel_flag = _2_channel; }

    bool is_p_flag() const { return p_flag; }
    void set_p_flag(bool p) { p_flag = p; }

    bool is_valid() const { return valid_data; }
    void set_valid(bool valid) { valid_data = valid; }

private:
    // P-Channel metadata
    bool p_flag;

    // Q-Channel metadata
    QModes q_mode;
    FrameType frame_type;
    FrameTime frame_time;
    FrameTime absolute_frame_time;
    uint8_t track_number;
    bool valid_data;

    // Q-Channel control metadata
    bool is_audio_flag;
    bool is_copy_prohibited_flag;
    bool is_preemphasis_flag;
    bool is_2_channel_flag;
};

#endif // FRAME_METADATA_H