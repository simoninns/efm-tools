/************************************************************************

    dec_data24toaudio.cpp

    ld-efm-decoder - EFM data decoder
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

#include "dec_data24toaudio.h"

Data24ToAudio::Data24ToAudio()
{
    start_time = SectionTime(59, 59, 74);
    end_time = SectionTime(0, 0, 0);

    invalid_data24_frames_count = 0;
    valid_data24_frames_count = 0;
    invalid_samples_count = 0;
    valid_samples_count = 0;
}

void Data24ToAudio::push_section(Data24Section data24_section)
{
    // Add the data to the input buffer
    input_buffer.enqueue(data24_section);

    // Process the queue
    process_queue();
}

AudioSection Data24ToAudio::pop_section()
{
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool Data24ToAudio::is_ready() const
{
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void Data24ToAudio::process_queue()
{
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        Data24Section data24_section = input_buffer.dequeue();
        AudioSection audio_section;

        // Sanity check the Data24 section
        if (data24_section.is_complete() == false) {
            qFatal("Data24ToAudio::process_queue - Data24 Section is not complete");
        }

        for (int index = 0; index < 98; index++) {
            QVector<uint8_t> data24_data = data24_section.get_frame(index).get_data();
            QVector<uint8_t> data24_error_data = data24_section.get_frame(index).get_error_data();

            if (data24_section.get_frame(index).count_errors() != 0) {
                invalid_data24_frames_count++;
            } else {
                valid_data24_frames_count++;
            }

            // Convert the 24 bytes of data into 12 16-bit audio samples
            QVector<int16_t> audio_data;
            QVector<int16_t> audio_error_data;
            for (int i = 0; i < 24; i += 2) {
                int16_t sample = (data24_data[i + 1] << 8) | data24_data[i];
                audio_data.append(sample);

                // Set an error flag if either byte of the sample is an error
                if (data24_error_data[i + 1] == 1 || data24_error_data[i] == 1) {
                    audio_error_data.append(1);
                    invalid_samples_count++;
                } else {
                    audio_error_data.append(0);
                    valid_samples_count++;
                }
            }

            // Put the resulting data into a Audio frame and push it to the output buffer
            Audio audio;
            audio.set_data(audio_data);
            audio.set_error_data(audio_error_data);

            audio_section.push_frame(audio);
        }

        audio_section.metadata = data24_section.metadata;

        if (audio_section.metadata.get_absolute_section_time() < start_time) {
            start_time = audio_section.metadata.get_absolute_section_time();
        }

        if (audio_section.metadata.get_absolute_section_time() >= end_time) {
            end_time = audio_section.metadata.get_absolute_section_time();
        }

        // Add the section to the output buffer
        output_buffer.enqueue(audio_section);
    }
}

void Data24ToAudio::show_statistics()
{
    qInfo() << "Data24 to Audio statistics:";
    qInfo().nospace() << "  Data24 Frames:";
    qInfo().nospace() << "    Total Frames: "
                      << valid_data24_frames_count + invalid_data24_frames_count;
    qInfo().nospace() << "    Valid Frames: " << valid_data24_frames_count;
    qInfo().nospace() << "    Invalid Frames: " << invalid_data24_frames_count;

    qInfo() << "  Audio Samples:";
    qInfo().nospace() << "    Total stereo samples: "
                      << (valid_samples_count + invalid_samples_count) / 2;
    qInfo().nospace() << "    Valid stereo samples: " << valid_samples_count / 2;
    qInfo().nospace() << "    Corrupt stereo samples: " << invalid_samples_count / 2;

    qInfo() << "  Section time information:";
    qInfo().noquote() << "    Start time:" << start_time.to_string();
    qInfo().noquote() << "    End time:" << end_time.to_string();
    qInfo().noquote() << "    Total time:" << (end_time - start_time).to_string();
}