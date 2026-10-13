# DiskSim

This release of disksim takes the 4.0 release from PDL and makes the following changes/enhancements/improvements. See our wiki for further details. 

### Enhancements:
  - 64bit OS compliance
  - 64bit LBA compliance – now supports disk models larger than 2TB
  - Dual Seek Profiles – Support for separate read and write seek tables

### Utilities and Features:
  - Command Logging and tagging for more verbose simulation debugging
  - Model layout tools – Provides a simple user interface to check conversion from LBA to PBA
  - WCE experimental code base – The original disksim does not provide for a write cache enabled mode, this currently does not support idle cache flushing, hence the 'EXPERIMENTAL' directory

## Upgrades

Claude Code updated the original Western Digital repo to work with modern compilers with stricter checks.  It then outputted this message regarding other compilers:

- Tested: GCC 15.2.0 on x86-64. The build succeeds and valid/ascii.parv runs and exits 0.
- Where the problem starts: GCC 14. That's the release that turned implicit declarations and incompatible pointer types into errors, so the original code fails on GCC 14 as well as 15. I haven't tried GCC 14 myself, so that part comes from my knowledge of the compiler change, not from a test.
- Older GCC (13 and earlier): the original code should already build there with warnings. My fix should also work, because the added prototypes are plain valid C that matches the existing definitions. I haven't tested that either.

Basically speaking, it added prototypes to C files and PROTO to Bison files for generating C files.
