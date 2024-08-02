//
// Copyright 2018 PMSF IT Consulting - Pierre R. Mai
// Copyright 2022 Persival GmbH
// SPDX-License-Identifier: MPL-2.0
//

#include "OSMPTraceFilePlayer.h"

/*
 * Debug Breaks
 *
 * If you define DEBUG_BREAKS_TRACE_FILE_PLAYER the FMU will automatically break
 * into an attached Debugger on all major computation functions.
 * Note that the FMU is likely to break all environments if no
 * Debugger is actually attached when the breaks are triggered.
 */
#if defined(DEBUG_BREAKS_TRACE_FILE_PLAYER) && !defined(NDEBUG)
#if defined(__has_builtin) && !defined(__ibmxl__)
#if __has_builtin(__builtin_debugtrap)
#define DEBUGBREAK() __builtin_debugtrap()
#elif __has_builtin(__debugbreak)
#define DEBUGBREAK() __debugbreak()
#endif
#endif
#if !defined(DEBUGBREAK)
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#include <intrin.h>
#define DEBUGBREAK() __debugbreak()
#else
#include <signal.h>
#if defined(SIGTRAP)
#define DEBUGBREAK() raise(SIGTRAP)
#else
#define DEBUGBREAK() raise(SIGABRT)
#endif
#endif
#endif
#else
#define DEBUGBREAK()
#endif

#include <string>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <ctime>

using namespace std;
namespace fs = std::experimental::filesystem;

#ifdef PRIVATE_LOG_PATH_TRACE_FILE_PLAYER
ofstream COSMPTraceFilePlayer::private_log_file;
#endif

/*
 * ProtocolBuffer Accessors
 */

int COSMPTraceFilePlayer::ReallocBuffer(char **message_buf, size_t new_size)
{
    char *new_ptr = *message_buf;
    new_ptr = (char *) realloc(new_ptr, new_size);
    if (new_ptr == nullptr)
    {
        NormalLog("OSI", "Failed to allocate buffer memory!!!");
    }
    *message_buf = new_ptr;
    return 0;
}

void *DecodeIntegerToPointer(fmi2Integer hi, fmi2Integer lo)
{
#if PTRDIFF_MAX == INT64_MAX
    union addrconv
    {
        struct
        {
            int lo;
            int hi;
        } base;
        unsigned long long address;
    } myaddr;
    myaddr.base.lo = lo;
    myaddr.base.hi = hi;
    return reinterpret_cast<void *>(myaddr.address);
#elif PTRDIFF_MAX == INT32_MAX
    return reinterpret_cast<void*>(lo);
#else
#error "Cannot determine 32bit or 64bit environment!"
#endif
}

void EncodePointerToInteger(const void *ptr, fmi2Integer &hi, fmi2Integer &lo)
{
#if PTRDIFF_MAX == INT64_MAX
    union addrconv
    {
        struct
        {
            int lo;
            int hi;
        } base;
        unsigned long long address;
    } myaddr;
    myaddr.address = reinterpret_cast<unsigned long long>(ptr);
    hi = myaddr.base.hi;
    lo = myaddr.base.lo;
#elif PTRDIFF_MAX == INT32_MAX
    hi=0;
    lo=reinterpret_cast<int>(ptr);
#else
#error "Cannot determine 32bit or 64bit environment!"
#endif
}

void COSMPTraceFilePlayer::SetFmiSensorViewOut(const osi3::SensorView &data)
{
    data.SerializeToString(current_buffer_);
    EncodePointerToInteger(current_buffer_->data(), integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX], integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX]);
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX] = (fmi2Integer) current_buffer_->length();
    NormalLog("OSMP",
              "Providing %08X %08X, writing from %p ...",
              integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX],
              integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX],
              current_buffer_->data());
    swap(current_buffer_, last_buffer_);
}

void COSMPTraceFilePlayer::SetFmiSensorDataOut(const osi3::SensorData &data)
{
    data.SerializeToString(current_buffer_);
    EncodePointerToInteger(current_buffer_->data(), integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX], integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX]);
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX] = (fmi2Integer) current_buffer_->length();
    NormalLog("OSMP",
              "Providing %08X %08X, writing from %p ...",
              integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX],
              integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX],
              current_buffer_->data());
    swap(current_buffer_, last_buffer_);
}

void COSMPTraceFilePlayer::ResetFmiSensorViewOut()
{
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX] = 0;
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX] = 0;
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX] = 0;
}

void COSMPTraceFilePlayer::ResetFmiSensorDataOut()
{
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX] = 0;
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX] = 0;
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX] = 0;
}

/*
 * Actual Core Content
 */

fmi2Status COSMPTraceFilePlayer::DoInit()
{
    DEBUGBREAK();

    /* Booleans */
    for (int i = 0; i < FMI_BOOLEAN_VARS; i++)
    {
        boolean_vars_[i] = fmi2False;
    }

    /* Integers */
    for (int i = 0; i < FMI_INTEGER_VARS; i++)
    {
        integer_vars_[i] = 0;
    }

    /* Reals */
    for (int i = 0; i < FMI_REAL_VARS; i++)
    {
        real_vars_[i] = 0.0;
    }

    /* Strings */
    for (int i = 0; i < FMI_STRING_VARS; i++)
    {
        string_vars_[i] = "";
    }

    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::DoStart(fmi2Boolean tolerance_defined, fmi2Real tolerance, fmi2Real start_time, fmi2Boolean stop_time_defined, fmi2Real stop_time)
{
    DEBUGBREAK();

    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::DoEnterInitializationMode()
{
    DEBUGBREAK();

    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::DoExitInitializationMode()
{
    DEBUGBREAK();

    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::DoCalc(fmi2Real current_communication_point, fmi2Real communication_step_size, fmi2Boolean no_set_fmu_state_prior_to_current_point)
{
    DEBUGBREAK();
    std::chrono::milliseconds start_source_calc = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    double time = current_communication_point + communication_step_size;

    NormalLog("OSI", "Playing binary SensorView at %f for %f (step size %f)", current_communication_point, time, communication_step_size);

    fs::path dir = FmiTracePath();
    string binary_file_name = dir / FmiTraceName();

    if (binary_file_name.empty())
    {
        // Get first .osi file in directory
        std::vector<fs::directory_entry> entries;
        fs::directory_iterator di(dir);
        fs::directory_iterator end;
        std::copy_if(di, end, std::back_inserter(entries),
                     file_extension_is(".osi"));
        binary_file_name = entries.begin()->path().string();
    }

    std::size_t sv_found = binary_file_name.find("_sv_");
    std::size_t sd_found = binary_file_name.find("_sd_");
    NormalLog("OSI", "Playing binary trace file from %s", binary_file_name.c_str());
    NormalLog("OSI", "At %f for %f (step size %f)", current_communication_point, time, communication_step_size);
    if ((sv_found == std::string::npos) && (sd_found == std::string::npos))
    {
        NormalLog("OSI", "No SensorView or SensorData found in proto binary file!!!");
        return fmi2Error;
    }

    FILE *binary_file = fopen(binary_file_name.c_str(), "rb");
    if (binary_file == nullptr)
    {
        perror("Open failed");
    }

    int is_ok = 1;
    size_t buf_size = 0;
    typedef unsigned int MessageSizeT;
    char *message_buf = nullptr;
    fseek(binary_file, 4 * played_frames_ + total_length_, SEEK_SET);
    MessageSizeT size = 0;
    uint ret = fread(&size, sizeof(MessageSizeT), 1, binary_file);
    if (ret == 0)
    {
        NormalLog("OSI", "End of trace!!!");
    } else if (ret != 1)
    {
        NormalLog("OSI", "Failed to read the size of the message!!!");
        is_ok = 0;
    }
    if ((is_ok != 0) && size > buf_size)
    {
        size_t new_size = size * 2;
        if (ReallocBuffer(&message_buf, new_size) < 0)
        {
            is_ok = 0;
            NormalLog("OSI", "Failed to allocate memory!!!");
        }
    }
    if (is_ok != 0)
    {
        size_t already_read = 0;
        while (already_read < size)
        {
            fseek(binary_file, 4 * (played_frames_ + 1) + total_length_, SEEK_SET);
            uint res = fread(message_buf + already_read, sizeof(message_buf[0]), size - already_read, binary_file);
            if (res == 0)
            {
                NormalLog("OSI", "Unexpected end of file!!!");
                is_ok = 0;
                NormalLog("OSI", "Failed to read the message!!!");
            }
            already_read += res;
        }
    }

    fclose(binary_file);

    if (sv_found != std::string::npos)
    {
        osi3::SensorView current_out;
        if (is_ok != 0)
        {
            std::string message_str(message_buf, message_buf + size);
            if (!current_out.ParseFromString(message_str))
            {
                NormalLog("OSI", "Trace file not parsed correctly!!!");
            }
        }
        NormalLog("OSI", "Buffer length at frame %i: %i", played_frames_, size);

        total_length_ = total_length_ + size;
        NormalLog("OSI", "Total length after frame %i: %i", played_frames_, total_length_);
        played_frames_ = played_frames_ + 1;

        SetFmiSensorViewOut(current_out);
        SetFmiValid(1);
        if (current_out.has_global_ground_truth())
        {
            SetFmiCount(current_out.global_ground_truth().moving_object_size());
        }
    } else if (sd_found != std::string::npos)
    {
        osi3::SensorData current_out;
        if (is_ok != 0)
        {
            std::string message_str(message_buf, message_buf + size);
            if (!current_out.ParseFromString(message_str))
            {
                NormalLog("OSI", "Trace file not parsed correctly!!!");
            }
        }
        NormalLog("OSI", "Buffer length at frame %i: %i", played_frames_, size);

        total_length_ = total_length_ + size;
        NormalLog("OSI", "Total length after frame %i: %i", played_frames_, total_length_);
        played_frames_ = played_frames_ + 1;

        SetFmiSensorDataOut(current_out);
        SetFmiValid(1);
        if (current_out.sensor_view(0).has_global_ground_truth())
        {
            SetFmiCount(current_out.sensor_view(0).global_ground_truth().moving_object_size());
        }
    }

    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::DoTerm()
{
    DEBUGBREAK();
    return fmi2OK;
}

void COSMPTraceFilePlayer::DoFree()
{
    DEBUGBREAK();
}

/*
 * Generic C++ Wrapper Code
 */

COSMPTraceFilePlayer::COSMPTraceFilePlayer(fmi2String theinstance_name,
                                           fmi2Type thefmu_type,
                                           fmi2String thefmu_guid,
                                           fmi2String thefmu_resource_location,
                                           const fmi2CallbackFunctions *thefunctions,
                                           fmi2Boolean thevisible,
                                           fmi2Boolean thelogging_on)
    : instance_name_(theinstance_name),
      fmu_type_(thefmu_type),
      fmu_guid_(thefmu_guid),
      fmu_resource_location_(thefmu_resource_location),
      functions_(*thefunctions),
      visible_(thevisible != 0),
      logging_on_(thelogging_on != 0)
{
    current_buffer_ = new string();
    last_buffer_ = new string();
    logging_categories_.clear();
    logging_categories_.insert("FMI");
    logging_categories_.insert("OSMP");
    logging_categories_.insert("OSI");
}

COSMPTraceFilePlayer::~COSMPTraceFilePlayer()
{
    delete current_buffer_;
    delete last_buffer_;
}

fmi2Status COSMPTraceFilePlayer::SetDebugLogging(fmi2Boolean thelogging_on, size_t n_categories, const fmi2String categories[])
{
    FmiVerboseLog("fmi2SetDebugLogging(%s)", thelogging_on != 0 ? "true" : "false");
    logging_on_ = thelogging_on != 0;
    if ((categories != nullptr) && (n_categories > 0))
    {
        logging_categories_.clear();
        for (size_t i = 0; i < n_categories; i++)
        {
            if (0 == strcmp(categories[i], "FMI"))
            {
                logging_categories_.insert("FMI");
            } else if (0 == strcmp(categories[i], "OSMP"))
            {
                logging_categories_.insert("OSMP");
            } else if (0 == strcmp(categories[i], "OSI"))
            {
                logging_categories_.insert("OSI");
            }
        }
    } else
    {
        logging_categories_.clear();
        logging_categories_.insert("FMI");
        logging_categories_.insert("OSMP");
        logging_categories_.insert("OSI");
    }
    return fmi2OK;
}

fmi2Component COSMPTraceFilePlayer::Instantiate(fmi2String instance_name,
                                                fmi2Type fmu_type,
                                                fmi2String fmu_guid,
                                                fmi2String fmu_resource_location,
                                                const fmi2CallbackFunctions *functions,
                                                fmi2Boolean visible,
                                                fmi2Boolean logging_on)
{
    auto *myc = new COSMPTraceFilePlayer(instance_name, fmu_type, fmu_guid, fmu_resource_location, functions, visible, logging_on);

    if (myc->DoInit() != fmi2OK)
    {
        fmi_verbose_log_global("fmi2Instantiate(\"%s\",%d,\"%s\",\"%s\",\"%s\",%d,%d) = NULL (DoInit failure)",
                               instance_name, fmu_type, fmu_guid,
                               (fmu_resource_location != NULL) ? fmu_resource_location : "<NULL>",
                               "FUNCTIONS", visible, logging_on);
        delete myc;
        return nullptr;
    }

    fmi_verbose_log_global("fmi2Instantiate(\"%s\",%d,\"%s\",\"%s\",\"%s\",%d,%d) = %p",
                           instance_name, fmu_type, fmu_guid,
                           (fmu_resource_location != NULL) ? fmu_resource_location : "<NULL>",
                           "FUNCTIONS", visible, logging_on, myc);
    return (fmi2Component) myc;

}

fmi2Status COSMPTraceFilePlayer::SetupExperiment(fmi2Boolean tolerance_defined, fmi2Real tolerance, fmi2Real start_time, fmi2Boolean stop_time_defined, fmi2Real stop_time)
{
    FmiVerboseLog("fmi2SetupExperiment(%d,%g,%g,%d,%g)", tolerance_defined, tolerance, start_time, stop_time_defined, stop_time);
    return DoStart(tolerance_defined, tolerance, start_time, stop_time_defined, stop_time);
}

fmi2Status COSMPTraceFilePlayer::EnterInitializationMode()
{
    FmiVerboseLog("fmi2EnterInitializationMode()");
    return DoEnterInitializationMode();
}

fmi2Status COSMPTraceFilePlayer::ExitInitializationMode()
{
    FmiVerboseLog("fmi2ExitInitializationMode()");
    return DoExitInitializationMode();
}

fmi2Status COSMPTraceFilePlayer::DoStep(fmi2Real current_communication_point, fmi2Real communication_step_size, fmi2Boolean no_set_fmu_state_prior_to_current_pointfmi_2_component)
{
    FmiVerboseLog("fmi2DoStep(%g,%g,%d)", current_communication_point, communication_step_size, no_set_fmu_state_prior_to_current_pointfmi_2_component);
    return DoCalc(current_communication_point, communication_step_size, no_set_fmu_state_prior_to_current_pointfmi_2_component);
}

fmi2Status COSMPTraceFilePlayer::Terminate()
{
    FmiVerboseLog("fmi2Terminate()");
    return DoTerm();
}

fmi2Status COSMPTraceFilePlayer::Reset()
{
    FmiVerboseLog("fmi2Reset()");

    DoFree();
    return DoInit();
}

void COSMPTraceFilePlayer::FreeInstance()
{
    FmiVerboseLog("fmi2FreeInstance()");
    DoFree();
}

fmi2Status COSMPTraceFilePlayer::GetReal(const fmi2ValueReference vr[], size_t nvr, fmi2Real value[])
{
    FmiVerboseLog("fmi2GetReal(...)");
    for (size_t i = 0; i < nvr; i++)
    {
        if (vr[i] < FMI_REAL_VARS)
        {
            value[i] = real_vars_[vr[i]];
        } else
        {
            return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::GetInteger(const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[])
{
    FmiVerboseLog("fmi2GetInteger(...)");
    for (size_t i = 0; i < nvr; i++)
    {
        if (vr[i] < FMI_INTEGER_VARS)
        {
            value[i] = integer_vars_[vr[i]];
        } else
        {
            return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::GetBoolean(const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[])
{
    FmiVerboseLog("fmi2GetBoolean(...)");
    for (size_t i = 0; i < nvr; i++)
    {
        if (vr[i] < FMI_BOOLEAN_VARS)
        {
            value[i] = boolean_vars_[vr[i]];
        } else
        {
            return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::GetString(const fmi2ValueReference vr[], size_t nvr, fmi2String value[])
{
    FmiVerboseLog("fmi2GetString(...)");
    for (size_t i = 0; i < nvr; i++)
    {
        if (vr[i] < FMI_STRING_VARS)
        {
            value[i] = string_vars_[vr[i]].c_str();
        } else
        {
            return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::SetReal(const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[])
{
    FmiVerboseLog("fmi2SetReal(...)");
    for (size_t i = 0; i < nvr; i++)
    {
        if (vr[i] < FMI_REAL_VARS)
        {
            real_vars_[vr[i]] = value[i];
        } else
        {
            return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::SetInteger(const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[])
{
    FmiVerboseLog("fmi2SetInteger(...)");
    for (size_t i = 0; i < nvr; i++)
    {
        if (vr[i] < FMI_INTEGER_VARS)
        {
            integer_vars_[vr[i]] = value[i];
        } else
        {
            return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::SetBoolean(const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[])
{
    FmiVerboseLog("fmi2SetBoolean(...)");
    for (size_t i = 0; i < nvr; i++)
    {
        if (vr[i] < FMI_BOOLEAN_VARS)
        {
            boolean_vars_[vr[i]] = value[i];
        } else
        {
            return fmi2Error;
        }
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::SetString(const fmi2ValueReference vr[], size_t nvr, const fmi2String value[])
{
    FmiVerboseLog("fmi2SetString(...)");
    for (size_t i = 0; i < nvr; i++)
    {
        if (vr[i] < FMI_STRING_VARS)
        {
            string_vars_[vr[i]] = value[i];
        } else
        {
            return fmi2Error;
        }
    }
    return fmi2OK;
}

/*
 * FMI 2.0 Co-Simulation Interface API
 */

extern "C" {

FMI2_Export const char *fmi2GetTypesPlatform()
{
    return fmi2TypesPlatform;
}

FMI2_Export const char *fmi2GetVersion()
{
    return fmi2Version;
}

FMI2_Export fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean logging_on, size_t n_categories, const fmi2String categories[])
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->SetDebugLogging(logging_on, n_categories, categories);
}

/*
* Functions for Co-Simulation
*/
FMI2_Export fmi2Component fmi2Instantiate(fmi2String instance_name,
                                          fmi2Type fmu_type,
                                          fmi2String fmu_guid,
                                          fmi2String fmu_resource_location,
                                          const fmi2CallbackFunctions *functions,
                                          fmi2Boolean visible,
                                          fmi2Boolean logging_on)
{
    return COSMPTraceFilePlayer::Instantiate(instance_name, fmu_type, fmu_guid, fmu_resource_location, functions, visible, logging_on);
}

FMI2_Export fmi2Status fmi2SetupExperiment(fmi2Component c,
                                           fmi2Boolean tolerance_defined,
                                           fmi2Real tolerance,
                                           fmi2Real start_time,
                                           fmi2Boolean stop_time_defined,
                                           fmi2Real stop_time)
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->SetupExperiment(tolerance_defined, tolerance, start_time, stop_time_defined, stop_time);
}

FMI2_Export fmi2Status fmi2EnterInitializationMode(fmi2Component c)
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->EnterInitializationMode();
}

FMI2_Export fmi2Status fmi2ExitInitializationMode(fmi2Component c)
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->ExitInitializationMode();
}

FMI2_Export fmi2Status fmi2DoStep(fmi2Component c,
                                  fmi2Real current_communication_point,
                                  fmi2Real communication_step_size,
                                  fmi2Boolean no_set_fmu_state_prior_to_current_pointfmi_2_component)
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->DoStep(current_communication_point, communication_step_size, no_set_fmu_state_prior_to_current_pointfmi_2_component);
}

FMI2_Export fmi2Status fmi2Terminate(fmi2Component c)
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->Terminate();
}

FMI2_Export fmi2Status fmi2Reset(fmi2Component c)
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->Reset();
}

FMI2_Export void fmi2FreeInstance(fmi2Component c)
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    myc->FreeInstance();
    delete myc;
}

/*
 * Data Exchange Functions
 */
FMI2_Export fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[])
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->GetReal(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[])
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->GetInteger(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[])
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->GetBoolean(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[])
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->GetString(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[])
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->SetReal(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[])
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->SetInteger(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[])
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->SetBoolean(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[])
{
    auto *myc = (COSMPTraceFilePlayer *) c;
    return myc->SetString(vr, nvr, value);
}

/*
 * Unsupported Features (FMUState, Derivatives, Async DoStep, Status Enquiries)
 */
FMI2_Export fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate *fmu_state)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate fmu_state)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate *fmu_state)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate fmu_state, size_t *size)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate fmu_state, fmi2Byte serialized_state[], size_t size)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serialized_state[], size_t size, fmi2FMUstate *fmu_state)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2GetDirectionalDerivative(fmi2Component c,
                                                    const fmi2ValueReference v_unknown_ref[], size_t n_unknown,
                                                    const fmi2ValueReference v_known_ref[], size_t n_known,
                                                    const fmi2Real dv_known[],
                                                    fmi2Real dv_unknown[])
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2SetRealInputDerivatives(fmi2Component c,
                                                   const fmi2ValueReference vr[],
                                                   size_t nvr,
                                                   const fmi2Integer order[],
                                                   const fmi2Real value[])
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c,
                                                    const fmi2ValueReference vr[],
                                                    size_t nvr,
                                                    const fmi2Integer order[],
                                                    fmi2Real value[])
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2CancelStep(fmi2Component c)
{
    return fmi2OK;
}

FMI2_Export fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status *value)
{
    return fmi2Discard;
}

FMI2_Export fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real *value)
{
    return fmi2Discard;
}

FMI2_Export fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer *value)
{
    return fmi2Discard;
}

FMI2_Export fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean *value)
{
    return fmi2Discard;
}

FMI2_Export fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String *value)
{
    return fmi2Discard;
}

}
