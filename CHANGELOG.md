# Changelog


## [0.6] - unreleased
### Added
- **CMakeLists.txt**: setup cmake at top level and in src/, use versioning in lib and apps
- **.travis.yml**: setup Travis CI
- **Config.h.in**: templates for Config.h headers that get cmake version values
- **PCA9685demo.c**: print key menu

### Changed
- **PCA9685demo.c**: change #define constants to getopt vars
- **PCA9685demo.c**: remove select() delay from loop, update as fast as possible
- **PCA9685demo.c**: fix boundary condition bug in changing selected channel
- **PCA9685demo.c**: only dumpVals() the validation array in ncurses
- **quickstart.c**: fix initHardware() to use local file descriptor returned from PCA9685_openI2C()
- **PCA9685.c**: change #define DEBUG to extern int \_PCA9685_DEBUG
- **PCA9685.c**: remove SUBnBITs from mode1val
- **PCA9685.c**: fix debug logs
- **PCA9685.h**: add extern int _PCA9685_DEBUG

### Removed
- **Makefile**: all Makefiles removed


## [0.5] - 2018-04-18
### Added
- **PCA9685.c**: add logic for setting MODE2 register based on OPENDRAIN and INVERT
- **PCA9685demo.c**: add hsv2rgb() to better support auto mode color cycle
- **PCA9685demo.c**: add manual commands '[' and ']' for setting channel to MIN or MAX
- **CHANGELOG.md**: added

### Changed
- **PCA9685.c**: fix setting of MODE1 register

### Removed
- 

