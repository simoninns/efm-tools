/************************************************************************

    main.cpp

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

#include <QCoreApplication>
#include <QDebug>
#include <QtGlobal>
#include <QCommandLineParser>
#include <QThread>

#include "logging.h"
#include "efm_processor.h"

int main(int argc, char *argv[])
{
    // Set 'binary mode' for stdin and stdout on windows
    setBinaryMode();
    // Install the local debug message handler
    setDebug(true);
    qInstallMessageHandler(debugOutputHandler);

    QCoreApplication app(argc, argv);

    // Set application name and version
    QCoreApplication::setApplicationName("efm-decoder");
    QCoreApplication::setApplicationVersion(
            QString("Branch: %1 / Commit: %2").arg(APP_BRANCH, APP_COMMIT));
    QCoreApplication::setOrganizationDomain("domesday86.com");

    // Set up the command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
            "efm-decoder - EFM data decoder\n"
            "\n"
            "(c)2025 Simon Inns\n"
            "GPLv3 Open-Source - github: https://github.com/simoninns/efm-tools");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add the standard debug options --debug and --quiet
    addStandardDebugOptions(parser);

    // Group of options for specifying output data file type
    QList<QCommandLineOption> outputTypeOptions = {
        QCommandLineOption("output-wav",
                           QCoreApplication::translate("main", "Output data as a WAV file")),
        QCommandLineOption(
                "output-wav-metadata",
                QCoreApplication::translate("main", "Output data as a WAV file with metadata")),
        QCommandLineOption(
                "no-wav-correction",
                QCoreApplication::translate("main", "Do not correct the output WAV data")),
        QCommandLineOption("output-data",
                QCoreApplication::translate("main", "Output ECMA-130 decoded data")),
    };
    parser.addOptions(outputTypeOptions);

    // Group of options for showing frame data
    QList<QCommandLineOption> displayFrameDataOptions = {
        QCommandLineOption("show-f3", QCoreApplication::translate("main", "Show F3 frame data")),
        QCommandLineOption("show-f2", QCoreApplication::translate("main", "Show F2 frame data")),
        QCommandLineOption("show-f1", QCoreApplication::translate("main", "Show F1 frame data")),
        QCommandLineOption("show-data24",
                           QCoreApplication::translate("main", "Show Data24 frame data")),
        QCommandLineOption("show-audio",
                           QCoreApplication::translate("main", "Show Audio frame data")),
        QCommandLineOption("show-rawsector",
                           QCoreApplication::translate("main", "Show Raw Sector frame data")),
    };
    parser.addOptions(displayFrameDataOptions);

    // Group of options for advanced debugging
    QList<QCommandLineOption> advancedDebugOptions = {
        QCommandLineOption(
                "show-tvalues-debug",
                QCoreApplication::translate("main", "Show T-values to channel decoding debug")),
        QCommandLineOption(
                "show-channel-debug",
                QCoreApplication::translate("main", "Show channel to F3 decoding debug")),
        QCommandLineOption(
                "show-f3-debug",
                QCoreApplication::translate("main", "Show F3 to F2 section decoding debug")),
        QCommandLineOption("show-f2-correct-debug",
                           QCoreApplication::translate("main", "Show F2 section correction debug")),
        QCommandLineOption("show-f2-debug",
                           QCoreApplication::translate("main", "Show F2 to F1 decoding debug")),
        QCommandLineOption("show-f1-debug",
                           QCoreApplication::translate("main", "Show F1 to Data24 decoding debug")),
        QCommandLineOption(
                "show-audio-debug",
                QCoreApplication::translate("main", "Show Data24 to audio decoding debug")),
        QCommandLineOption("show-audio-correction-debug",
                           QCoreApplication::translate("main", "Show Audio correction debug")),
        QCommandLineOption("show-all-iec-debug",
                           QCoreApplication::translate("main", "Show all IEC spec (Audio-CD) decoding debug")),
        QCommandLineOption("show-rawsector-debug",
                QCoreApplication::translate("main", "Show Data24 to raw sector decoding debug")),
        QCommandLineOption("show-sector-debug",
                QCoreApplication::translate("main", "Show raw sector to sector decoding debug")),
        QCommandLineOption("show-all-ecma-debug",
                QCoreApplication::translate("main", "Show all ECMA spec (CD-ROM) decoding debug")),
    };
    parser.addOptions(advancedDebugOptions);

    // -- Positional arguments --
    parser.addPositionalArgument("input",
                                 QCoreApplication::translate("main", "Specify input EFM file"));
    parser.addPositionalArgument("output",
                                 QCoreApplication::translate("main", "Specify output data file"));

    // Process the command line options and arguments given by the user
    parser.process(app);

    // Standard logging options
    processStandardDebugOptions(parser);

    // Check for output data type options
    bool outputWav = parser.isSet("output-wav");
    bool outputWavMetadata = parser.isSet("output-wav-metadata");
    bool noWavCorrection = parser.isSet("no-wav-correction");
    bool outputData = parser.isSet("output-data");

    // Check for frame data options
    bool showF1 = parser.isSet("show-f1");
    bool showF2 = parser.isSet("show-f2");
    bool showF3 = parser.isSet("show-f3");
    bool showData24 = parser.isSet("show-data24");
    bool showAudio = parser.isSet("show-audio");
    bool showRawSector = parser.isSet("show-rawsector");

    // Check for advanced debug options
    bool showTValuesDebug = parser.isSet("show-tvalues-debug");
    bool showChannelDebug = parser.isSet("show-channel-debug");
    bool showF3Debug = parser.isSet("show-f3-debug");
    bool showF2CorrectDebug = parser.isSet("show-f2-correct-debug");
    bool showF2Debug = parser.isSet("show-f2-debug");
    bool showF1Debug = parser.isSet("show-f1-debug");
    bool showAudioDebug = parser.isSet("show-audio-debug");
    bool showAudioCorrectionDebug = parser.isSet("show-audio-correction-debug");
    bool showAllIecDebug = parser.isSet("show-all-iec-debug");
    bool showRawSectorDebug = parser.isSet("show-rawsector-debug");
    bool showSectorDebug = parser.isSet("show-sector-debug");
    bool showAllEcmaDebug = parser.isSet("show-all-ecma-debug");

    if (showAllIecDebug) {
        showTValuesDebug = true;
        showChannelDebug = true;
        showF3Debug = true;
        showF2CorrectDebug = true;
        showF2Debug = true;
        showF1Debug = true;
        showAudioDebug = true;
        showAudioCorrectionDebug = true;
    }

    if (showAllEcmaDebug) {
        showRawSectorDebug = true;
        showSectorDebug = true;
    }

    // Get the filename arguments from the parser
    QString inputFilename;
    QString outputFilename;
    QStringList positionalArguments = parser.positionalArguments();

    if (positionalArguments.count() != 2) {
        qWarning() << "You must specify the input EFM filename and the output data filename";
        return 1;
    }
    inputFilename = positionalArguments.at(0);
    outputFilename = positionalArguments.at(1);

    // Perform the processing
    qInfo() << "Beginning EFM decoding of" << inputFilename;
    EfmProcessor efmProcessor;

    efmProcessor.setShowData(showRawSector, showAudio, showData24, showF1, showF2, showF3);
    efmProcessor.setOutputType(outputWav, outputWavMetadata, noWavCorrection, outputData);
    efmProcessor.setDebug(showTValuesDebug, showChannelDebug, showF3Debug, showF2CorrectDebug,
                          showF2Debug, showF1Debug, showAudioDebug, showAudioCorrectionDebug,
                          showRawSectorDebug, showSectorDebug);

    if (!efmProcessor.process(inputFilename, outputFilename)) {
        return 1;
    }

    // Quit with success
    return 0;
}