/************************************************************************

    dec_audiocorrection.cpp

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

#include "dec_audiocorrection.h"

AudioCorrection::AudioCorrection() {
    valid_samples_count = 0;
    concealed_samples_count = 0;
    silenced_samples_count = 0;

    last_section_left_sample = 0;
    last_section_right_sample = 0;
}

void AudioCorrection::push_section(AudioSection audio_section) {
    // Add the data to the input buffer
    input_buffer.enqueue(audio_section);

    // Process the queue
    process_queue();
}

AudioSection AudioCorrection::pop_section() {
    // Return the first item in the output buffer
    return output_buffer.dequeue();
}

bool AudioCorrection::is_ready() const {
    // Return true if the output buffer is not empty
    return !output_buffer.isEmpty();
}

void AudioCorrection::process_queue() {
    // Process the input buffer
    while (!input_buffer.isEmpty()) {
        AudioSection audio_section_in = input_buffer.dequeue();
        AudioSection audio_section_out;

        // Sanity check the Audio section
        if (audio_section_in.is_complete() == false) {
            qFatal("AudioCorrection::process_queue - Audio Section is not complete");
        }

        for (int index = 0; index < 98; index++) {
            QVector<int16_t> audio_in_data = audio_section_in.get_frame(index).get_data();
            QVector<int16_t> audio_in_error_data = audio_section_in.get_frame(index).get_error_data();

            // Does the frame contain any errors?
            if (audio_section_in.get_frame(index).count_errors() != 0) {
                // There are errors in the frame
                if (show_debug) {
                    qDebug().nospace().noquote() << "AudioCorrection::process_queue(): Frame " << index << " in section with absolute time " << audio_section_in.metadata.get_absolute_section_time().to_string() << " contains errors";
                }

                for (int sidx = 0; sidx < 12; sidx++) {
                    // Check if the sample is valid
                    if (audio_in_error_data[sidx] == 1) {
                        // Determine the preceding sample value
                        int16_t preceding_sample = 0;
                        int16_t preceding_error = 0;
                        if (sidx > 1) {
                            preceding_sample = audio_in_data[sidx - 2];
                            preceding_error = audio_in_error_data[sidx - 2];
                        } else {
                            if (sidx % 2 == 0) {
                                preceding_sample = last_section_left_sample;
                                preceding_error = last_section_left_error;
                            } else {
                                preceding_sample = last_section_right_sample;
                                preceding_error = last_section_right_error;
                            }
                        }

                        // Determine the following sample value
                        int16_t following_sample = 0;
                        int16_t following_error = 0;
                        if (sidx < 10) {
                            following_sample = audio_in_data[sidx + 2];
                            following_error = audio_in_error_data[sidx + 2];
                        } else {
                            if (index < 96) {
                                following_sample = audio_section_in.get_frame(index + 2).get_data()[0];
                                following_error = audio_section_in.get_frame(index + 2).get_error_data()[0];
                            } else {
                                // We are at the end of the section, use the preceding sample
                                following_sample = preceding_error;
                                following_error = preceding_error;
                            }
                        }

                        // Do we have valid preceding and following samples?
                        if (preceding_error == 0 && following_error == 0) {
                            // We have valid preceding and following samples
                            audio_in_data[sidx] = (preceding_sample + following_sample) / 2;
                            if (show_debug) {
                                qDebug().nospace() << "AudioCorrection::process_queue(): Concealing sample " << sidx << " in frame " << index << " with preceding sample " << preceding_sample << " and following sample " << following_sample <<
                                    " by replacing with average " << audio_in_data[sidx];
                            }
                            concealed_samples_count++;
                        } else {
                            // We don't have valid preceding and following samples
                            if (show_debug) {
                                qDebug().nospace() << "AudioCorrection::process_queue(): Silencing sample " << sidx << " in frame " << index << " as preceding/following samples are invalid";
                            } 
                            audio_in_data[sidx] = 0;
                            silenced_samples_count++;
                        }
                    } else {
                        // All frame samples are valid
                        valid_samples_count++;
                    }
                }
            } else {
                // All frame samples are valid
                valid_samples_count += 12;
            }

            // Put the resulting data into a Audio frame and push it to the output buffer
            Audio audio;
            audio.set_data(audio_in_data);
            audio.set_error_data(audio_in_error_data);

            audio_section_out.push_frame(audio);
        }

        // Save the last sample of the section in case it's needed for the next section
        last_section_left_sample = audio_section_out.get_frame(97).get_data()[10];
        last_section_right_sample = audio_section_out.get_frame(97).get_data()[11];
        last_section_left_error = audio_section_out.get_frame(97).get_error_data()[10];
        last_section_right_error = audio_section_out.get_frame(97).get_error_data()[11];

        // Add the section to the output buffer
        output_buffer.enqueue(audio_section_out);
    }
}

void AudioCorrection::show_statistics() {
    qInfo() << "Audio correction statistics:";   
    qInfo().nospace() << "  Total samples: " << valid_samples_count + concealed_samples_count + silenced_samples_count;
    qInfo().nospace() << "  Valid samples: " << valid_samples_count;
    qInfo().nospace() << "  Concealed samples: " << concealed_samples_count;
    qInfo().nospace() << "  Silenced samples: " << silenced_samples_count;
}