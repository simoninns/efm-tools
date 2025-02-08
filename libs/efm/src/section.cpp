/************************************************************************

    section.cpp

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

#include "section.h"

F2Section::F2Section() {
    frames.reserve(98);
}

void F2Section::push_frame(F2Frame in_frame) {
    if (frames.size() >= 98) {
        qFatal("F2Section::push_frame - Section is full");
    }
    frames.push_back(in_frame);
}

F2Frame F2Section::get_frame(int index) const {
    if (index >= frames.size() || index < 0) {
        qFatal("F2Section::get_frame - Index %d out of range", index);
    }
    return frames.at(index);
}

void F2Section::set_frame(int index, F2Frame in_frame) {
    if (index >= frames.size() || index < 0) {
        qFatal("F2Section::set_frame - Index %d out of range", index);
    }
    frames[index] = in_frame;
}

bool F2Section::is_complete() const {
    return frames.size() == 98;
}

void F2Section::clear() {
    frames.clear();
}

void F2Section::show_data() {
    for (int i = 0; i < frames.size(); ++i) {
        frames[i].show_data();
    }
}

F1Section::F1Section() {
    frames.reserve(98);
}

void F1Section::push_frame(F1Frame in_frame) {
    if (frames.size() >= 98) {
        qFatal("F1Section::push_frame - Section is full");
    }
    frames.push_back(in_frame);
}

F1Frame F1Section::get_frame(int index) const {
    if (index >= frames.size() || index < 0) {
        qFatal("F1Section::get_frame - Index %d out of range", index);
    }
    return frames.at(index);
}

void F1Section::set_frame(int index, F1Frame in_frame) {
    if (index >= 98 || index < 0) {
        qFatal("F1Section::set_frame - Index %d out of range", index);
    }
    frames[index] = in_frame;
}

bool F1Section::is_complete() const {
    return frames.size() == 98;
}

void F1Section::clear() {
    frames.clear();
}

void F1Section::show_data() {
    for (int i = 0; i < frames.size(); ++i) {
        frames[i].show_data();
    }
}

Data24Section::Data24Section() {
    frames.reserve(98);
}

void Data24Section::push_frame(Data24 in_frame) {
    if (frames.size() >= 98) {
        qFatal("Data24Section::push_frame - Section is full");
    }
    frames.push_back(in_frame);
}

Data24 Data24Section::get_frame(int index) const {
    if (index >= frames.size() || index < 0) {
        qFatal("Data24Section::get_frame - Index %d out of range", index);
    }
    return frames.at(index);
}

void Data24Section::set_frame(int index, Data24 in_frame) {
    if (index >= frames.size() || index < 0) {
        qFatal("Data24Section::set_frame - Index %d out of range", index);
    }
    frames[index] = in_frame;
}

bool Data24Section::is_complete() const {
    return frames.size() == 98;
}

void Data24Section::clear() {
    frames.clear();
}

void Data24Section::show_data() {
    for (int i = 0; i < frames.size(); ++i) {
        frames[i].show_data();
    }
}