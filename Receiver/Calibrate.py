from Position import save_position
from gpiozero import DigitalOutputDevice

def home():
    enable.on()
    print("Homing: move to mechanical zero manually")
    input("Press Enter when axis is at 0")
    enable.off()
    save_position(0)
    print("Homing complete")
    return 0

if __name__ == "__main__":
    enable = DigitalOutputDevice(17)

    match input("What would you like to do (Home (H) Abort (A)):").upper():
        case "H":
            home()
        case "A":
            print("Aborted")
        case _:
            print("Not a valid action")
