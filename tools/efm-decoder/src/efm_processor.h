/************************************************************************

    efm_processor.h

    ld-efm-decoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-decoder is free software: you can redistribute it and/or
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

#ifndef EFM_PROCESSOR_H
#define EFM_PROCESSOR_H

#include <QString>

class EfmProcessor
{
public:
    EfmProcessor();

    bool process(QString input_filename, QString output_filename);
    void set_show_data(bool _showOutput, bool _showF1, bool _showF2, bool _showF3);
    void set_output_type(bool _wavOutput);

private:
    bool showOutput;
    bool showF1;
    bool showF2;
    bool showF3;
    bool is_output_data_wav;
};

#endif // EFM_PROCESSOR_H