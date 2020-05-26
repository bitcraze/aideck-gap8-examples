First get the 3.1 GAP SDK to get OpenOCD for gap8.

Configure it for this target:
source configs/gapuino_v2.sh

Once the SDK is compiled, also compile OpenOCD with:
make opencd

Then either compile and run the test with:
make all run

Or directly run the precompiled test with:
openocd -f interface/ftdi/gapuino_ftdi.cfg -f target/gap8revb.tcl -f tcl/jtag_boot.tcl -c "gap8_jtag_load_binary_and_start test elf"



# How to install openocd
* You will find in this archive a version of openocd ported to support GAP8 basic operations.
* Installation follows the straightforward ./configure && make && make install procedure
* You might if you so witch add a PREFIX=My_Prefix option to change default install target (/usr)

# How to use it to run a binary over jtag
* Now that you have openocd installed you might run binaries and do some basic debug (halt core, monitor regs...)
* To load a binary in L2 memory and execute using jtag proceed as follow:
** In a first terminal execute, considering you are in gap_tools archive: 
openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg -f target/gap8revb.tcl -f ./tcl/jtag_boot.tcl
** In second terminal run:
telnet localhost 4444
** in the telnet terminal invoke command as follows
gap8_jtag_load_binary_and_start ./gap_sdk/tools/gap8-openocd-tools/gap_bins/gap_flasher@gapoc_a.elf elf
*** this will load and run your binary. You can halt the core with the command "halt" or reset with "reset" (will erase the program)

# How to fuse (for hyperflash boot) (not needed for ai-deckv2)
* To be able to boot from hyperflash you will need to use a fuser
* The process is as follows (most is scripted)
** Run openocd with these arguments:
*** interface/ftdi/olimex-arm-usb-ocd-h.cfg -f target/gap8revb.tcl -f ./tcl_scripts/jtag_boot.tcl -f ./tcl_scripts/fuser.tcl -f ./tcl_scripts/fuser_flash.tcl
*** load the gap_fuser@gapoc_a.elf binary contains in gap_bins of the archive
*** then simply run: "fuse_hyperflash_boot"
*** you can unplug your board and it should now boot from flash.
*** After this manipulation, you might need to redo a reset of the board when pluging openocd for jtag boot

# How to generate and flash an image (raw image) (a similar code can be found in the pulp-dronet repository)
* For image generation: see with Germain for the exact procedure, the python script is also in the archive 
* to flash the image:
** Run openocd with this arguments: 
openocd -f interface/ftdi/olimex-arm-usb-tiny-h.cfg -f target/gap8revb.tcl -f ./tcl/jtag_boot.tcl -f ./tcl/flash_image.tcl
** load the flasher binary (gapoc_a) from the gap_bins
** then run the command: 
gap_flash_raw ./my_flash_img.raw my_img_size ./gap_sdk/tools/gap8-openocd-tools

