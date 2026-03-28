Thanks for choosing the Daniel Borkovec rover experience. The following guide help you navigate your life with our Rover V1.0!

1. The rover has a slow and fast mode, choosable depending on the position of the on the bottom left of the MCU board switch ON START UP.
    Left -> Slow mode   
    Right -> Fast mode

2. This zip includes multiple version of the code. One version has all capabilites included, and one version has only the line following working.

3. The bluetooth module is on and accepting commands on all modes. The rover is programmed to send debug data by console every 100 ms, and accepts char inputs all the time.
    Name: "TEAM 2 ROVER""
    Pin: 1234

    How to connect to it:
    1. Once its paired, connect to it.
    2. If on windows, open a serial terminal to the appropriate port (Frequently ports COM4 or COM5)
    3. If on mac, open a serial terminal to ports (dev/tty.cu.TEAM_2_ROVER)
    4. Enjoy the wireless experience

    The rover accepts the following commands:
    w / s / a / d → slow forward / back / left / right
    W / S / A / D → full-speed forward / back / left / right
    z / Z → coast (stop power) -> coast to full stop, waits for input
    q / Q → full stop (brake) -> won't do anything until restart or another input
    x / X → exit manual → continue line following starting from SEEK mode

4. To get inside the rover to recharge the battery:
    1. Unscrew all four screens
    2. Unplug the two wires coming from the RIGH SIDE module 
            (One four wire going into the PCB -> unplug from the PCB, One four wire going into the top of the color sensing moudle -> UNPLUG FROM THE MODULE SIDE, NOT MCU BOARD (easier that way))
    3. Unplug and take the battery out, it's velcro'd to the bottom, easy swapping
    4. Make sure to reconnect the two wires back to the right side modules (ultrasonic + color sensing)

NOTES:
    1. Somtimes the rover starts in an incorrect state for some reason, simply make sure to start it on and off again. If it keeps spinning in circles it booted wrong. 
    2. The bluetooth is unfortunatelly prone to disconnections. The only way I found to re-enable the serial terminal is to turn the rover off, disconnect from the module, and attempt it again. Unfortunate I know.
    3. For serial terminal, on MAC I use a console download called :tio, very useful in general. Another option is using VSCODE extension PlatformIO's terminal commands (pio monitor). Laast option is the VSCODE debug terminal, have to press enter after every char. 
    4. On windows, I use an app called AntiMicroX to fake keyboard unputs from a Controller. 
            I'm attaching the AntiMicroX file in case you want to show off controlling it using a Controller
            Simply start up AntiMicroX, point it to the config file, connect the controller (it shoudl show up on AntiMicroX), click on the serial terminal and you should see keyboard inputs appearing based on the controller inputs.



PLEASE FEEL MORE THAN FREE TO MESSAGE ME REGARDING ANY CODE UPDATES OR ANY QUESTIONS!!!!
    Name: Daniel Borkovec
    emails: 2941124B@student.gla.ac.uk
            dan.borky@gmaill.com