WDDisksim includes Western Digital Technologies, Inc. modifications after applying the sst-disksim-64bit-patch formerly available from Sandia Corporation at (https://code.google.com/p/sst-simulator/).  

The WD modified files to sst-disksim-64bit-patch are identified below:  

diskmodel/layout_g1.c
diskmodel/layout_g2.c
diskmodel/layout_g4.c
diskmodel/layout_g4_tools/g4_skews.c
diskmodel/Makefile
libddbg/libddbg.h
libddbg/Makefile
libparam/libparam.h
libparam/libparam.y
libparam/Makefile
libparam/util.c
src/disksim_bus.c
src/disksim_controller.c
src/disksim_device.c
src/disksim_disk.c
src/disksim_diskctlr.c
src/disksim_global.h
src/disksim_interface.c
src/disksim_interface.h
src/disksim_iodriver.c
src/disksim_ioqueue.c
src/disksim_iosim.c
src/disksim_loadparams.c
src/disksim_logorg.c
disksim_simpledisk.c
src/disksim_synthio.c
src/modules/ctlr.modspec
src/modules/disk.modspec
src/modules/iodriver.modspec
src/modules/simpledisk.modspec
utils/params/Makefile

In addition, the following files were modified by the application of the sst-disksim-64bit-patch (https://sst-simulator.org/) on DiskSim 4.0 (http://www.pdl.cmu.edu/PDL-FTP/DriveChar/disksim-4.0.tar.gz).

/libparam: myutil.c
memsmodel/*
disksim_2.01.016_with_dual_seek/src: seek_sim2.c
disksim_2.01.016_with_dual_seek/src: seek_sim.c
disksim_2.01.016_with_dual_seek/src: skippy_opts.c
disksim_2.01.016_with_dual_seek/src: skippy_opts.h
disksim_2.01.016_with_dual_seek/src: skippy_sim.c
disksim_2.01.016_with_dual_seek/src: skippy_sim_cur.c
disksim_2.01.016_with_dual_seek/src: syssim_interface.c
disksim_2.01.016_with_dual_seek/src: syssim_interface.h
disksim_2.01.016_with_dual_seek/src: syssim_opts.c
disksim_2.01.016_with_dual_seek/src: syssim_opts.h

Additional information regarding file-level WD modifications to DiskSim available in the accompanying diff files found in /diffs/.
