# NanoCockpit

[NanoCockpit](https://github.com/idsia-robotics/nanocockpit) is an application framework for
AI-based applications on the Crazyflie with an AI-deck. This folder contains the framework for the
GAP8 (`lib/`), example applications (`examples/`) and a viewer for the computer
(`client/aideck_cpx_streamer/`). The NanoCockpit examples need the
[ESP32 firmware for NanoCockpit](https://github.com/bitcraze/aideck-esp-firmware-v2).

## Documentation

* [NanoCockpit overview and requirements](../docs/examples/nanocockpit/index.md)
* [NanoCockpit streamer](../docs/examples/nanocockpit/streamer.md)
* [PULP-Frontnet](../docs/examples/nanocockpit/pulp-frontnet.md)
* [Component examples](../docs/examples/nanocockpit/component-examples.md)
* [Switching between NanoCockpit and the original firmware](../docs/development/switching-firmware.md)

## More in upstream NanoCockpit

Some parts of NanoCockpit are not included here. They are found in the
[upstream repository](https://github.com/idsia-robotics/nanocockpit):

* the closed-loop follow-me demo, in which the Crazyflie follows a person using the on-board
  neural network
* a ROS 2 viewer
* Wireshark dissectors for the NanoCockpit protocols
* a notebook for analysing traces from the ESP32

## Credit

NanoCockpit was created by Elia Cereda, Alessandro Giusti and Daniele Palossi (IDSIA, USI-SUPSI;
IIS, ETH Zürich). If you use NanoCockpit in an academic context, please cite their publication:

E. Cereda, A. Giusti, and D. Palossi, "NanoCockpit: Performance-optimized Application Framework for
AI-based Autonomous Nanorobotics", in IEEE Robotics and Automation Practice, 2026.
[IEEE Xplore](https://ieeexplore.ieee.org/document/11494460),
[arXiv](https://arxiv.org/abs/2601.07476)

```bibtex
@article{cereda2025nanocockpit,
  author={Cereda, Elia and Giusti, Alessandro and Palossi, Daniele},
  title={NanoCockpit: Performance-optimized Application Framework for AI-based Autonomous Nanorobotics},
  journal={IEEE Robotics and Automation Practice},
  year={2026},
  volume={},
  number={},
  pages={1-6},
  doi={10.1109/RAP.2026.3687493}
}
```

## License

NanoCockpit is licensed under the Apache License 2.0, see [LICENSE](LICENSE). The exceptions are
`lib/crc32.c` and `lib/crc32.h`, which are licensed under the GNU General Public License version 3,
as stated in their headers.
