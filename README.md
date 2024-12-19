# SL-5-5 OSI Trace File Player

This [FMU](https://fmi-standard.org/) is able to play OSI trace files. It supports the formats `.mcap`, `.osi`, and `.txth`.
For `.osi` and `.txth` files, the file names have to comply to the [OSI trace file naming convention](https://opensimulationinterface.github.io/osi-antora-generator/asamosi/latest/interface/architecture/trace_file_naming.html),
as the player parses the name to identify the message type (e.g. SensorView).
The folder containing the trace files has to be passed as FMI parameter _trace_path_.
The trace file player is build according to the [ASAM Open simulation Interface (OSI)](https://github.com/OpenSimulationInterface/open-simulation-interface) and the [OSI Sensor Model Packaging (OSMP)](https://github.com/OpenSimulationInterface/osi-sensor-model-packaging) examples.

An exemplary trace file is available in folder _trace_file_examples_.

## Parameterization

The following FMI parameters can be set.
At least the `trace_path` has to be set.
Otherwise, the FMU will return with an error.

| Type   | Parameter    | Default | Description                                                                                                   |
|--------|--------------|---------|---------------------------------------------------------------------------------------------------------------|
| String | `trace_path` | _""_    | Path to the directory containing one or more OSI trace files                                                  |
| String | `trace_name` | _""_    | Filename of the trace file to be played. If empty, the first OSI trace file in the given directory is played. |

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
