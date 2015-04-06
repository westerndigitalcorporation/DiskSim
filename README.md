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
