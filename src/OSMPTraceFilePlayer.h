//
// Copyright 2018 PMSF IT Consulting - Pierre R. Mai
// Copyright 2022 Persival GmbH
// SPDX-License-Identifier: MPL-2.0
//
#ifndef OSMPTraceFilePlayer_H_
#define OSMPTraceFilePlayer_H_
#include <filesystem>

using namespace std;

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
#define FMI_BOOLEAN_VARS (FMI_BOOLEAN_LAST_IDX + 1)

/* Integer Variables */
#define FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX 0
#define FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX 1
#define FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX 2
#define FMI_INTEGER_COUNT_IDX 3
#define FMI_INTEGER_LAST_IDX FMI_INTEGER_COUNT_IDX
#define FMI_INTEGER_VARS (FMI_INTEGER_LAST_IDX + 1)

/* Real Variables */
#define FMI_REAL_LAST_IDX 0
#define FMI_REAL_VARS (FMI_REAL_LAST_IDX + 1)

/* String Variables */
#define FMI_STRING_TRACE_PATH_IDX 0
#define FMI_STRING_TRACE_NAME_IDX 1
#define FMI_STRING_LAST_IDX FMI_STRING_TRACE_NAME_IDX
#define FMI_STRING_VARS (FMI_STRING_LAST_IDX + 1)

#include <cstdarg>
#include <set>
#include <string>

#undef min
#undef max
#include "osi-utilities/tracefile/Reader.h"
#include "osi_sensordata.pb.h"
#include "osi_sensorview.pb.h"

/* FMU Class */
class COSMPTraceFilePlayer
{
  public:
    /* FMI2 Interface mapped to C++ */
    COSMPTraceFilePlayer(fmi2String theinstance_name,
                         fmi2Type thefmu_type,
                         fmi2String thefmu_guid,
                         fmi2String thefmu_resource_location,
                         const fmi2CallbackFunctions* thefunctions,
                         fmi2Boolean thevisible,
                         fmi2Boolean thelogging_on);
    ~COSMPTraceFilePlayer();
    fmi2Status SetDebugLogging(fmi2Boolean thelogging_on, size_t n_categories, const fmi2String categories[]);
    static fmi2Component Instantiate(fmi2String instance_name,
                                     fmi2Type fmu_type,
                                     fmi2String fmu_guid,
                                     fmi2String fmu_resource_location,
                                     const fmi2CallbackFunctions* functions,
                                     fmi2Boolean visible,
                                     fmi2Boolean logging_on);
    fmi2Status SetupExperiment(fmi2Boolean tolerance_defined, fmi2Real tolerance, fmi2Real start_time, fmi2Boolean stop_time_defined, fmi2Real stop_time);
    fmi2Status EnterInitializationMode();
    fmi2Status ExitInitializationMode();
    fmi2Status DoStep(fmi2Real current_communication_point, fmi2Real communication_step_size, fmi2Boolean no_set_fmu_state_prior_to_current_pointfmi_2_component);
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
    fmi2Status GetBooleanStatus(fmi2StatusKind s, fmi2Boolean* value) const;

  protected:
    /* Internal Implementation */
    fmi2Status DoInit();
    static fmi2Status DoStart(fmi2Boolean tolerance_defined, fmi2Real tolerance, fmi2Real start_time, fmi2Boolean stop_time_defined, fmi2Real stop_time);
    static fmi2Status DoEnterInitializationMode();
    fmi2Status DoExitInitializationMode();
    fmi2Status DoCalc(fmi2Real current_communication_point, fmi2Real communication_step_size, fmi2Boolean no_set_fmu_state_prior_to_current_point);
    static fmi2Status DoTerm();
    static void DoFree();

    /* Private File-based Logging just for Debugging */
#ifdef PRIVATE_LOG_PATH_TRACE_FILE_PLAYER
    static ofstream private_log_file;
#endif

    static void fmi_verbose_log_global(const char* format, ...)
    {
#ifdef VERBOSE_FMI_LOGGING_TRACE_FILE_PLAYER
#ifdef PRIVATE_LOG_PATH_TRACE_FILE_PLAYER
        va_list ap;
        va_start(ap, format);
        char buffer[1024];
        if (!private_log_file.is_open())
            private_log_file.open(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER, ios::out | ios::app);
        if (private_log_file.is_open())
        {
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

    void InternalLog(const char* category, const char* format, va_list arg)
    {
#if defined(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER) || defined(PUBLIC_LOGGING_TRACE_FILE_PLAYER)
        char buffer[1024];
#ifdef _WIN32
        vsnprintf_s(buffer, 1024, format, arg);
#else
        vsnprintf(buffer, 1024, format, arg);
        std::cout << "OSMPTraceFilePlayer" << "::" << instance_name_ << "<" << ((void*)this) << ">:" << category << ": " << buffer << std::endl;
#endif
#ifdef PRIVATE_LOG_PATH_TRACE_FILE_PLAYER
        if (!private_log_file.is_open())
            private_log_file.open(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER, ios::out | ios::app);
        if (private_log_file.is_open())
        {
            private_log_file << "OSMPBinarySource" << "::" << instance_name_ << "<" << ((void*)this) << ">:" << category << ": " << buffer << endl;
            private_log_file.flush();
        }
#endif
#ifdef PUBLIC_LOGGING_TRACE_FILE_PLAYER
        if (logging_on_ && logging_categories_.count(category))
            functions_.logger(functions_.componentEnvironment, instance_name_.c_str(), fmi2OK, category, buffer);
#endif
#endif
    }

    void FmiVerboseLog(const char* format, ...)
    {
#if defined(VERBOSE_FMI_LOGGING_TRACE_FILE_PLAYER) && (defined(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER) || defined(PUBLIC_LOGGING_TRACE_FILE_PLAYER))
        va_list ap;
        va_start(ap, format);
        internal_log("FMI", format, ap);
        va_end(ap);
#endif
    }

    /* Normal Logging */
    void NormalLog(const char* category, const char* format, ...)
    {
#if defined(PRIVATE_LOG_PATH_TRACE_FILE_PLAYER) || defined(PUBLIC_LOGGING_TRACE_FILE_PLAYER)
        va_list ap;
        va_start(ap, format);
        InternalLog(category, format, ap);
        va_end(ap);
#endif
    }

    /* Members */
    string instance_name_;
    fmi2Type fmu_type_;
    string fmu_guid_;
    string fmu_resource_location_;
    bool visible_;
    bool logging_on_;
    set<string> logging_categories_;
    fmi2CallbackFunctions functions_;
    fmi2Boolean boolean_vars_[FMI_BOOLEAN_VARS]{};
    fmi2Integer integer_vars_[FMI_INTEGER_VARS]{};
    fmi2Real real_vars_[FMI_REAL_VARS]{};
    string string_vars_[FMI_STRING_VARS];
    string* current_buffer_;
    string* last_buffer_;
    std::unique_ptr<osi3::TraceFileReader> trace_file_reader_;

    int ReallocBuffer(char** message_buf, size_t new_size);

    /* Simple Accessors */
    fmi2Boolean FmiValid() { return boolean_vars_[FMI_BOOLEAN_VALID_IDX]; }
    void SetFmiValid(fmi2Boolean value) { boolean_vars_[FMI_BOOLEAN_VALID_IDX] = value; }
    fmi2Integer FmiCount() { return integer_vars_[FMI_INTEGER_COUNT_IDX]; }
    void SetFmiCount(fmi2Integer value) { integer_vars_[FMI_INTEGER_COUNT_IDX] = value; }
    string FmiTracePath() { return string_vars_[FMI_STRING_TRACE_PATH_IDX]; }
    string FmiTraceName() { return string_vars_[FMI_STRING_TRACE_NAME_IDX]; }

    /* Protocol Buffer Accessors */
    void SetFmiSensorViewOut(const osi3::SensorView& data);
    void SetFmiSensorDataOut(const osi3::SensorData& data);

    void ResetFmiSensorViewOut();
    void ResetFmiSensorDataOut();
};
#endif