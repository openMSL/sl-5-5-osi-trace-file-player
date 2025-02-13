//
// Copyright 2018 PMSF IT Consulting - Pierre R. Mai
// Copyright 2022 Persival GmbH
// SPDX-License-Identifier: MPL-2.0
//

#include "OSMPTraceFilePlayer.h"

#include <filesystem>

#include "osi-utilities/tracefile/Reader.h"

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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

using namespace std;

#ifdef PRIVATE_LOG_PATH_TRACE_FILE_PLAYER
ofstream COSMPTraceFilePlayer::private_log_file;
#endif

/*
 * ProtocolBuffer Accessors
 */

void* DecodeIntegerToPointer(fmi2Integer hi, fmi2Integer lo)
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
    } myaddr{};
    myaddr.base.lo = lo;
    myaddr.base.hi = hi;
    return reinterpret_cast<void*>(myaddr.address);
#elif PTRDIFF_MAX == INT32_MAX
    return reinterpret_cast<void*>(lo);
#else
#error "Cannot determine 32bit or 64bit environment!"
#endif
}

void EncodePointerToInteger(const void* ptr, fmi2Integer& hi, fmi2Integer& lo)
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
    } myaddr{};
    myaddr.address = reinterpret_cast<unsigned long long>(ptr);
    hi = myaddr.base.hi;
    lo = myaddr.base.lo;
#elif PTRDIFF_MAX == INT32_MAX
    hi = 0;
    lo = reinterpret_cast<int>(ptr);
#else
#error "Cannot determine 32bit or 64bit environment!"
#endif
}

void COSMPTraceFilePlayer::SetFmiSensorViewOut(const osi3::SensorView& data)
{
    data.SerializeToString(current_buffer_);
    EncodePointerToInteger(current_buffer_->data(), integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX], integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX]);
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX] = static_cast<fmi2Integer>(current_buffer_->length());
    NormalLog("OSMP",
              "Providing %08X %08X, writing from %p ...",
              integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX],
              integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX],
              current_buffer_->data());
    swap(current_buffer_, last_buffer_);
}

void COSMPTraceFilePlayer::SetFmiSensorDataOut(const osi3::SensorData& data)
{
    data.SerializeToString(current_buffer_);
    EncodePointerToInteger(current_buffer_->data(), integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX], integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX]);
    integer_vars_[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX] = static_cast<fmi2Integer>(current_buffer_->length());
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
    for (int& boolean_var : boolean_vars_)
    {
        boolean_var = fmi2False;
    }

    /* Integers */
    for (int& integer_var : integer_vars_)
    {
        integer_var = 0;
    }

    /* Reals */
    for (double& real_var : real_vars_)
    {
        real_var = 0.0;
    }

    /* Strings */
    for (auto& string_var : string_vars_)
    {
        string_var = "";
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

    const std::filesystem::path folder_path = FmiTracePath();
    std::string trace_file_name = FmiTraceName();
    if (trace_file_name.empty())
    {
        for (const auto& entry : std::filesystem::directory_iterator(folder_path))
        {
            if (entry.path().extension() == ".osi")
            {
                trace_file_name = entry.path().filename().string();
                break;
            }
        }
    }

    if (trace_file_name.empty())
    {
        throw std::runtime_error("No trace file found in " + folder_path.string());
    }

    const std::filesystem::path trace_path = folder_path / trace_file_name;

    trace_file_reader_ = osi3::TraceFileReaderFactory::createReader(trace_path);

    if (!trace_file_reader_->Open(trace_path))
    {

        return fmi2Fatal;
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::DoCalc(fmi2Real current_communication_point, fmi2Real communication_step_size, fmi2Boolean no_set_fmu_state_prior_to_current_point)
{

    if (!trace_file_reader_->HasNext())
    {
        std::cerr << "End of trace file reached (experiment stopTime longer than tracefile)" << std::endl;
        return fmi2Discard;
    }

    const auto reading_result = trace_file_reader_->ReadMessage();
    if (!reading_result)
    {
        std::cerr << "Error reading message." << std::endl;
        return fmi2Fatal;
    }

    switch (reading_result->message_type)
    {
        case osi3::ReaderTopLevelMessage::kSensorData: {
            auto* const sensor_data = dynamic_cast<osi3::SensorData*>(reading_result->message.get());
            SetFmiSensorDataOut(*sensor_data);
            break;
        }
        case osi3::ReaderTopLevelMessage::kSensorView: {
            auto* const sensor_view = dynamic_cast<osi3::SensorView*>(reading_result->message.get());
            SetFmiSensorViewOut(*sensor_view);
            break;
        }
        default: {
            std::cerr << "Could not determine type of message or is not a SensorData or SensorView" << std::endl;
            return fmi2Fatal;
        }
    }
    SetFmiValid(1);
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::DoTerm()
{
    DEBUGBREAK();
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::GetBooleanStatus(fmi2StatusKind s, fmi2Boolean* value) const
{
    if (s == fmi2Terminated)
    {
        return trace_file_reader_->HasNext() ? fmi2Discard : fmi2OK;
    }
    return fmi2Discard;
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
                                           const fmi2CallbackFunctions* thefunctions,
                                           fmi2Boolean thevisible,
                                           fmi2Boolean thelogging_on)
    : instance_name_(theinstance_name),
      fmu_type_(thefmu_type),
      fmu_guid_(thefmu_guid),
      fmu_resource_location_(thefmu_resource_location),
      visible_(thevisible != 0),
      logging_on_(thelogging_on != 0),
      functions_(*thefunctions)

{
    current_buffer_ = new string();
    last_buffer_ = new string();

    logging_categories_.clear();
    logging_categories_.insert("FMI");
    logging_categories_.insert("OSMP");
    logging_categories_.insert("OSI");
}

COSMPTraceFilePlayer::~COSMPTraceFilePlayer() = default;

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
            }
            else if (0 == strcmp(categories[i], "OSMP"))
            {
                logging_categories_.insert("OSMP");
            }
            else if (0 == strcmp(categories[i], "OSI"))
            {
                logging_categories_.insert("OSI");
            }
        }
    }
    else
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
                                                const fmi2CallbackFunctions* functions,
                                                fmi2Boolean visible,
                                                fmi2Boolean logging_on)
{
    auto* myc = new COSMPTraceFilePlayer(instance_name, fmu_type, fmu_guid, fmu_resource_location, functions, visible, logging_on);

    fmi_verbose_log_global(R"(fmi2Instantiate("%s",%d,"%s","%s","%s",%d,%d) = %p)",
                           instance_name,
                           fmu_type,
                           fmu_guid,
                           (fmu_resource_location != nullptr) ? fmu_resource_location : "<NULL>",
                           "FUNCTIONS",
                           visible,
                           logging_on,
                           myc);
    return (fmi2Component)myc;
}

fmi2Status COSMPTraceFilePlayer::SetupExperiment(fmi2Boolean tolerance_defined,  // NOLINT (returns always OK)
                                                 fmi2Real tolerance,
                                                 fmi2Real start_time,
                                                 fmi2Boolean stop_time_defined,
                                                 fmi2Real stop_time)
{
    FmiVerboseLog("fmi2SetupExperiment(%d,%g,%g,%d,%g)", tolerance_defined, tolerance, start_time, stop_time_defined, stop_time);
    return DoStart(tolerance_defined, tolerance, start_time, stop_time_defined, stop_time);
}

fmi2Status COSMPTraceFilePlayer::EnterInitializationMode()  // NOLINT (returns always OK)
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

fmi2Status COSMPTraceFilePlayer::Terminate()  // NOLINT (returns always OK)
{
    FmiVerboseLog("fmi2Terminate()");
    return DoTerm();
}

fmi2Status COSMPTraceFilePlayer::Reset()  // NOLINT (returns always OK)
{
    FmiVerboseLog("fmi2Reset()");
    if (trace_file_reader_ != nullptr)
    {
        trace_file_reader_->Close();
    }
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
        }
        else
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
        }
        else
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
        }
        else
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
        }
        else
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
        }
        else
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
        }
        else
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
        }
        else
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
        }
        else
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

FMI2_Export const char* fmi2GetTypesPlatform()
{
    return fmi2TypesPlatform;
}

FMI2_Export const char* fmi2GetVersion()
{
    return fmi2Version;
}

FMI2_Export fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean logging_on, size_t n_categories, const fmi2String categories[])
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->SetDebugLogging(logging_on, n_categories, categories);
}

/*
 * Functions for Co-Simulation
 */
FMI2_Export fmi2Component fmi2Instantiate(fmi2String instance_name,
                                          fmi2Type fmu_type,
                                          fmi2String fmu_guid,
                                          fmi2String fmu_resource_location,
                                          const fmi2CallbackFunctions* functions,
                                          fmi2Boolean visible,
                                          fmi2Boolean logging_on)
{
    return COSMPTraceFilePlayer::Instantiate(instance_name, fmu_type, fmu_guid, fmu_resource_location, functions, visible, logging_on);
}

FMI2_Export fmi2Status
fmi2SetupExperiment(fmi2Component c, fmi2Boolean tolerance_defined, fmi2Real tolerance, fmi2Real start_time, fmi2Boolean stop_time_defined, fmi2Real stop_time)
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->SetupExperiment(tolerance_defined, tolerance, start_time, stop_time_defined, stop_time);
}

FMI2_Export fmi2Status fmi2EnterInitializationMode(fmi2Component c)
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->EnterInitializationMode();
}

FMI2_Export fmi2Status fmi2ExitInitializationMode(fmi2Component c)
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->ExitInitializationMode();
}

FMI2_Export fmi2Status fmi2DoStep(fmi2Component c,
                                  fmi2Real current_communication_point,
                                  fmi2Real communication_step_size,
                                  fmi2Boolean no_set_fmu_state_prior_to_current_pointfmi_2_component)
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->DoStep(current_communication_point, communication_step_size, no_set_fmu_state_prior_to_current_pointfmi_2_component);
}

FMI2_Export fmi2Status fmi2Terminate(fmi2Component c)
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->Terminate();
}

FMI2_Export fmi2Status fmi2Reset(fmi2Component c)
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->Reset();
}

FMI2_Export void fmi2FreeInstance(fmi2Component c)
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    myc->FreeInstance();
    delete myc;
}

/*
 * Data Exchange Functions
 */
FMI2_Export fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[])
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->GetReal(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[])
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->GetInteger(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[])
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->GetBoolean(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[])
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->GetString(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[])
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->SetReal(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[])
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->SetInteger(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[])
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->SetBoolean(vr, nvr, value);
}

FMI2_Export fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[])
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->SetString(vr, nvr, value);
}

/*
 * Unsupported Features (FMUState, Derivatives, Async DoStep, Status Enquiries)
 */
FMI2_Export fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* fmu_state)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate fmu_state)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* fmu_state)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate fmu_state, size_t* size)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate fmu_state, fmi2Byte serialized_state[], size_t size)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serialized_state[], size_t size, fmi2FMUstate* fmu_state)
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2GetDirectionalDerivative(fmi2Component c,
                                                    const fmi2ValueReference v_unknown_ref[],
                                                    size_t n_unknown,
                                                    const fmi2ValueReference v_known_ref[],
                                                    size_t n_known,
                                                    const fmi2Real dv_known[],
                                                    fmi2Real dv_unknown[])
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], const fmi2Real value[])
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer order[], fmi2Real value[])
{
    return fmi2Error;
}

FMI2_Export fmi2Status fmi2CancelStep(fmi2Component c)
{
    return fmi2OK;
}

FMI2_Export fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status* value)
{
    return fmi2Discard;
}

FMI2_Export fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real* value)
{
    return fmi2Discard;
}

FMI2_Export fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* value)
{
    return fmi2Discard;
}

FMI2_Export fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* value)
{
    auto* myc = static_cast<COSMPTraceFilePlayer*>(c);
    return myc->GetBooleanStatus(s, value);
}

FMI2_Export fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String* value)
{
    return fmi2Discard;
}
}
