from gpiozero import DigitalOutputDevice
import time
import threading
from Position import load_position, save_position


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

        self.target_steps = 0
        self.current_steps = self.distance_to_steps(load_position())
        self.operation_mode = "IDLE"
        self.move_running = True
        self.enable.off()
        self.thread = threading.Thread(target = self.move_steps, daemon = True)
        self.thread.start()

    def shutdown(self):
        self.move_running = False
        self.thread.join()
        self.enable.on()
        save_position(self.steps_to_distance(self.current_steps))

    def step_delay(self):
        return 1/(2*self.pulses_per_rev*self.speed_rps) # devide by 2 because twice delay (between on and off) 
        # maybe change to m/s instead of rps

    def steps_to_distance(self, steps):
        return steps/self.pulses_per_rev*self.pitch_um

    def distance_to_steps(self, distance):
        return int(round(distance/self.pitch_um*self.pulses_per_rev))

    def move_steps(self):
        while self.move_running:
            if self.operation_mode == "IDLE":
                time.sleep(0.001)
                continue

            delay_s = self.step_delay()
            
            if self.operation_mode == "POS": 
                if self.current_steps == self.target_steps:
                    self.operation_mode = "IDLE"
                    save_position(self.steps_to_distance(self.current_steps))   # Change later, saving too often now
                    continue
                
                forward = self.target_steps > self.current_steps
                self.direction.value = forward

                self.do_step(delay_s) 
                
                self.current_steps += (1 if forward else -1)

            elif self.operation_mode == "SPEED":
                self.do_step(delay_s)

    def do_step(self, delay_s):
        self.pulse.on()
        time.sleep(delay_s)
        self.pulse.off()
        time.sleep(delay_s)

    def abs_move(self, requested_position_um):
        self.operation_mode = "POS"
        self.target_steps = self.distance_to_steps(requested_position_um)

    def rel_move(self, relative_position_um):
        self.operation_mode = "POS"
        self.target_steps = self.distance_to_steps(relative_position_um) + self.current_steps

    def move(self, position_um):
        if self.mode == "G90":
            self.abs_move(position_um)
        elif self.mode == "G91":
            self.rel_move(position_um)
        else:
            print("Invalid movement mode")

    def move_speed_start(self, speed_rps, direction_forward = True):
        if(speed_rps == 0):
            self.move_speed_stop()
            return
        self.direction.value = direction_forward
        self.speed_rps = speed_rps
        self.operation_mode = "SPEED"
    
    def move_speed_stop(self):
        if self.operation_mode == "SPEED":
            self.operation_mode = "IDLE"

            self.target_steps = self.current_steps
            save_position(self.steps_to_distance(self.current_steps))
        
        return self.steps_to_distance(self.current_steps)
        

if __name__ == "__main__":

    
    pitch_um = 5000  # 5000 µm per rotation
    pulses_per_rev = 1600  # Settings of stepper controller 8 microsteps
    mode = "G90"
    speed_rps = 3
    stepperController = StepperController(pitch_um, pulses_per_rev, speed_rps)

    try:
        requested_position_um = float(input("Give the requested position in mm: "))*1000 # Integrate with HAL (pos to µm)
        stepperController.move(requested_position_um)
        while(stepperController.operation_mode == "POS"):
            time.sleep(1)
        stepperController.shutdown()
    except ValueError:
        print("Please enter a number")

