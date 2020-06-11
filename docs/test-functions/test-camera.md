---
title: Test Himax Camera
page_id: test-camera
---

# Testing the Himax camera on the AIdeck


This concers the example in folder *GAP8/test_functionalities/test_camera/*

In the makefile enable `APP_CFLAGS += -DASYNC_CAPTURE` if you want to test the asynchronous camera capture and remove it if you want to test the normal one.

To test out the code, first source the AIdeck config (configs/ai_deck.sh) in your terminal and write 

    make clean all run io=host

for onram flashing with your programmer.
