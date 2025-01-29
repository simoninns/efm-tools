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

Section::Section() {
    // Set some sane defaults for the subcode channels
    subcode.p_channel.set_flag(false);
    subcode.q_channel.set_q_mode_1(Qchannel::Control::AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED, 1, FrameTime(0, 0, 0), FrameTime(0, 0, 0), FrameType::USER_DATA);
}

void Section::push_frame(F2Frame f2_frame) {
    f2_frames.push_back(f2_frame);
}

F2Frame Section::get_f2_frame(int index) const {
    return f2_frames.at(index);
}

bool Section::is_complete() const {
    return f2_frames.size() == 98;
}

uint8_t Section::get_subcode_byte(int index) const {
    return subcode.get_subcode_byte(index);
}

void Section::clear() {
    f2_frames.clear();

    // Set some sane defaults for the subcode channels
    subcode.p_channel.set_flag(false);
    subcode.q_channel.set_q_mode_1(Qchannel::Control::AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED, 1, FrameTime(0, 0, 0), FrameTime(0, 0, 0), FrameType::USER_DATA);
}