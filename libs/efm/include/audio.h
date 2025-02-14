/************************************************************************

    audio.h

    EFM-library - Audio frame type class
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

#ifndef AUDIO_H
#define AUDIO_H

#include <QVector>
#include <cstdint>
#include <QDebug>

// Audio class
class Audio {
public:
    void set_data(const QVector<int16_t>& data);
    QVector<int16_t> get_data() const;

    void set_error_data(const QVector<int16_t>& error_data);
    QVector<int16_t> get_error_data() const;
    uint32_t count_errors() const;

    bool is_full() const;
    bool is_empty() const;

    void show_data();

    int get_frame_size() const;

private:
    QVector<int16_t> audio_data;
    QVector<int16_t> audio_error_data;
};

#endif // AUDIO_H