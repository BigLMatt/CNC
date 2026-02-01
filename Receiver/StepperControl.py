from gpiozero import DigitalOutputDevice
import time
from Position import load_position, save_position

def steps_to_distance(steps, pitch, pulsesPerRev):
    return steps/pulsesPerRev*pitch

def distance_to_steps(distance, pitch, pulsesPerRev):
    return int(distance/pitch*pulsesPerRev)

# Speed in revolutions per second
def abs_move(currentPosition, requestedPosition, pitch, speed, pulsesPerRev):
    delaysec = 1/(2*pulsesPerRev*speed) # devide by 2 because twice delay (between on and off) maybe change to m/s speed
    
    requestedSteps = distance_to_steps(requestedPosition, pitch, pulsesPerRev)
    currentSteps = distance_to_steps(currentPosition, pitch, pulsesPerRev)

    if(requestedSteps > currentSteps):
        direction.off()
        while(requestedSteps > currentSteps):
            pulse.on()
            time.sleep(delaysec)
            pulse.off()
            time.sleep(delaysec)
            currentSteps += 1

    if(requestedSteps < currentSteps):
        direction.on()
        while(requestedSteps < currentSteps):
            pulse.on()
            time.sleep(delaysec)
            pulse.off()
            time.sleep(delaysec)
            currentSteps -= 1

    return steps_to_distance(currentSteps, pitch, pulsesPerRev)

def rel_move(currentPosition, relativePosition, pitch, speed, pulsesPerRev):
    return abs_move(currentPosition, currentPosition + relativePosition, pitch, speed, pulsesPerRev)

if __name__ == "__main__":
    enable = DigitalOutputDevice(17)
    direction = DigitalOutputDevice(27)
    pulse = DigitalOutputDevice(22)

    enable.off()

    pitch = 5000  # 5000 µm per rotation
    pulsesPerRev = 1600  # Settings of stepper controller 8 microsteps
    mode = "G90"

    currentPosition = load_position()
    try:
        requestedPosition = float(input("Give the requested position in mm: "))*1000 # Integrate with HAL (pos to µm)
    except ValueError:
        print("Please enter a number")
    
    if mode == "G90":
        currentPosition = abs_move(currentPosition, requestedPosition, pitch, 4, pulsesPerRev)
    elif mode == "G91":
        currentPosition = rel_move(currentPosition, requestedPosition, pitch, 4, pulserPerRev)
    save_position(currentPosition)
