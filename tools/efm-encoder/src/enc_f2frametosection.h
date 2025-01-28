/************************************************************************

    enc_f2frametosection.h

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

#ifndef ENC_F2FRAMETOSECTION_H
#define ENC_F2FRAMETOSECTION_H

#include "encoders.h"
#include "section.h"

class F2FrameToSection : Encoder {
public:
    F2FrameToSection();
    void push_frame(F2Frame f2_frame);
    Section pop_section();
    bool is_ready() const;
    uint32_t get_valid_output_frames_count() const override { return valid_sections_count; };

    void set_qmode_options(Qchannel::QModes _qmode, Qchannel::Control _qcontrol);

private:
    void process_queue();

    QQueue<F2Frame> input_buffer;
    QQueue<Section> output_buffer;

    uint32_t valid_sections_count;

    // Q-Channel options
    Qchannel::QModes qmode;
    Qchannel::Control qcontrol;
};

#endif // ENC_F2FRAMETOSECTION_H
