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

int COSMPTraceFilePlayer::realloc_buffer(char **message_buf, size_t new_size) {
    char *new_ptr = *message_buf;
    new_ptr = (char *)realloc(new_ptr, new_size);
    if (new_ptr == nullptr) {
        normal_log("OSI", "Failed to allocate buffer memory!!!");
    }
    *message_buf = new_ptr;
    return 0;
}

void* decode_integer_to_pointer(fmi2Integer hi,fmi2Integer lo)
{
#if PTRDIFF_MAX == INT64_MAX
    union addrconv {
        struct {
            int lo;
            int hi;
        } base;
        unsigned long long address;
    } myaddr;
    myaddr.base.lo=lo;
    myaddr.base.hi=hi;
    return reinterpret_cast<void*>(myaddr.address);
#elif PTRDIFF_MAX == INT32_MAX
    return reinterpret_cast<void*>(lo);
#else
#error "Cannot determine 32bit or 64bit environment!"
#endif
}

void encode_pointer_to_integer(const void* ptr,fmi2Integer& hi,fmi2Integer& lo)
{
#if PTRDIFF_MAX == INT64_MAX
    union addrconv {
        struct {
            int lo;
            int hi;
        } base;
        unsigned long long address;
    } myaddr;
    myaddr.address=reinterpret_cast<unsigned long long>(ptr);
    hi=myaddr.base.hi;
    lo=myaddr.base.lo;
#elif PTRDIFF_MAX == INT32_MAX
    hi=0;
    lo=reinterpret_cast<int>(ptr);
#else
#error "Cannot determine 32bit or 64bit environment!"
#endif
}

void COSMPTraceFilePlayer::set_fmi_sensor_view_out(const osi3::SensorView& data)
{
    data.SerializeToString(currentBuffer);
    encode_pointer_to_integer(currentBuffer->data(),integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX],integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX]);
    integer_vars[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX]=(fmi2Integer)currentBuffer->length();
    normal_log("OSMP","Providing %08X %08X, writing from %p ...",integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX],integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX],currentBuffer->data());
    swap(currentBuffer,lastBuffer);
}

void COSMPTraceFilePlayer::set_fmi_sensor_data_out(const osi3::SensorData& data)
{
    data.SerializeToString(currentBuffer);
    encode_pointer_to_integer(currentBuffer->data(),integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX],integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX]);
    integer_vars[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX]=(fmi2Integer)currentBuffer->length();
    normal_log("OSMP","Providing %08X %08X, writing from %p ...",integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX],integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX],currentBuffer->data());
    swap(currentBuffer,lastBuffer);
}

void COSMPTraceFilePlayer::reset_fmi_sensor_view_out()
{
    integer_vars[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX]=0;
    integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX]=0;
    integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX]=0;
}

void COSMPTraceFilePlayer::reset_fmi_sensor_data_out()
{
    integer_vars[FMI_INTEGER_SENSORVIEW_OUT_SIZE_IDX]=0;
    integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASEHI_IDX]=0;
    integer_vars[FMI_INTEGER_SENSORVIEW_OUT_BASELO_IDX]=0;
}

/*
 * Actual Core Content
 */

fmi2Status COSMPTraceFilePlayer::doInit()
{
    DEBUGBREAK();

    /* Booleans */
    for (int i = 0; i<FMI_BOOLEAN_VARS; i++)
        boolean_vars[i] = fmi2False;

    /* Integers */
    for (int i = 0; i<FMI_INTEGER_VARS; i++)
        integer_vars[i] = 0;

    /* Reals */
    for (int i = 0; i<FMI_REAL_VARS; i++)
        real_vars[i] = 0.0;

    /* Strings */
    for (int i = 0; i<FMI_STRING_VARS; i++)
        string_vars[i] = "";

    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::doStart(fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime)
{
    DEBUGBREAK();

    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::doEnterInitializationMode()
{
    DEBUGBREAK();

    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::doExitInitializationMode()
{
    DEBUGBREAK();

    return fmi2OK;
}

void rotatePoint(double x, double y, double z,double yaw,double pitch,double roll,double &rx,double &ry,double &rz)
{
    double matrix[3][3];
    double cos_yaw = cos(yaw);
    double cos_pitch = cos(pitch);
    double cos_roll = cos(roll);
    double sin_yaw = sin(yaw);
    double sin_pitch = sin(pitch);
    double sin_roll = sin(roll);

    matrix[0][0] = cos_yaw*cos_pitch;  matrix[0][1]=cos_yaw*sin_pitch*sin_roll - sin_yaw*cos_roll; matrix[0][2]=cos_yaw*sin_pitch*cos_roll + sin_yaw*sin_roll;
    matrix[1][0] = sin_yaw*cos_pitch;  matrix[1][1]=sin_yaw*sin_pitch*sin_roll + cos_yaw*cos_roll; matrix[1][2]=sin_yaw*sin_pitch*cos_roll - cos_yaw*sin_roll;
    matrix[2][0] = -sin_pitch;         matrix[2][1]=cos_pitch*sin_roll;                            matrix[2][2]=cos_pitch*cos_roll;

    rx = matrix[0][0] * x + matrix[0][1] * y + matrix[0][2] * z;
    ry = matrix[1][0] * x + matrix[1][1] * y + matrix[1][2] * z;
    rz = matrix[2][0] * x + matrix[2][1] * y + matrix[2][2] * z;
}

fmi2Status COSMPTraceFilePlayer::doCalc(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint)
{
    DEBUGBREAK();
    std::chrono::milliseconds start_source_calc = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());

    double time = currentCommunicationPoint+communicationStepSize;

    normal_log("OSI","Playing binary SensorView at %f for %f (step size %f)",currentCommunicationPoint,time,communicationStepSize);

    // Get first .osi file
    fs::path dir = fmi_trace_path();
    std::vector<fs::directory_entry> entries;
    fs::directory_iterator di(dir);
    fs::directory_iterator end;
    std::copy_if(di, end, std::back_inserter(entries),
                 file_extension_is(".osi"));
    std::string binaryFileName = entries.begin()->path().string();

    std::size_t SVFound = binaryFileName.find("_sv_");
    std::size_t SDFound = binaryFileName.find("_sd_");
    normal_log("OSI", "Playing binary trace file from %s", binaryFileName.c_str());
    normal_log("OSI", "At %f for %f (step size %f)", currentCommunicationPoint, time, communicationStepSize);
    if ((SVFound == std::string::npos) && (SDFound == std::string::npos)) {
        normal_log("OSI", "No SensorView or SensorData found in proto binary file!!!");
        return fmi2Error;
    }

    FILE *binaryFile = fopen(binaryFileName.c_str(), "rb");
    if (binaryFile == nullptr) {
        perror("Open failed");
    }

    int is_ok = 1;
    size_t buf_size = 0;
    typedef unsigned int message_size_t;
    char *message_buf = nullptr;
    fseek(binaryFile, 4 * playedFrames + totalLength, SEEK_SET);
    message_size_t size = 0;
    int ret = fread(&size, sizeof(message_size_t), 1, binaryFile);
    if (ret == 0) {
        normal_log("OSI", "End of trace!!!");
    } else if (ret != 1) {
        normal_log("OSI", "Failed to read the size of the message!!!");
        is_ok = 0;
    }
    if (is_ok && size > buf_size) {
        size_t new_size = size * 2;
        if (realloc_buffer(&message_buf, new_size) < 0) {
            is_ok = 0;
            normal_log("OSI", "Failed to allocate memory!!!");
        } else {
            buf_size = new_size;
        }
    }
    if (is_ok) {
        size_t already_read = 0;
        while (already_read < size) {
            fseek(binaryFile, 4  * (playedFrames + 1) + totalLength, SEEK_SET);
            int res = fread(message_buf + already_read, sizeof(message_buf[0]), size - already_read, binaryFile);
            if (res < 0) {
                is_ok = 0;
                normal_log("OSI", "Failed to read the message!!!");
            }
            if (res == 0) {
                normal_log("OSI", "Unexpected end of file!!!");
                is_ok = 0;
                normal_log("OSI", "Failed to read the message!!!");
            }
            already_read += res;
        }
    }

    fclose(binaryFile);

    if (SVFound != std::string::npos) {
        osi3::SensorView currentOut;
        if (is_ok) {
            std::string message_str(message_buf, message_buf + size);
            if (!currentOut.ParseFromString(message_str)) {
                normal_log("OSI", "Trace file not parsed correctly!!!");
            }
        }
        normal_log("OSI","Buffer length at frame %i: %i", playedFrames, size);

        totalLength = totalLength + size;
        normal_log("OSI","Total length after frame %i: %i", playedFrames, totalLength);
        playedFrames = playedFrames + 1;

        float osiSimTime = currentOut.global_ground_truth().timestamp().seconds() + (float)currentOut.global_ground_truth().timestamp().nanos() * 0.000000001;

        std::chrono::milliseconds startOSISerialize = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());

        set_fmi_sensor_view_out(currentOut);
        set_fmi_valid(true);
        if (currentOut.has_global_ground_truth())
            set_fmi_count(currentOut.global_ground_truth().moving_object_size());
    } else if (SDFound != std::string::npos) {
        osi3::SensorData currentOut;
        if (is_ok) {
            std::string message_str(message_buf, message_buf + size);
            if (!currentOut.ParseFromString(message_str)) {
                normal_log("OSI", "Trace file not parsed correctly!!!");
            }
        }
        normal_log("OSI","Buffer length at frame %i: %i", playedFrames, size);

        totalLength = totalLength + size;
        normal_log("OSI","Total length after frame %i: %i", playedFrames, totalLength);
        playedFrames = playedFrames + 1;

        float osiSimTime = currentOut.sensor_view(0).global_ground_truth().timestamp().seconds() + (float)currentOut.sensor_view(0).global_ground_truth().timestamp().nanos() * 0.000000001;

        std::chrono::milliseconds startOSISerialize = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());

        set_fmi_sensor_data_out(currentOut);
        set_fmi_valid(true);
        if (currentOut.sensor_view(0).has_global_ground_truth())
            set_fmi_count(currentOut.sensor_view(0).global_ground_truth().moving_object_size());
    }

    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::doTerm()
{
    DEBUGBREAK();
    return fmi2OK;
}

void COSMPTraceFilePlayer::doFree()
{
    DEBUGBREAK();
}

/*
 * Generic C++ Wrapper Code
 */

COSMPTraceFilePlayer::COSMPTraceFilePlayer(fmi2String theinstanceName, fmi2Type thefmuType, fmi2String thefmuGUID, fmi2String thefmuResourceLocation, const fmi2CallbackFunctions* thefunctions, fmi2Boolean thevisible, fmi2Boolean theloggingOn)
    : instanceName(theinstanceName),
    fmuType(thefmuType),
    fmuGUID(thefmuGUID),
    fmuResourceLocation(thefmuResourceLocation),
    functions(*thefunctions),
    visible(!!thevisible),
    loggingOn(!!theloggingOn)
{
    currentBuffer = new string();
    lastBuffer = new string();
    loggingCategories.clear();
    loggingCategories.insert("FMI");
    loggingCategories.insert("OSMP");
    loggingCategories.insert("OSI");
}

COSMPTraceFilePlayer::~COSMPTraceFilePlayer()
{
    delete currentBuffer;
    delete lastBuffer;
}

fmi2Status COSMPTraceFilePlayer::SetDebugLogging(fmi2Boolean theloggingOn, size_t nCategories, const fmi2String categories[])
{
    fmi_verbose_log("fmi2SetDebugLogging(%s)", theloggingOn ? "true" : "false");
    loggingOn = theloggingOn ? true : false;
    if (categories && (nCategories > 0)) {
        loggingCategories.clear();
        for (size_t i=0;i<nCategories;i++) {
            if (0==strcmp(categories[i],"FMI"))
                loggingCategories.insert("FMI");
            else if (0==strcmp(categories[i],"OSMP"))
                loggingCategories.insert("OSMP");
            else if (0==strcmp(categories[i],"OSI"))
                loggingCategories.insert("OSI");
        }
    } else {
        loggingCategories.clear();
        loggingCategories.insert("FMI");
        loggingCategories.insert("OSMP");
        loggingCategories.insert("OSI");
    }
    return fmi2OK;
}

fmi2Component COSMPTraceFilePlayer::Instantiate(fmi2String instanceName, fmi2Type fmuType, fmi2String fmuGUID, fmi2String fmuResourceLocation, const fmi2CallbackFunctions* functions, fmi2Boolean visible, fmi2Boolean loggingOn)
{
    COSMPTraceFilePlayer* myc = new COSMPTraceFilePlayer(instanceName,fmuType,fmuGUID,fmuResourceLocation,functions,visible,loggingOn);

    if (myc == NULL) {
        fmi_verbose_log_global("fmi2Instantiate(\"%s\",%d,\"%s\",\"%s\",\"%s\",%d,%d) = NULL (alloc failure)",
            instanceName, fmuType, fmuGUID,
            (fmuResourceLocation != NULL) ? fmuResourceLocation : "<NULL>",
            "FUNCTIONS", visible, loggingOn);
        return NULL;
    }

    if (myc->doInit() != fmi2OK) {
        fmi_verbose_log_global("fmi2Instantiate(\"%s\",%d,\"%s\",\"%s\",\"%s\",%d,%d) = NULL (doInit failure)",
            instanceName, fmuType, fmuGUID,
            (fmuResourceLocation != NULL) ? fmuResourceLocation : "<NULL>",
            "FUNCTIONS", visible, loggingOn);
        delete myc;
        return NULL;
    }
    else {
        fmi_verbose_log_global("fmi2Instantiate(\"%s\",%d,\"%s\",\"%s\",\"%s\",%d,%d) = %p",
            instanceName, fmuType, fmuGUID,
            (fmuResourceLocation != NULL) ? fmuResourceLocation : "<NULL>",
            "FUNCTIONS", visible, loggingOn, myc);
        return (fmi2Component)myc;
    }
}

fmi2Status COSMPTraceFilePlayer::SetupExperiment(fmi2Boolean toleranceDefined, fmi2Real tolerance, fmi2Real startTime, fmi2Boolean stopTimeDefined, fmi2Real stopTime)
{
    fmi_verbose_log("fmi2SetupExperiment(%d,%g,%g,%d,%g)", toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
    return doStart(toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
}

fmi2Status COSMPTraceFilePlayer::EnterInitializationMode()
{
    fmi_verbose_log("fmi2EnterInitializationMode()");
    return doEnterInitializationMode();
}

fmi2Status COSMPTraceFilePlayer::ExitInitializationMode()
{
    fmi_verbose_log("fmi2ExitInitializationMode()");
    return doExitInitializationMode();
}

fmi2Status COSMPTraceFilePlayer::DoStep(fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPointfmi2Component)
{
    fmi_verbose_log("fmi2DoStep(%g,%g,%d)", currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPointfmi2Component);
    return doCalc(currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPointfmi2Component);
}

fmi2Status COSMPTraceFilePlayer::Terminate()
{
    fmi_verbose_log("fmi2Terminate()");
    return doTerm();
}

fmi2Status COSMPTraceFilePlayer::Reset()
{
    fmi_verbose_log("fmi2Reset()");

    doFree();
    return doInit();
}

void COSMPTraceFilePlayer::FreeInstance()
{
    fmi_verbose_log("fmi2FreeInstance()");
    doFree();
}

fmi2Status COSMPTraceFilePlayer::GetReal(const fmi2ValueReference vr[], size_t nvr, fmi2Real value[])
{
    fmi_verbose_log("fmi2GetReal(...)");
    for (size_t i = 0; i<nvr; i++) {
        if (vr[i]<FMI_REAL_VARS)
            value[i] = real_vars[vr[i]];
        else
            return fmi2Error;
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::GetInteger(const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[])
{
    fmi_verbose_log("fmi2GetInteger(...)");
    for (size_t i = 0; i<nvr; i++) {
        if (vr[i]<FMI_INTEGER_VARS)
            value[i] = integer_vars[vr[i]];
        else
            return fmi2Error;
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::GetBoolean(const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[])
{
    fmi_verbose_log("fmi2GetBoolean(...)");
    for (size_t i = 0; i<nvr; i++) {
        if (vr[i]<FMI_BOOLEAN_VARS)
            value[i] = boolean_vars[vr[i]];
        else
            return fmi2Error;
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::GetString(const fmi2ValueReference vr[], size_t nvr, fmi2String value[])
{
    fmi_verbose_log("fmi2GetString(...)");
    for (size_t i = 0; i<nvr; i++) {
        if (vr[i]<FMI_STRING_VARS)
            value[i] = string_vars[vr[i]].c_str();
        else
            return fmi2Error;
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::SetReal(const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[])
{
    fmi_verbose_log("fmi2SetReal(...)");
    for (size_t i = 0; i<nvr; i++) {
        if (vr[i]<FMI_REAL_VARS)
            real_vars[vr[i]] = value[i];
        else
            return fmi2Error;
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::SetInteger(const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[])
{
    fmi_verbose_log("fmi2SetInteger(...)");
    for (size_t i = 0; i<nvr; i++) {
        if (vr[i]<FMI_INTEGER_VARS)
            integer_vars[vr[i]] = value[i];
        else
            return fmi2Error;
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::SetBoolean(const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[])
{
    fmi_verbose_log("fmi2SetBoolean(...)");
    for (size_t i = 0; i<nvr; i++) {
        if (vr[i]<FMI_BOOLEAN_VARS)
            boolean_vars[vr[i]] = value[i];
        else
            return fmi2Error;
    }
    return fmi2OK;
}

fmi2Status COSMPTraceFilePlayer::SetString(const fmi2ValueReference vr[], size_t nvr, const fmi2String value[])
{
    fmi_verbose_log("fmi2SetString(...)");
    for (size_t i = 0; i<nvr; i++) {
        if (vr[i]<FMI_STRING_VARS)
            string_vars[vr[i]] = value[i];
        else
            return fmi2Error;
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

    FMI2_Export fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[])
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->SetDebugLogging(loggingOn, nCategories, categories);
    }

    /*
    * Functions for Co-Simulation
    */
    FMI2_Export fmi2Component fmi2Instantiate(fmi2String instanceName,
        fmi2Type fmuType,
        fmi2String fmuGUID,
        fmi2String fmuResourceLocation,
        const fmi2CallbackFunctions* functions,
        fmi2Boolean visible,
        fmi2Boolean loggingOn)
    {
        return COSMPTraceFilePlayer::Instantiate(instanceName, fmuType, fmuGUID, fmuResourceLocation, functions, visible, loggingOn);
    }

    FMI2_Export fmi2Status fmi2SetupExperiment(fmi2Component c,
        fmi2Boolean toleranceDefined,
        fmi2Real tolerance,
        fmi2Real startTime,
        fmi2Boolean stopTimeDefined,
        fmi2Real stopTime)
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->SetupExperiment(toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime);
    }

    FMI2_Export fmi2Status fmi2EnterInitializationMode(fmi2Component c)
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->EnterInitializationMode();
    }

    FMI2_Export fmi2Status fmi2ExitInitializationMode(fmi2Component c)
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->ExitInitializationMode();
    }

    FMI2_Export fmi2Status fmi2DoStep(fmi2Component c,
        fmi2Real currentCommunicationPoint,
        fmi2Real communicationStepSize,
        fmi2Boolean noSetFMUStatePriorToCurrentPointfmi2Component)
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->DoStep(currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPointfmi2Component);
    }

    FMI2_Export fmi2Status fmi2Terminate(fmi2Component c)
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->Terminate();
    }

    FMI2_Export fmi2Status fmi2Reset(fmi2Component c)
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->Reset();
    }

    FMI2_Export void fmi2FreeInstance(fmi2Component c)
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        myc->FreeInstance();
        delete myc;
    }

    /*
     * Data Exchange Functions
     */
    FMI2_Export fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[])
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->GetReal(vr, nvr, value);
    }

    FMI2_Export fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[])
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->GetInteger(vr, nvr, value);
    }

    FMI2_Export fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[])
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->GetBoolean(vr, nvr, value);
    }

    FMI2_Export fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[])
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->GetString(vr, nvr, value);
    }

    FMI2_Export fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[])
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->SetReal(vr, nvr, value);
    }

    FMI2_Export fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[])
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->SetInteger(vr, nvr, value);
    }

    FMI2_Export fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[])
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->SetBoolean(vr, nvr, value);
    }

    FMI2_Export fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[])
    {
        COSMPTraceFilePlayer* myc = (COSMPTraceFilePlayer*)c;
        return myc->SetString(vr, nvr, value);
    }

    /*
     * Unsupported Features (FMUState, Derivatives, Async DoStep, Status Enquiries)
     */
    FMI2_Export fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* FMUstate)
    {
        return fmi2Error;
    }

    FMI2_Export fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate FMUstate)
    {
        return fmi2Error;
    }

    FMI2_Export fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate)
    {
        return fmi2Error;
    }

    FMI2_Export fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t *size)
    {
        return fmi2Error;
    }

    FMI2_Export fmi2Status fmi2SerializeFMUstate (fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte serializedState[], size_t size)
    {
        return fmi2Error;
    }

    FMI2_Export fmi2Status fmi2DeSerializeFMUstate (fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate)
    {
        return fmi2Error;
    }

    FMI2_Export fmi2Status fmi2GetDirectionalDerivative(fmi2Component c,
        const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
        const fmi2ValueReference vKnown_ref[] , size_t nKnown,
        const fmi2Real dvKnown[],
        fmi2Real dvUnknown[])
    {
        return fmi2Error;
    }

    FMI2_Export fmi2Status fmi2SetRealInputDerivatives(fmi2Component c,
        const  fmi2ValueReference vr[],
        size_t nvr,
        const  fmi2Integer order[],
        const  fmi2Real value[])
    {
        return fmi2Error;
    }

    FMI2_Export fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c,
        const   fmi2ValueReference vr[],
        size_t  nvr,
        const   fmi2Integer order[],
        fmi2Real value[])
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
        return fmi2Discard;
    }

    FMI2_Export fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String* value)
    {
        return fmi2Discard;
    }

}
