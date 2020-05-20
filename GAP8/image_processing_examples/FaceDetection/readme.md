
When installing the gap sdk (3.4), make sure that you have installed everything:
`make all`
`make openocd`
`make gap_tools`
`make pulp_tools`

To start out, first write this in the terminal:
`source (YOUR GAP SDK FOLDER)/configs/ai_deck.sh`
`export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg`

To make the facedetection application
`make clean`
`make all PMSIS_OS=pulpos`

