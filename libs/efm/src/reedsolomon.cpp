/************************************************************************

    reedsolomon.cpp

    EFM-library - Reed-Solomon CIRC functions
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

#include <QDebug>
#include "ezpwd/rs_base"
#include "ezpwd/rs"
#include "reedsolomon.h"

// ezpwd C1 ECMA-130 CIRC configuration
template < size_t SYMBOLS, size_t PAYLOAD > struct C1RS;
template < size_t PAYLOAD > struct C1RS<255, PAYLOAD>
    : public __RS(C1RS, uint8_t, 255, PAYLOAD, 0x11D, 0, 1, false);

C1RS<255, 255-4> c1rs;

// ezpwd C2 ECMA-130 CIRC configuration
template < size_t SYMBOLS, size_t PAYLOAD > struct C2RS;
template < size_t PAYLOAD > struct C2RS<255, PAYLOAD>
    : public __RS(C2RS, uint8_t, 255, PAYLOAD, 0x11D, 0, 1, false);

C2RS<255, 255-4> c2rs;

ReedSolomon::ReedSolomon() {
    // Initialise statistics
    valid_c1s = 0;
    fixed_c1s = 0;
    error_c1s = 0;

    valid_c2s = 0;
    fixed_c2s = 0;
    error_c2s = 0;
}

// Perform a C1 Reed-Solomon encoding operation on the input data
// This is a (32,28) Reed-Solomon encode - 28 bytes in, 32 bytes out
void ReedSolomon::c1_encode(QVector<uint8_t>& input_data) {
    // Ensure input data is 28 bytes long
    if (input_data.size() != 28) {
        qFatal("ReedSolomon::c1_encode - Input data must be 28 bytes long");
    }

    // Check for potential overflow
    if (static_cast<uint64_t>(input_data.size()) > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        qFatal("ReedSolomon::c1_encode - Input data size exceeds maximum allowable size.  Input data size: %d", input_data.size());
    }

    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(input_data.begin(), input_data.end());
    
    c1rs.encode(tmp_data);

    input_data = QVector<uint8_t>(tmp_data.begin(), tmp_data.end());
}

// Perform a C1 Reed-Solomon decoding operation on the input data
// This is a (32,28) Reed-Solomon encode - 32 bytes in, 28 bytes out
void ReedSolomon::c1_decode(QVector<uint8_t>& input_data, QVector<uint8_t>& error_data) {
    // Ensure input data is 32 bytes long
    if (input_data.size() != 32) {
        qFatal("ReedSolomon::c1_decode - Input data must be 32 bytes long");
    }

    // Check for potential overflow
    if (static_cast<uint64_t>(input_data.size()) > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        qFatal("ReedSolomon::c1_decode - Input data size exceeds maximum allowable size.  Input data size: %d", input_data.size());
    }

    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(input_data.begin(), input_data.end());
    std::vector<int> erasures; // 0-based indices of "missing" input bytes i.e. {12, 13, 14, 15}
    std::vector<int> position; // 0-based indices of corrected bytes in the output

    // Decode the data
    int result = -1;
    result = c1rs.decode(tmp_data, erasures, &position);

    // Convert the std::vector back to a QVector and strip the parity bytes
    input_data = QVector<uint8_t>(tmp_data.begin(), tmp_data.end() - 4);
    error_data.resize(input_data.size());

    // If result == 0, then the Reed-Solomon decode was successful
    if (result == 0) {
        //qDebug() << "ReedSolomon::c1_decode - C1 Decoded successfully";
        error_data.fill(0);
        valid_c1s++;
        return;
    }

    // Note1: If we have more than 2 erasures we have to flag the whole output as
    // erasures and copy the original input data to the output (according to Sorin 2.4 p66)
    // This is because the Reed-Solomon decoder can't correct more than 2 erasures unless
    // we already know the erasure positions (which, for C1, we don't).

    // Note2: We return the error data as a vector of 1s and 0s. 1 indicates an error, 0 indicates no error.
    // This is because we need to pass the error data through the same treatment as the actual data 
    // (delay lines and so on) to ensure that the error data is in sync with the actual data when performing
    // C2 correction.

    // If result < 0, then the Reed-Solomon decode completely failed
    // If result > 2, then there were too many errors for the Reed-Solomon decode to fix
    if (result < 0 || result > 2) {
        //qDebug().noquote() << "ReedSolomon::c1_decode - C1 corrupt and could not be fixed";
        // Make every byte in the error data 1 - i.e. all errors
        error_data.fill(1);
        error_c1s++;
        return;
    }

    // If result > 0 && result <= 2, then the Reed-Solomon decode fixed some errors
    // and the data is now correct
    if (result > 0 && result <= 2) {
        QString s_positions;
        for (int pos : position) {
            s_positions += QString::number(pos);
            if (pos != position.back()) {
                s_positions += " and ";
            }
        }
        // if (position.size() == 2) qDebug().noquote() << "ReedSolomon::c1_decode - C1 Fixed" << result << "errors at byte positions" << s_positions.trimmed();
        // else qDebug().noquote() << "ReedSolomon::c1_decode - C1 Fixed" << result << "error at byte position" << s_positions.trimmed();

        error_data.fill(0);
        fixed_c1s++;
        return;
    }

    qDebug() << "ReedSolomon::c1_decode - C1 result unknown - a bug perhaps?";
    return;   
}

// Perform a C2 Reed-Solomon encoding operation on the input data
// This is a (28,24) Reed-Solomon encode - 24 bytes in, 28 bytes out
void ReedSolomon::c2_encode(QVector<uint8_t>& input_data) {
    // Ensure input data is 24 bytes long
    if (input_data.size() != 24) {
        qFatal("ReedSolomon::c2_encode - Input data must be 24 bytes long");
    }

    // Check for potential overflow
    if (static_cast<uint64_t>(input_data.size()) > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        qFatal("ReedSolomon::c2_encode - Input data size exceeds maximum allowable size.  Input data size: %d", input_data.size());
    }

    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data;

    // Copy the first 12 bytes of the input data
    tmp_data.insert(tmp_data.end(), input_data.begin(), input_data.begin() + 12);

    // Insert 4 zeros for the parity bytes
    tmp_data.insert(tmp_data.end(), 4, 0);

    // Copy the last 12 bytes of the input data
    tmp_data.insert(tmp_data.end(), input_data.end() - 12, input_data.end());

    // Mark the parity byte positions as erasures (12-15)
    std::vector<int> erasures = {12, 13, 14, 15};  // 0-based indices of "missing" bytes

    // Decode and correct erasures (i.e. insert the missing parity bytes in the middle)
    c2rs.decode(tmp_data, erasures);

    // Copy the data back to a QVector
    input_data = QVector<uint8_t>(tmp_data.begin(), tmp_data.end());
}

// Perform a C2 Reed-Solomon decoding operation on the input data
// This is a (28,24) Reed-Solomon encode - 28 bytes in, 24 bytes out
void ReedSolomon::c2_decode(QVector<uint8_t>& input_data, QVector<uint8_t>& error_data) {
    // Ensure input data is 28 bytes long
    if (input_data.size() != 28) {
        qFatal("ReedSolomon::c2_decode - Input data must be 28 bytes long");
    }

    if (error_data.size() != 28) {
        qFatal("ReedSolomon::c2_decode - Error data must be 28 bytes long");
    }

    // Check for potential overflow
    if (static_cast<uint64_t>(input_data.size()) > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        qFatal("ReedSolomon::c2_decode - Input data size exceeds maximum allowable size.  Input data size: %d", input_data.size());
    }

    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(input_data.begin(), input_data.end());
    std::vector<int> position;
    std::vector<int> erasures;

    // Convert the error_data into a list of erasure positions
    for (int index = 0; index < error_data.size(); index++) {
        if (error_data[index] == 1) {
            erasures.push_back(index);
        }
    }

    // if (erasures.size() > 0) {
    //     qDebug() << "ReedSolomon::c2_decode - Input data erasures reported at positions:" << erasures;
    // }

    // Since we know the erasure positions, we can correct a maximum of 4 errors.  If the number
    // of know input erasures is greater than 4, then we can't correct the data.
    if (erasures.size() > 4) {
        // If there are more than 4 erasures, then we can't correct the data - however, since the C1 marks all bytes as erasures if it fails,
        // there is a chance the C2 only has 1 or 2 errors... so it's worth trying to fix the data without erasures
        //qDebug().nospace() << "ReedSolomon::c2_decode - Too many erasures to correct (" << erasures.size() << ") - Attempting to fix without erasures";
        erasures.clear();
    }

    // Decode the data
    int result = -1;
    result = c2rs.decode(tmp_data, erasures, &position);

    // Convert the std::vector back to a QVector and remove the parity bytes
    // by copying bytes 0-11 and 16-27 to the output data
    input_data = QVector<uint8_t>(tmp_data.begin(), tmp_data.begin() + 12) + QVector<uint8_t>(tmp_data.begin() + 16, tmp_data.end());
    error_data.resize(input_data.size());

    // If result == 0, then the Reed-Solomon decode was successful
    if (result == 0) {
        //qDebug() << "ReedSolomon::c2_decode - C2 Decoded successfully";
        error_data.fill(0);
        valid_c2s++;
        return;
    }

    // If result < 0, then the Reed-Solomon decode completely failed
    // If result > 4, then there were too many errors for the Reed-Solomon decode to fix
    if (result < 0 || result > 4) {
        //qDebug().noquote() << "ReedSolomon::c2_decode - C2 corrupt and could not be fixed: result" << result;
        // Make every byte in the error data 1 - i.e. all errors
        error_data.fill(1);
        error_c2s++;
        return;
    }

    // If result > 0 && result <= 4, then the Reed-Solomon decode fixed some errors
    // and the data is now correct
    if (result > 0 && result <= 4) {
        QString s_positions;
        for (int pos : position) {
            s_positions += QString::number(pos);
            if (pos != position.back()) {
                s_positions += " and ";
            } else {
                s_positions += ", ";
            }
        }
        // if (position.size() != 1) qDebug().noquote() << "ReedSolomon::c2_decode - C2 Fixed" << result << "errors at byte positions" << s_positions.trimmed();
        // else qDebug().noquote() << "ReedSolomon::c2_decode - C2 Fixed" << result << "error at byte position" << s_positions.trimmed();

        error_data.fill(0);
        fixed_c2s++;
        return;
    }

    qDebug() << "ReedSolomon::c2_decode - C2 result unknown - a bug perhaps?";
    return;   
}

// Getter functions for the statistics
int32_t ReedSolomon::get_valid_c1s() {
    return valid_c1s;
}

int32_t ReedSolomon::get_fixed_c1s() {
    return fixed_c1s;
}

int32_t ReedSolomon::get_error_c1s() {
    return error_c1s;
}

int32_t ReedSolomon::get_valid_c2s() {
    return valid_c2s;
}

int32_t ReedSolomon::get_fixed_c2s() {
    return fixed_c2s;
}

int32_t ReedSolomon::get_error_c2s() {
    return error_c2s;
}