/************************************************************************

    enc_data24sectiontof1section.cpp

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-encoder is free software: you can redistribute it and/or
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

#include "enc_data24sectiontof1section.h"

// Data24SectionToF1Section class implementation
Data24SectionToF1Section::Data24SectionToF1Section()
{
    valid_f1_sections_count = 0;
}

void Data24SectionToF1Section::push_section(Data24Section data24_section)
{
    input_buffer.enqueue(data24_section);
    process_queue();
}

F1Section Data24SectionToF1Section::pop_section()
{
    if (!is_ready()) {
        qFatal("Data24SectionToF1Section::pop_frame(): No F1 sections are available.");
    }
    return output_buffer.dequeue();
}

void Data24SectionToF1Section::process_queue()
{
    while (!input_buffer.isEmpty()) {
        Data24Section data24_section = input_buffer.dequeue();
        F1Section f1_section;
        f1_section.metadata = data24_section.metadata;

        for (int index = 0; index < 98; ++index) {
            Data24 data24 = data24_section.frame(index);

            // ECMA-130 issue 2 page 16 - Clause 16
            // All byte pairs are swapped by the F1 Frame encoder
            QVector<uint8_t> data = data24.getData();
            for (int i = 0; i < data.size(); i += 2) {
                std::swap(data[i], data[i + 1]);
            }

            F1Frame f1_frame;
            f1_frame.setData(data);
            f1_section.pushFrame(f1_frame);
        }

        valid_f1_sections_count++;
        output_buffer.enqueue(f1_section);
    }
}

bool Data24SectionToF1Section::is_ready() const
{
    return !output_buffer.isEmpty();
}
