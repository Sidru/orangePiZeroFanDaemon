# orangePiZeroFanDaemon
A fan daemon for the orange pi zero. It gets too hot, so I need to install a fan!

Got from -> https://forum.armbian.com/topic/6112-easy-fan-install-and-software-daemon-self-speed-tuning-orange-pi-zero-probably-others-aswell/

Onyl made a few changes to meet my needs.

armbian-config -> pwm

####################################################################################################
########### 							ORIGINAL TEXT          							 ###########
####################################################################################################

Trival endevour, though fun and easy to do.

Orange Pi Zero fan self adapting to temperature, inside Shenzhen Xunlong Software CO.,Limited manufacturered low profile (non-expansion board version) case.
Probably can be used with other SBC's. FanDaemon source provided.

Comes with no warranty of any sort. Use at your own risk.

The software included works with Mainline Kernel 4.13 Armbian image.

Run armbian-config - system - hardware, use arrow keys to highlight "pwm" and press spacebar to [*] enable it. Select save and reboot.

Install the fan how you wish.

I used a 5volt 3cm wide (bigger will not fit), 1cm thick blower fan from ebay (sucks air in from below, blows from side) glued to the ceiling of the case against the top vents.
Could also cut holes into the roof and mount a regular fan to blow air out the top, or on top of the sbc and leave vents unaltered.
Use sticky tape to cover the extra vent slits and round the blower fan if there is a gap, to stop leakage.
I filed the power cable hole in the case wider, to allow more air flow.

Mosfet I used was "FQP30N06L/FQP30N06" very common and cheap if you can wait several weeks for shipping from ebay.
10kohm resistor between outer pins using wires with 2.54 header plugs. Tight fit but doable.

You can use either the +3.3v power rails (pin 1 or 9 on 26 pin header) or +5v for faster fan speed, or for fans that do not handle the lower voltage.
For some reason when using +5v, I only got a very narrow range of operation or 2 effective speeds whereas with 3.3v, duty cycles 20% to 100% worked.
For +5v, pin 1 on the 13 pin header can be used, however the wire tends to hand right on top of fan and risks it being sucked into the blades.

Put the fand executable where ever you want and do...
chown root fand
chmod u+x fand
(I used midnight commander and used in its File menu, set owner can execute and chown adding file to root user and root group)

fand software needs root priviledges for manipulating the pwm interface in /sys folder. Maybe another user in wheel group would do?

run
sudo crontab -e

and below "# m h dom mon dow    command" add (#how to customise below)
@reboot sleep 10 && nohup /root/fand/./fand 48000 5 1 65 10 100 &

Remember to press enter after "&" as the crontab requires a new empty line after declaring the last cronjob.
Sleep is added as I found if the daemon was started too early, there was a chance the fand would fail to start for whatever reason.
* e.g. above for FanD executable located in /root/fand/ sets Duty Cycle freqency to 48000Hz (or something or rather), temperature check at 5 second intervals temperature/DT% 1c(10%DT) to 65c(100%DT)

#fand command line is as below
/(path to execultable)/./fand duty cycle (Duty Cycle (DT) frequency) (polling interval in seconds) (min temperature, lowest DT% => 1) (temp for max DT%) (& to fork into background)

FanD shows Duty Cycle % in its process name when using the "top" command, scroll down using down arrow if you cannot see it at first.

To compile included fand.c source, if you edit it or do not trust the included binary.
gcc -std=gnu99 fand.c -o fand

Daemon has negligible memory and cpu usage.

You can also set a fixed speed with console commands in a script as root. You may need to experiment with values.
echo 0 > /sys/class/pwm/pwmchip0/unexport
echo 0 > /sys/class/pwm/pwmchip0/export
echo normal > /sys/class/pwm/pwmchip0/pwm0/polarity
# set the DT period to 30000 ns
echo 30000 > /sys/class/pwm/pwmchip0/pwm0/period
# e.g. set the duty cycle to 15000 ns (50%DT?)
echo 15000 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle
echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable

to disable pwm (and the fan), as root enter...
echo 0 > /sys/class/pwm/pwmchip0/pwm0/enable

Longer DT periods/lower DT frequency, if used, produces a humming sound from the motor. 

Credits to FanD software to Andrea Cioni.
Forked by me from github version for better compatibility with Orange Pi Zero. Did not comment changes I made, on account of being too lazy.
DT polarity corrected and device in unready state (preventing daemon start) bug improved.

I have ceramic heatsinks on both the ram and soc chip bonded using Artic Silver. Dries and cements the heatsinks to surface rather well.
With cpu governor set to performance (constant 1200mhz), ambient air about 22celcius at idle the cpu temperature is 40-45celcius using the cronjob given above.
