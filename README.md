# orangePiZeroFanDaemon
A fan daemon for the orange pi zero. It gets too hot, so I needed to install a fan! The fan speed adapts to the temperature.

## Getting Started
I got the original code from [here](https://forum.armbian.com/topic/6112-easy-fan-install-and-software-daemon-self-speed-tuning-orange-pi-zero-probably-others-aswell/).
Only some changes were made to meet my needs, not in the functionality, but in the appearance.

The software was tested under Armbian with kernel 4.19.57-sunxi64

### Prerequisites
Pwm has to be enabled on Armbian. The easiest way is with armbian-config.

Run armbian-config -> system -> hardware, use arrow keys to highlight "pwm" and press spacebar to [*] enable it. Select save and reboot.

### Installing
You can just download the binary, or compile it yourself with
```
gcc -std=gnu99 fand.c -o fand
```
To compile included fand.c source, if you edit it or do not trust the included binary.

After downloading/compiling the source you can check if works with just calling the binary as sudo and giving to it the correct rights.
```
sudo chown root fand
sudo chmod u+f fand
sudo <path_of_executable>./fan <frequence> <polling interval in seconds> <t_min> <t_max> <pwm_min> <pwm_max>
```
If this works, you can now just create a service and save it onto `/etc/systemd/system/fand.service`
```
[Unit]
Description=<Your Description>
After=syslog.target
StartLimitIntervalSec=0

[Service]
Type=simple
Restart=always
RestartSec=1
User=<your_user_or_root>
ExecStart=<path_of_executable>./fan <frequence> <polling interval in seconds> <t_min> <t_max> <pwm_min> <pwm_max>

[Install]
WantedBy=multi-user.target
```

Just call `sudo systemctl enable fand`. After a restart, your Fan will be working (You may turn off the OrangePi).

## License
This project is licensed under the MIT License - see the LICENSE.md file for details.

## ToDos 
Use without root permission
