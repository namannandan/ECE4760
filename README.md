##This repository contains code for the lab exercises of the course ECE4760.##

###The following programs have been written for an AVR ATMega microcontroller.###


**Lab1 :- Human reaction time tester**
  * Description - A reaction time measurement device which expects the user to press a button as soon as an led lights up. The shortest reaction time is stored in EEPROM. Everytime the the user tests his/her reaction time, the current reaction time that is recorded and the high score (shortest reaction time) is displayed on the lcd. The system also warns the user of cheating if he/she pushes the button before the led lights up. 
  * Microcontroller - ATMega8 (8 MHz internal oscillator)
  * Programmer - USBasp
  * [Video demo](https://www.youtube.com/watch?v=nKClmKczyAs)


**Lab2 :- Digital multimeter**
  * Description - An autoranging digital multimeter capable of measuring frequency, voltage and resistance. The system has three buttons. The first button is used to cycle through the quantities that can be measured (frequency, voltage or resistance). The second button is used to toggle autoranging on or off. The third button is used to manually select a range of measurement when autoranging is diabled. An lcd is used to display :-
    * the mode the system is currntly in 
    * the measured value
    * units
    * selected range
    * A (to indicate autoranging) or M (to indicate manually selected range)
  * Microcontroller - ATMega328P (8 MHz internal oscillator)
  * Programmer - USBasp
  * [Video demo](https://www.youtube.com/watch?v=QhZsdq6Vz5E)
