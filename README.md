# alexa-esp32-shower

This project uses the espressif exp32 with capacitive touch sensor to control a solenoid valve connected between the mixer valve and the shower head.  

It can be controlled through alexa which allows you to set it up on schedule and start the shower on weekdays a few minutes before you get in to have the temperature just right before you get in.  

The touch sensor is connected to the copper shower pipe leading to the shower head and allows the user to turn-off the shower by touching the pipe or possibly the shower head if there is a conductive connection between the two.   Otherwise you can use the alternative light switch control installed outside the shower so that it can be turned off without touching normal shower controls (mixer valve) so that the temp is always set to your preference.
<p>
Parts needed:
  <ul><li>
Espressif ESP32 board or equivalent
    <li>
12v dc power supply rated for at least enough amps for the solenoid
      <li>
12v normally closed solenoid valve for 1/2" pipe
 <li>
DC 5V-36V 15A(Max 30A) 400W Dual High-Power MOS Transistor Driving Controller Module FET Trigger Switch Drive Board 0-20KHz PWM Electronic Switch
<li>
            Or instead of the above you can use a 3-5v realy 
</ul>

This code includes a webserver which you can use to <ul><li>enable or disble the touch sensor (in case of false triggers) <li> query for usage stats <li> clear usage statistics.  </ul>

