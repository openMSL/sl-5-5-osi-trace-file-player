//
// Copyright 2018 PMSF IT Consulting - Pierre R. Mai
// Copyright 2022 Persival GmbH
// SPDX-License-Identifier: MPL-2.0
//

#include "OSMPTraceFilePlayerConfig.h"
#include <experimental/filesystem>

using namespace std;
namespace fs = std::experimental::filesystem;

#ifndef FMU_SHARED_OBJECT
#define FMI2_FUNCTION_PREFIX OSMPTraceFilePlayer_
#endif
#include "fmi2Functions.h"

/*
 * Logging Control
 *
 * Logging is controlled via three definitions:
 *
 * - If PRIVATE_LOG_PATH_TRACE_FILE_PLAYER is defined it gives the name of a file
 *   that is to be used as a private log file.
 * - If PUBLIC_LOGGING is defined then we will (also) log to
 *   the FMI logging facility where appropriate.
 * - If VERBOSE_FMI_LOGGING_TRACE_FILE_PLAYER is defined then logging of basic
 *   FMI calls is enabled, which can get very verbose.
 */

/*
 * Variable Definitions
 *
 * Define FMI_*_LAST_IDX to the zero-based index of the last variable
 * of the given type (0 if no variables of the type exist).  This
 * ensures proper space allocation, initialisation and handling of
 * the given variables in the template code.  Optionally you can
 * define FMI_TYPENAME_VARNAME_IDX definitions (e.g. FMI_REAL_MYVAR_IDX)
 * to refer to individual variables inside your code, or for example
 * FMI_REAL_MYARRAY_OFFSET and FMI_REAL_MYARRAY_SIZE definitions for
 * array variables.
 */

/* Boolean Variables */
#define FMI_BOOLEAN_VALID_IDX 0
#define FMI_BOOLEAN_LAST_IDX FMI_BOOLEAN_VALID_IDX
#define FMI_BOOLEAN_VARS (FMI_BOOLEAN_LAST_IDX+1)

/* Integer Variables */
#define FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX 0
#define FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX 1
#define FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX 2
#define FMI_INTEGER_COUNT_IDX 3
#define FMI_INTEGER_LAST_IDX FMI_INTEGER_COUNT_IDX
#define FMI_INTEGER_VARS (FMI_INTEGER_LAST_IDX+1)

/* Real Variables */
#define FMI_REAL_LAST_IDX 0
#define FMI_REAL_VARS (FMI_REAL_LAST_IDX+1)

/* String Variables */
#define FMI_STRING_TRACE_PATH_IDX 0
#define FMI_STRING_LAST_IDX FMI_STRING_TRACE_PATH_IDX
#define FMI_STRING_VARS (FMI_STRING_LAST_IDX+1)

#include <iostream>
#include <fstream>
#include <string>
#include <cstdarg>
#include <set>
#include <utility>

#undef min
#undef max
#include "osi_sensorview.pb.h"
#include "osi_sensordata.pb.h"

/* FMU Class */
class COSMPTraceFilePlayer {
public:
    /* FMI2 Interface mapped to C++ */
    COSMPTraceFilePlayer(fmi2String theinstanceName, fmi2Type thefmuType, fmi2String thefmuGUID, fmi2String thefmuResourceLocation, const fmi2CallbackFunctions* thefunctions, fmi2Boolean thevisible, fmi2Boolean theloggingOn);
    ~COSMPTraceFilePlayer();
    fmi2Status SetDebugLogging(fmi2Boolean theloggingOn,size_t nCategories, const fmi2String categories[]);
    static fmi2Component Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn);
    fmi2Status SetupExperiment(fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime);
    fmi2Status EnterInitializationMode();
    fmi2Status ExitInitializationMode();
    fmi2Status DoStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPointfmi2Component);
    fmi2Status Terminate();
    fmi2Status Reset();
    void FreeInstance();
    fmi2Status GetReal(const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]);
    fmi2Status GetInteger(const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]);
    fmi2Status GetBoolean(const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]);
    fmi2Status GetString(const fmi2ValueReference vr[], size_t nvr, fmi2String value[]);
    fmi2Status SetReal(const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]);
    fmi2Status SetInteger(const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]);
    fmi2Status SetBoolean(const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]);
    fmi2Status SetString(const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]);

protected:
    /* Internal Implementation */
    fmi2Status doInit();
    fmi2Status doStart(fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime);
    fmi2Status doEnterInitializationMode();
    fmi2Status doExitInitializationMode();
    fmi2Status doCalc(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPointfmi2Component);
    fmi2Status doTerm();
    void doFree();

protected:
    /* Private File-based Logging just for Debugging */
#ifdef PRIVATE_LOG_PATH_TRACE_FILE_PLAYER
    static ofstream private_log_file;
#endif

    static void fmi_verbose_log_global(const char* format, ...) {
#ifdef VERBOSE_FMI_LOGGING_TRACE_FILE_PLAYER
#ifdef PRIVATE_LOG_PATH_TRACE_FILE_PLAYER
        va_list ap;
        va_start(ap, format);
        char buffer[1024];
        if (!private_log_file.is_open())
            private_log_file.open(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER, ios::out | ios::app);
        if (private_log_file.is_open()) {
#ifdef _WIN32
            vsnprintf_s(buffer, 1024, format, ap);
#else
            vsnprintf(buffer, 1024, format, ap);
#endif
            private_log_file << "OSMPBinarySource" << "::Global:FMI: " << buffer << endl;
            private_log_file.flush();
        }
#endif
#endif
    }

    void internal_log(const char* category, const char* format, va_list arg)
    {
#if defined(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER) || defined(PUBLIC_LOGGING_TRACE_FILE_PLAYER)
        char buffer[1024];
#ifdef _WIN32
        vsnprintf_s(buffer, 1024, format, arg);
#else
        vsnprintf(buffer, 1024, format, arg);
        std::cout << "OSMPTraceFilePlayer" << "::" << instanceName << "<" << ((void*)this) << ">:" << category << ": " << buffer << std::endl;
#endif
#ifdef PRIVATE_LOG_PATH_TRACE_FILE_PLAYER
        if (!private_log_file.is_open())
            private_log_file.open(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER, ios::out | ios::app);
        if (private_log_file.is_open()) {
            private_log_file << "OSMPBinarySource" << "::" << instanceName << "<" << ((void*)this) << ">:" << category << ": " << buffer << endl;
            private_log_file.flush();
        }
#endif
#ifdef PUBLIC_LOGGING_TRACE_FILE_PLAYER
        if (loggingOn && loggingCategories.count(category))
            functions.logger(functions.componentEnvironment,instanceName.c_str(),fmi2OK,category,buffer);
#endif
#endif
    }

    void fmi_verbose_log(const char* format, ...) {
#if  defined(VERBOSE_FMI_LOGGING_TRACE_FILE_PLAYER) && (defined(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER) || defined(PUBLIC_LOGGING_TRACE_FILE_PLAYER))
        va_list ap;
        va_start(ap, format);
        internal_log("FMI",format,ap);
        va_end(ap);
#endif
    }

    /* Normal Logging */
    void normal_log(const char* category, const char* format, ...) {
#if defined(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER) || defined(PUBLIC_LOGGING_TRACE_FILE_PLAYER)
        va_list ap;
        va_start(ap, format);
        internal_log(category,format,ap);
        va_end(ap);
#endif
    }

protected:
    /* Members */
    string instanceName;
    fmi2Type fmuType;
    string fmuGUID;
    string fmuResourceLocation;
    bool visible;
    bool loggingOn;
    set<string> loggingCategories;
    fmi2CallbackFunctions functions;
    fmi2Boolean boolean_vars[FMI_BOOLEAN_VARS];
    fmi2Integer integer_vars[FMI_INTEGER_VARS];
    fmi2Real real_vars[FMI_REAL_VARS];
    string string_vars[FMI_STRING_VARS];
    string* currentBuffer;
    string* lastBuffer;
    long totalLength = 0;
    long playedFrames = 0;
    struct file_extension_is
    {
        std::string ext;
        explicit file_extension_is(std::string  ext): ext(std::move(ext)) {}
        bool operator()(fs::directory_entry const& entry) const
        {
            return entry.path().extension() == ext;
        }
    };
    int realloc_buffer(char **message_buf, size_t new_size);

    /* Simple Accessors */
    fmi2Boolean fmi_valid() { return boolean_vars[FMI_BOOLEAN_VALID_IDX]; }
    void set_fmi_valid(fmi2Boolean value) { boolean_vars[FMI_BOOLEAN_VALID_IDX]=value; }
    fmi2Integer fmi_count() { return integer_vars[FMI_INTEGER_COUNT_IDX]; }
    void set_fmi_count(fmi2Integer value) { integer_vars[FMI_INTEGER_COUNT_IDX]=value; }
    string fmi_trace_path() { return string_vars[FMI_STRING_TRACE_PATH_IDX]; }
    void set_fmi_trace_path(string value) { string_vars[FMI_STRING_TRACE_PATH_IDX]=value; }

    /* Protocol Buffer Accessors */
    void set_fmi_sensor_view_out(const osi3::SensorView& data);
    void set_fmi_sensor_data_out(const osi3::SensorData& data);
    void reset_fmi_sensor_view_out();
    void reset_fmi_sensor_data_out();

};
