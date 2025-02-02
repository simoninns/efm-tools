/************************************************************************

    frame.h

    EFM-library - EFM Frame type classes
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

#ifndef FRAME_H
#define FRAME_H

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

// Frame class - base class for F1, F2, and F3 frames
class Frame {
public:
    virtual ~Frame() {} // Virtual destructor
    virtual int get_frame_size() const = 0; // Pure virtual function to get frame size

    virtual void set_data(const QVector<uint8_t>& data);
    virtual QVector<uint8_t> get_data() const;

    bool is_full() const;
    bool is_empty() const;

protected:
    QVector<uint8_t> frame_data;
};

class Data24 : public Frame {
public:
    Data24();
    int get_frame_size() const override;
    void show_data();
    void set_data(const QVector<uint8_t>& data) override;

    void set_frame_type(FrameType _frame_type);
    FrameType get_frame_type() const;
    void set_frame_time(const FrameTime& _frame_time);
    FrameTime get_frame_time() const;
    void set_track_number(uint8_t _track_number);
    uint8_t get_track_number() const;

private:
    uint8_t track_number;
    FrameTime frame_time;
    FrameType frame_type;
};

class F1Frame : public Frame {
public:
    F1Frame();
    int get_frame_size() const override;
    void show_data();

    void set_error_data(const QVector<uint8_t>& error_data);
    QVector<uint8_t> get_error_data() const;

    void set_frame_type(FrameType _frame_type);
    FrameType get_frame_type() const;
    void set_frame_time(const FrameTime& _frame_time);
    FrameTime get_frame_time() const;
    void set_absolute_frame_time(const FrameTime& _frame_time);
    FrameTime get_absolute_frame_time() const;
    void set_track_number(uint8_t _track_number);
    uint8_t get_track_number() const;

private:
    // Subcode data
    uint8_t track_number;
    FrameTime frame_time;
    FrameTime absolute_frame_time;
    FrameType frame_type;

    // Error data
    QVector<uint8_t> frame_error_data;
};

class F2Frame : public Frame {
public:
    F2Frame();
    int get_frame_size() const override;
    void show_data();
    
    void set_frame_type(FrameType _frame_type);
    FrameType get_frame_type() const;
    void set_frame_time(const FrameTime& _frame_time);
    FrameTime get_frame_time() const;
    void set_absolute_frame_time(const FrameTime& _frame_time);
    FrameTime get_absolute_frame_time() const;
    void set_track_number(uint8_t _track_number);
    uint8_t get_track_number() const;

private:
    // Subcode data
    uint8_t track_number;
    FrameTime frame_time;
    FrameTime absolute_frame_time;
    FrameType frame_type;
};

class F3Frame : public Frame {
public:
    enum F3FrameType { SUBCODE, SYNC0, SYNC1 };

    F3Frame();
    int get_frame_size() const override;

    void set_frame_type_as_subcode(uint8_t subcode);
    void set_frame_type_as_sync0();
    void set_frame_type_as_sync1();

    F3FrameType get_f3_frame_type() const;
    QString get_f3_frame_type_as_string() const;
    uint8_t get_subcode_byte() const;

    void show_data();

private:
    F3FrameType f3_frame_type;
    uint8_t subcode_byte;
};

#endif // FRAME_H