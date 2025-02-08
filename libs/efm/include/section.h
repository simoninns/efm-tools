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
#include "section_metadata.h"

class F2Section {
public:
    F2Section();
    void push_frame(F2Frame in_frame);
    F2Frame get_frame(int index) const;
    void set_frame(int index, F2Frame in_frame);
    bool is_complete() const;
    void clear();
    void show_data();

    SectionMetadata metadata;

private:
    QVector<F2Frame> frames;
};

class F1Section {
public:
    F1Section();
    void push_frame(F1Frame in_frame);
    F1Frame get_frame(int index) const;
    void set_frame(int index, F1Frame in_frame);
    bool is_complete() const;
    void clear();
    void show_data();

    SectionMetadata metadata;

private:
    QVector<F1Frame> frames;
};

class Data24Section {
public:
    Data24Section();
    void push_frame(Data24 in_frame);
    Data24 get_frame(int index) const;
    void set_frame(int index, Data24 in_frame);
    bool is_complete() const;
    void clear();
    void show_data();

    SectionMetadata metadata;

private:
    QVector<Data24> frames;
};

#endif // SECTION_H
