BinaryEpochClock
================

**THIS MAY NOT BE THE FIRMWARE THAT CAME WITH YOUR CLOCK**

*We are constantly working on improving the firmware of our kits and the firmware contained in this repository may not be the same as that which came pre-programmed on your Binary Epoch Clock Kit.*

There are a few improvements over the original v1.0 Firmware

- Based on changes suggested by [kredal](https://github.com/kredal/) there is an alternte clock face to make the clock face a little more readable. Use the following key to make it a little easier to read: http://maniacallabs.com/misc/Binary_Clock_Key.png
- A 1D Pong face is now included - It is not playable, just sit back and enjoy the game.
- Short press A in order to switch between the three clock faces





The Binary Epoch Clock is a new twist on the old binary clock idea. Instead of just showing the individual digits as binary values, it show [Unix Epoch](http://en.wikipedia.org/wiki/Unix_Epoch) time as a full 32-bit binary value. This unique timepiece will make a great addition to the desk of any computer, electronics, time, or binary geek.

It runs off of an ATMega328P which makes is both fully compatible with the Arduino IDE and easily hackable. Also built in is a DS1307 Real Time Clock chip and battery backup allowing the clock to keep accurate time even in the event of a power loss. The board includes both FTDI and ICSP headers to allow easy upload of new or modified firmware. It can be powered from any USB port or USB power adapter using the included Mini-USB cable.

The clock can bet set manually or via any 5V FTDI cable (not included) and our handy 'sync_time' script which is included in the code repository. See the [assembly and usage guide](http://maniacallabs.com/guides/binary-epoch-clock/) for instructions and a handy video detailing the manual setting procedure for the clock.

To download the firmware source and PCB design files, just click the "Download Zip" button at the right of the GitHub page.

Buy online at the [Maniacal Labs Store](http://maniacallabs.com/product/becv1/)
