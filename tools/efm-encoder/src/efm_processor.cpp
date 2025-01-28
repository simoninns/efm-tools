/************************************************************************

    efm_processor.cpp

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

#include <QDebug>
#include <QString>
#include <QFile>

#include "efm_processor.h"
#include "encoders.h"
#include "enc_data24tof1frame.h"
#include "enc_f1frametof2frame.h"
#include "enc_f2frametosection.h"
#include "enc_sectiontof3frame.h"
#include "enc_f3frametochannel.h"

EfmProcessor::EfmProcessor() {}

bool EfmProcessor::process(QString input_filename, QString output_filename) {
    qInfo() << "Encoding EFM data from file:" << input_filename << "to file:" << output_filename;

    // Open the input file
    QFile input_file(input_filename);

    if (!input_file.open(QIODevice::ReadOnly)) {
        qDebug() << "EfmProcessor::process(): Failed to open input file:" << input_filename;
        return false;
    }

    // Calculate the total size of the input file for progress reporting
    qint64 total_size = input_file.size();
    qint64 processed_size = 0;
    int last_reported_progress = 0;

    if (is_input_data_wav) {
        // Input data is a WAV file, so we need to verify the WAV file format
        // by reading the WAV header
        // Read the WAV header
        QByteArray header = input_file.read(44);
        if (header.size() != 44) {
            qWarning() << "Failed to read WAV header from input file:" << input_filename;
            return false;
        }

        // Check the WAV header for the correct format
        if (header.mid(0, 4) != "RIFF" || header.mid(8, 4) != "WAVE") {
            qWarning() << "Invalid WAV file format:" << input_filename;
            return false;
        }

        // Check the sampling rate (44.1KHz)
        uint32_t sampleRate = *reinterpret_cast<const quint32*>(header.mid(24, 4).constData());
        if (sampleRate != 44100) {
            qWarning() << "Unsupported sample rate:" << sampleRate << "in file:" << input_filename;
            return false;
        }

        // Check the bit depth (16 bits)
        uint16_t bitDepth = *reinterpret_cast<const quint16*>(header.mid(34, 2).constData());
        if (bitDepth != 16) {
            qWarning() << "Unsupported bit depth:" << bitDepth << "in file:" << input_filename;
            return false;
        }

        // Check the number of channels (stereo)
        uint16_t numChannels = *reinterpret_cast<const quint16*>(header.mid(22, 2).constData());
        if (numChannels != 2) {
            qWarning() << "Unsupported number of channels:" << numChannels << "in file:" << input_filename;
            return false;
        }
    }

    // Prepare the output file
    QFile output_file(output_filename);

    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "EfmProcessor::process(): Failed to open output file:" << output_filename;
        return false;
    }

    // Check for the pad start corruption option
    if (corrupt_start) {
        qInfo() << "Corrupting output: Padding start of file with" << corrupt_start_symbols << "t-value symbols";
        // Pad the start of the file with the specified number of symbols
        srand(static_cast<unsigned int>(time(nullptr))); // Seed the random number generator
        for (uint32_t i = 0; i < corrupt_start_symbols; i++) {
            // Pick a random value between 3 and 11
            uint8_t value = (rand() % 9) + 3;
            char val = static_cast<char>(value);
            output_file.write(&val, sizeof(val));
        }
    }

    // Check for the corrupt t-values option
    if (corrupt_tvalues) {
        if (corrupt_tvalues_frequency < 2) {
            qWarning() << "Corrupting output: Corrupt t-values frequency must be at least 2";
            return false;
        }
        qInfo() << "Corrupting output: Corrupting t-values with a frequency of" << corrupt_tvalues_frequency;
    }

    // Prepare the encoders
    Data24ToF1Frame data24_to_f1;
    F1FrameToF2Frame f1_frame_to_f2;
    F2FrameToSection f2_frame_to_section;
    SectionToF3Frame section_to_f3;
    F3FrameToChannel f3_frame_to_channel;

    // Apply encoder options
    f2_frame_to_section.set_qmode_options(qmode, qcontrol);
    f3_frame_to_channel.set_corruption(corrupt_f3sync, corrupt_f3sync_frequency, corrupt_subcode_sync, corrupt_subcode_sync_frequency);

    // Channel data counter
    uint32_t channel_byte_count = 0;

    // Process the input audio data 24 bytes at a time
    // The time, type and track number are set to default values for now
    uint8_t track_number = 1;
    FrameType frame_type = FrameType::USER_DATA;
    FrameTime frame_time;
    uint32_t data24_count = 0;

    QVector<uint8_t> input_data(24);
    uint64_t bytes_read = input_file.read(reinterpret_cast<char*>(input_data.data()), 24);

    while (bytes_read > 0) {
        // Create a Data24 object and set the data
        Data24 data24;
        data24.set_data(input_data);
        data24.set_frame_type(frame_type);
        data24.set_frame_time(frame_time);
        data24.set_track_number(track_number);
        if (showInput) data24.show_data();

        // Push the data to the first converter
        data24_to_f1.push_frame(data24);
        data24_count++;

        // Adjust the frame time if required
        if (data24_count % 98 == 0) {
            frame_time.increment_frame();
        }

        // Are there any F1 frames ready?
        if (data24_to_f1.is_ready()) {
            // Pop the F1 frame, count it and push it to the next converter
            F1Frame f1_frame = data24_to_f1.pop_frame();
            if (showF1) f1_frame.show_data();
            f1_frame_to_f2.push_frame(f1_frame);
        }

        // Are there any F2 frames ready?
        if (f1_frame_to_f2.is_ready()) {
            // Pop the F2 frame, count it and push it to the next converter
            F2Frame f2_frame = f1_frame_to_f2.pop_frame();
            if (showF2) f2_frame.show_data();
            f2_frame_to_section.push_frame(f2_frame);
        }

        // Are there any sections ready?
        if (f2_frame_to_section.is_ready()) {
            // Pop the section, count it and push it to the next converter
            Section section = f2_frame_to_section.pop_section();
            section_to_f3.push_section(section);
        }

        // Are there any F3 frames ready?
        if (section_to_f3.is_ready()) {
            // Pop the section, count it and push it to the next converter
            QVector<F3Frame> f3_frames = section_to_f3.pop_frames();

            for (int i = 0; i < f3_frames.size(); i++) {
                if (showF3) f3_frames[i].show_data();
                f3_frame_to_channel.push_frame(f3_frames[i]);
            }
        }

        // Is there any channel data ready?
        if (f3_frame_to_channel.is_ready()) {
            // Pop the channel data, count it and write it to the output file
            QVector<uint8_t> channel_data = f3_frame_to_channel.pop_frame();

            // Check for the corrupt t-values option
            if (corrupt_tvalues) {
                // We have to take the channel_byte_count and the size of channel data
                // and figure out if we need to corrupt the t-values in channel_data based
                // on the corrupt_tvalues_frequency
                //
                // Any t-values that should be corrupted will be set to a random
                // value between 3 and 11
                for (int j = 0; j < channel_data.size(); j++) {
                    if ((channel_byte_count + j) % corrupt_tvalues_frequency == 0) {
                        uint8_t new_value;
                        // Make sure the new value is different from the current value
                        do {
                            new_value = (rand() % 9) + 3;
                        } while (new_value == channel_data[j]);
                        channel_data[j] = new_value;
                        //qDebug() << "Corrupting t-value at" << (channel_byte_count + j) << "with value" << channel_data[j];
                    }
                }
            }

            channel_byte_count += channel_data.size();

            // Write the channel data to the output file
            output_file.write(reinterpret_cast<const char*>(channel_data.data()), channel_data.size());
        }

        // Update the processed size
        processed_size += bytes_read;

        // Calculate the current progress
        int current_progress = static_cast<int>((processed_size * 100) / total_size);

        // Report progress every 5%
        if (current_progress >= last_reported_progress + 5) {
            qInfo() << "Progress:" << current_progress << "%";
            last_reported_progress = current_progress;
        }

        // Read the next 24 bytes
        bytes_read = input_file.read(reinterpret_cast<char*>(input_data.data()), 24);
    }

    // Close the output file
    output_file.close();

    // Output some statistics
    double total_bytes = data24_count * 24;
    QString size_unit;
    double size_value;

    if (total_bytes < 1024) {
        size_unit = "bytes";
        size_value = total_bytes;
    } else if (total_bytes < 1024 * 1024) {
        size_unit = "Kbytes";
        size_value = total_bytes / 1024;
    } else {
        size_unit = "Mbytes";
        size_value = total_bytes / (1024 * 1024);
    }

    qInfo().noquote() << "Processed" << data24_count << "data24 frames totalling" << size_value << size_unit;
    qInfo().noquote() << "Final time was" << frame_time.to_string();

    qInfo() << data24_to_f1.get_valid_output_frames_count() << "F1 frames," << f1_frame_to_f2.get_valid_output_frames_count() << "F2 frames," <<
        f2_frame_to_section.get_valid_output_frames_count() << "Sections," << section_to_f3.get_valid_output_frames_count() << "F3 frames,";
    qInfo() << f3_frame_to_channel.get_valid_output_frames_count() << "channel frames" << f3_frame_to_channel.get_total_t_values() <<
        "T-values," << channel_byte_count << "channel bytes";

    // Show corruption warnings
    if (corrupt_tvalues) {
        qWarning() << "Corruption applied-> Corrupted t-values with a frequency of" << corrupt_tvalues_frequency;
    }
    if (corrupt_start) {
        qWarning() << "Corruption applied-> Padded start of file with" << corrupt_start_symbols << "random t-value symbols";
    }
    if (corrupt_f3sync) {
        qWarning() << "Corruption applied-> Corrupted F3 Frame 24-bit sync patterns with a frame frequency of" << corrupt_f3sync_frequency;
    }
    if (corrupt_subcode_sync) {
        qWarning() << "Corruption applied-> Corrupted subcode sync0 and sync1 patterns with a section frequency of" << corrupt_subcode_sync_frequency;
    }

    qInfo() << "Encoding complete";

    return true;
}

void EfmProcessor::set_qmode_options(bool _qmode_1, bool _qmode_4, bool _qmode_audio, bool _qmode_data, bool _qmode_copy, bool _qmode_nocopy) {
    if (_qmode_1 && _qmode_4) {
        qFatal("You can only specify one Q-Channel mode with --qmode-1 or --qmode-4");
    }
    if (_qmode_audio && _qmode_data) {
        qFatal("You can only specify one Q-Channel data type with --qmode-audio or --qmode-data");
    }
    if (_qmode_copy && _qmode_nocopy) {
        qFatal("You can only specify one Q-Channel copy type with --qmode-copy or --qmode-nocopy");
    }

    if (_qmode_1) qmode = Qchannel::QModes::QMODE_1;
    else if (_qmode_4) qmode = Qchannel::QModes::QMODE_4;

    if (_qmode_audio && _qmode_copy) qcontrol = Qchannel::Control::AUDIO_2CH_NO_PREEMPHASIS_COPY_PERMITTED;
    else if (_qmode_audio && _qmode_nocopy) qcontrol = Qchannel::Control::AUDIO_2CH_NO_PREEMPHASIS_COPY_PROHIBITED;
    else if (_qmode_data && _qmode_copy) qcontrol = Qchannel::Control::DIGITAL_COPY_PERMITTED;
    else if (_qmode_data && _qmode_nocopy) qcontrol = Qchannel::Control::DIGITAL_COPY_PROHIBITED;
    else {
        qFatal("Invalid Q-Channel control options");
    }
}

void EfmProcessor::set_show_data(bool _showInput, bool _showF1, bool _showF2, bool _showF3) {
    showInput = _showInput;
    showF1 = _showF1;
    showF2 = _showF2;
    showF3 = _showF3;
}

// Set the input data type (true for WAV, false for raw)
void EfmProcessor::set_input_type(bool _wavInput) {
    is_input_data_wav = _wavInput;
}

// Note: The corruption options are to assist in generating test data
// used to verify the decoder.  They are not intended to be used in
// normal operation.
bool EfmProcessor::set_corruption(bool _corrupt_tvalues,
        uint32_t _corrupt_tvalues_frequency, bool _pad_start, uint32_t _pad_start_symbols,
        bool _corrupt_f3sync, uint32_t _corrupt_f3sync_frequency,
        bool _corrupt_subcode_sync, uint32_t _corrupt_subcode_sync_frequency) {

    corrupt_tvalues = _corrupt_tvalues;
    corrupt_tvalues_frequency = _corrupt_tvalues_frequency;

    if (corrupt_tvalues && corrupt_tvalues_frequency < 2) {
        qInfo() << "Corrupting output: Corrupt t-values frequency must be at least 2";
        return false;
    }

    corrupt_start = _pad_start;
    corrupt_start_symbols = _pad_start_symbols;

    if (corrupt_start && corrupt_start_symbols < 1) {
        qInfo() << "Corrupting output: Pad start symbols must be at least 1";
        return false;
    }

    corrupt_f3sync = _corrupt_f3sync;
    corrupt_f3sync_frequency = _corrupt_f3sync_frequency;

    if (corrupt_f3sync && corrupt_f3sync_frequency < 2) {
        qInfo() << "Corrupting output: Corrupt F3 sync frequency must be at least 2";
        return false;
    }

    corrupt_subcode_sync = _corrupt_subcode_sync;
    corrupt_subcode_sync_frequency = _corrupt_subcode_sync_frequency;

    if (corrupt_subcode_sync && corrupt_subcode_sync_frequency < 2) {
        qInfo() << "Corrupting output: Corrupt subcode sync frequency must be at least 2";
        return false;
    }

    return true;
}