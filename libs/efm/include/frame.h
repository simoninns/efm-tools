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

class F1Frame : public Frame {
public:
    F1Frame();
    int get_frame_size() const override;
    void show_data();
};

class F2Frame : public Frame {
public:
    F2Frame();
    int get_frame_size() const override;
    void show_data();
};

class F3Frame : public Frame {
public:
    enum FrameType { SUBCODE, SYNC0, SYNC1 };

    F3Frame();
    int get_frame_size() const override;

    void set_frame_type_as_subcode(uint8_t subcode);
    void set_frame_type_as_sync0();
    void set_frame_type_as_sync1();

    FrameType get_frame_type() const;
    uint8_t get_subcode() const;

    void show_data();

private:
    FrameType frame_type;
    uint8_t subcode;
};

#endif // FRAME_H