/************************************************************************

    section.h

    EFM-library - EFM Section classes
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

#ifndef SECTION_H
#define SECTION_H

#include <QVector>
#include "frame.h"
#include "subcode.h"

class Section {
public:
    Section();

    void push_frame(F2Frame f2_frame);
    F2Frame get_f2_frame(int index) const;
    bool is_complete() const;
    uint8_t get_subcode_byte(int index) const;
    void clear();

    Subcode subcode;

private:
    QVector<F2Frame> f2_frames;
};

#endif // SECTION_H
