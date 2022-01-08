# WARNING: THIS PROJECT HAS MOVED

You can find the most recent version at the [mw-wf-cli GitLab project page](https://gitlab.com/doragasu/mw-wf-cli). This repository will be kept as is, and will not be updated anymore.

# mw-wf-cli
Command line client for wflash WiFi bootloader

## Usage

### Building
Currently only builing under GNU/Linux is supported. But feel free to adapt the sources to other platforms. And do not forget to send the pull request when you do ;-). To build the program just install a working compiler and use the provided makefile:
```
make
```
This should build the `wflash` program and leave it sitting in the current directory.

### Burning ROMs
`wflash` has built in help. Just launch it and it will tell you the supported options. Of course you will also need a wflash bootloader programmed to a MegaWiFi cartridge, inserted and running on a Genesis/Megadrive consonle. I will detail a bit more this section when I get some more time ¬_¬

## Author and contributions
This program has been written by doragasu. Contributions are welcome. Please don't hesitate sending a pull request.
