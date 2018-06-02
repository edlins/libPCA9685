# Changelog


## [0.8] - unreleased
### Added

### Changed
- **examples/olaclient/**: change from sysvinit to systemd, pathing, README
- **.travis.yml**: change from sysvinit to systemd, ldconfig

### Removed


## [0.7] - 2018-05-27
### Added
- **examples/olaclient/**: example application for driving a PCA9685 via DMX512
- **netinst.sh**: build script suitable to be run as an EXTRA from `picontrol-netinst`

### Changed
- **.travis.yml**: install libola-dev on Trusty, add `make examples`
- **[build]**: overhaul: exclude examples from default, every obj has own CMakeLists.txt

### Removed


## [0.6] - 2018-05-09
### Added
- **CMakeLists.txt**: setup cmake at top level and in src/, use lib versioning in lib and apps
- **CMakeLists.txt**: add_test for PCA9685test
- **.travis.yml**: setup Travis CI, whitelist "master" and "develop" (don't build others)
- **.travis.yml**: build is now make && ctest -V.  two tests: run_test and diff_output
- **Config.h.in**: templates for Config.h headers that get cmake version values
- **PCA9685demo.c**: print key menu in ncurses mode
- **PCA9685demo.c**: add many command line options, add -h usage msg
- **PCA9685.h**: added externs for MODE1 and MODE2 options and define all bits
- **PCA9685test.c**: test driver application
- **PCA9685_expected_output**: test driver expected stdout capture

### Changed
- **PCA9685demo.c**: change #define constants to getopt vars
- **PCA9685demo.c**: remove select() delay from loop, update as fast as possible
- **PCA9685demo.c**: fix boundary condition bug in changing selected channel
- **PCA9685demo.c**: only dumpVals() the validation array in ncurses
- **quickstart.c**: fix initHardware() to use local file descriptor returned from PCA9685_openI2C()
- **PCA9685.c**: change #define DEBUG to extern int \_PCA9685_DEBUG
- **PCA9685.c**: remove SUBnBITs from mode1val
- **PCA9685.c**: fix debug logs
- **PCA9685.c**: use externs for MODE1 and MODE2
- **PCA9685.h**: add extern bool \_PCA9685_DEBUG
- **PCA9685.h**: add extern bool \_PCA9685_TEST

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

