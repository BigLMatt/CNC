from gpiozero import DigitalOutputDevice
import time
import threading
from Position import load_position, save_position

def steps_to_distance(steps, pitch_um, pulses_per_rev):
    return steps/pulses_per_rev*pitch_um

def distance_to_steps(distance, pitch_um, pulses_per_rev):
    return int(distance/pitch_um*pulses_per_rev)

class StepperController:
    def __init__(self, pitch_um, pulses_per_rev, speed_rps, mode = "G90"):
        # GPIO
        self.enable = DigitalOutputDevice(17)
        self.direction = DigitalOutputDevice(27)
        self.pulse = DigitalOutputDevice(22)
        
        # External properties
        self.pitch_um = pitch_um
        self.pulses_per_rev = pulses_per_rev
        self.mode = mode
        self.speed_rps = speed_rps

        self.position_um = load_position()
        self.operation_mode = "IDLE"
        self.enable.off()
        
    def shutdown(self):
        self.enable.on()
        save_position(self.position_um)
    
    def start(self):
        self.thread = threading.Thread(target = self.move_steps, deamon = True)


    def step_delay(self):
        return 1/(2*self.pulses_per_rev*self.speed_rps) # devide by 2 because twice delay (between on and off) 
        # maybe change to m/s instead of rps

    def move_steps(self, target_steps):
        current_steps = distance_to_steps(self.position_um, self.pitch_um, self.pulses_per_rev)
        delta = int(target_steps - current_steps)
        if delta == 0: 
            return

        self.direction.value = delta < 0
        delay_s = self.step_delay()
        
        for step in range(abs(delta)):         # Make non-blocking!
            self.pulse.on()
            time.sleep(delay_s)
            self.pulse.off()
            time.sleep(delay_s)
        
        self.position_um = steps_to_distance(target_steps, self.pitch_um, self.pulses_per_rev)

    def abs_move(self, requested_position_um):
        self.operation_mode = "MOVE"
        target_steps = distance_to_steps(requested_position_um, self.pitch_um, self.pulses_per_rev)
        self.move_steps(target_steps)

    def rel_move(self, relative_position_um):
        self.operation_mode = "MOVE"
        self.abs_move(self.position_um + relative_position_um)

    def move(self, position_um):
        if self.mode == "G90":
            self.abs_move(position_um)
        elif self.mode == "G91":
            self.rel_move(position_um)
        else:
            print("Invalid movement mode")

    def move_speed_start(self, speed_rps, direction_forward):
        if direction_forward:
            self.direction.off()
        else:
            self.direction.on()

        self.speed_rps = speed_rps
        self.operation_mode = "SPEED"
    
    def move_speed_stop(self):
        self.operation_mode = "IDLE"
        self.speed_rps = 3   # Should revert speed to the default value because was overwritten before


if __name__ == "__main__":

    
    pitch_um = 5000  # 5000 µm per rotation
    pulses_per_rev = 1600  # Settings of stepper controller 8 microsteps
    mode = "G90"
    speed_rps = 3
    stepperController = StepperController(pitch_um, pulses_per_rev, speed_rps)

    try:
        requested_position_um = float(input("Give the requested position in mm: "))*1000 # Integrate with HAL (pos to µm)
        stepperController.move(requested_position_um)
        stepperController.shutdown()
    except ValueError:
        print("Please enter a number")

