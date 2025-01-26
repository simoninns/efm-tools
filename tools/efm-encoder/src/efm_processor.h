/************************************************************************

    efm_processor.h

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

#ifndef EFMPROCESSOR_H
#define EFMPROCESSOR_H

#include <QString>

class EfmProcessor
{
public:
    EfmProcessor();

    bool process(QString input_fileName, QString output_fileName);
    void set_show_data(bool _showInput, bool _showF1, bool _showF2, bool _showF3);
    void set_input_type(bool _wavInput);
    void set_corruption(bool _corrupt_tvalues, uint32_t _corrupt_tvalues_frequency, bool _pad_start, uint32_t _pad_start_symbols);

private:
    // Show data flags
    bool showInput;
    bool showF1;
    bool showF2;
    bool showF3;

    // Input type flag
    bool is_input_data_wav;

    // Corruption flags
    bool corrupt_tvalues;
    uint32_t corrupt_tvalues_frequency;
    bool pad_start;
    uint32_t pad_start_symbols;
};

#endif // EFMPROCESSOR_H