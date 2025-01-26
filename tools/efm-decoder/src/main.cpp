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

    QCoreApplication a(argc, argv);

    // Set application name and version
    QCoreApplication::setApplicationName("ld-efm-decoder");
    QCoreApplication::setApplicationVersion(QString("Branch: %1 / Commit: %2").arg(APP_BRANCH, APP_COMMIT));
    QCoreApplication::setOrganizationDomain("domesday86.com");

    // Set up the command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "ld-efm-decoder - EFM data decoder\n"
        "\n"
        "(c)2025 Simon Inns\n"
        "GPLv3 Open-Source - github: https://github.com/happycube/ld-decode");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add the standard debug options --debug and --quiet
    addStandardDebugOptions(parser);

    // Option to specify output data file type
    QCommandLineOption outputTypeOption("wav-output", QCoreApplication::translate("main", "Add wav header to output data"));
    parser.addOption(outputTypeOption);

    // Add custom options for showing frame data
    QCommandLineOption showOutputOption("show-output", QCoreApplication::translate("main", "Show output data"));
    QCommandLineOption showF1Option("show-f1", QCoreApplication::translate("main", "Show frame data at level F1"));
    QCommandLineOption showF2Option("show-f2", QCoreApplication::translate("main", "Show frame data at level F2"));
    QCommandLineOption showF3Option("show-f3", QCoreApplication::translate("main", "Show frame data at level F3"));
    parser.addOption(showOutputOption);
    parser.addOption(showF1Option);
    parser.addOption(showF2Option);
    parser.addOption(showF3Option);

    // -- Positional arguments --
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "Specify input EFM file"));
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "Specify output data file"));

    // Process the command line options and arguments given by the user
    parser.process(a);

    // Standard logging options
    processStandardDebugOptions(parser);

    // Check for custom options
    bool showOutput = parser.isSet(showOutputOption);
    bool showF1 = parser.isSet(showF1Option);
    bool showF2 = parser.isSet(showF2Option);
    bool showF3 = parser.isSet(showF3Option);

    // Get the filename arguments from the parser
    QString input_filename;
    QString output_filename;
    QStringList positional_arguments = parser.positionalArguments();
    
    if (positional_arguments.count() != 2) {
        qWarning() << "You must specify the input EFM filename and the output data filename";
        return 1;
    }
    input_filename = positional_arguments.at(0);
    output_filename = positional_arguments.at(1);

    // Check for output data type options
    bool wav_output = parser.isSet(outputTypeOption);

    // Perform the processing
    qInfo() << "Beginning EFM decoding of" << input_filename;
    EfmProcessor efm_processor;

    efm_processor.set_show_data(showOutput, showF1, showF2, showF3);
    efm_processor.set_output_type(wav_output);

    if (!efm_processor.process(input_filename, output_filename)) {
        return 1;
    }

    // Quit with success
    return 0;
}