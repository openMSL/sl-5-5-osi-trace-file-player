# SL-5-5 OSI Trace File Player

This [FMU](https://fmi-standard.org/) is able to play binary OSI trace files.
The trace file names have to comply to the [OSI trace file naming convention](https://opensimulationinterface.github.io/osi-antora-generator/asamosi/latest/interface/architecture/trace_file_naming.html),
as the player parses the name to identify the message type (SensorView or SensorData) of the trace file.
The folder containing the trace files has to be passed as FMI parameter _trace_path_.
The trace file player is build according to the [ASAM Open simulation Interface (OSI)](https://github.com/OpenSimulationInterface/open-simulation-interface) and the [OSI Sensor Model Packaging (OSMP)](https://github.com/OpenSimulationInterface/osi-sensor-model-packaging) examples.

An exemplary trace file is available in folder _trace_file_examples_.

## Installation

### Dependencies

Install `cmake` (at least version 3.10.2):

```bash
sudo apt-get install cmake
```

Install `protobuf` (at least version 3.0.0):

```bash
sudo apt-get install libprotobuf-dev protobuf-compiler
```

### Clone with submodules

```bash
git clone https://github.com/openMSL/sl-5-5-osi-trace-file-player.git
cd sl-5-5-osi-trace-file-player
git submodule update --init
```

### Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```
