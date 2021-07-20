# BYOD (Build-Your-Own Distortion)

![CI](https://github.com/Chowdhury-DSP/BYOD/workflows/CI/badge.svg)
[![License](https://img.shields.io/badge/License-GPL3-blue.svg)](https://opensource.org/licenses/GPL-3.0)

BYOD is a guitar distortion plugin with a customisable
signal chain that allows users to create their own guitar
distortion effects. The plugin contains a wide variety
of distortion effects from analog modelled circuits
to purely digital creations, along with some musical
tone-shaping filters, and a handful of other useful
processing blocks.

BYOD is open to contributions! If you would like to
add new guitar effect processors, please see
[the documentation](./docs).

**BYOD is currently in pre-release. If you would like to try
the latest Nightly Builds (potentially unstable),
they can be downloaded [here](https://chowdsp.com/nightly.html#byod).**

## Building

To build from scratch, you must have CMake installed.

```bash
# Clone the repository
$ git clone https://github.com/Chowdhury-DSP/BYOD.git
$ cd BYOD

# initialize and set up submodules
$ git submodule update --init --recursive

# build with CMake
$ cmake -Bbuild
$ cmake --build build --config Release
```

## Credits:

- GUI Framework - [Plugin GUI Magic](https://github.com/ffAudio/PluginGUIMagic)
- Extra Icons - [FontAwesome](https://fontawesome.com/)

Credits for the individual processing blocks are shown on
the information page for each block. Big thanks to all who
have contributed!

## License

BYOD is open source, and is dual-licensed  under the 
General Public License Version 3. For more details, 
please see the [licensing documentation](./docs/Licensing.md). Enjoy!
