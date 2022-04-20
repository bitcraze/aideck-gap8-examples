---
title: SDK
page_id: sdk
---

# Working with the GAP8 SDK

Due to some changes made in the GAP8 SDK it's not easy to use a local installation of the official
SDK yet, so use our dockerized version instead. If you would like to understand the changes we've made,
have a look in the [aideck-docker repository](https://github.com/bitcraze/docker-aideck) and the
[GreenWaves SDK](https://github.com/GreenWaves-Technologies/gap_sdk).

To see a list of what SDK versions we have docker containers for have a look on [our Docker hub](https://hub.docker.com/r/bitcraze/aideck/tags),
note that we only aim to keep the examples working with the latest tag.

For additional information on the SDK, have a look at the [GreenWaves GAP SDK documentation](https://github.com/GreenWaves-Technologies/gap_sdk).

## Note on initializing the file-system

Due to how our bootloader works, it's mandatory to remap where the filesystem partition pointer
is located. This means both that our docker container must be used (since changes are not pushed
up-stream yet) and that special care must be taken when initializing the filesystem.

Below is an extract of when initializing the file-system, the line ```conf.fs.offset_in_flash = 0x40000```
is mandatory and differs from the official GAP8 examples.

```c
pi_readfs_conf_init(&conf);
conf.fs.flash = flash;
conf.fs.offset_in_flash = 0x40000;
pi_open_from_conf(fs, &conf);
if (pi_fs_mount(fs))
{
    printf("Error FS mounting !\n");
    pmsis_exit(-2);
}
```
