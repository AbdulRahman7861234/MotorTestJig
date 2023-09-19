#include <UstepperS32.h>

#define TEN_MM_IN_STEPS 77000 //100MM
#define ONETHOUSAND_SIX_HUNDRED 77000
#define EIGHT_HUNDRED 680000
#define THREE_HUNDRED 425000
#define Base (THREE_HUNDRED + EIGHT_HUNDRED + ONETHOUSAND_SIX_HUNDRED)
bool runLoop = false; // Initialize the flag to false 

typedef enum MOTOR_STATES {
  OFF,
  RAMPING_UP,
  MOVING,
  RAMPING_DOWN,
  MOVING_REVERSE
};
 
typedef enum LOCATIONS {
  UNKNOWN,
  TRAVELLING,
  TRAVELED,
  MOTOR_STOP,
  END_STOP
};
 
typedef enum DIRECTIONS {
  FORWARD,
  REVERSE
};
 
typedef struct Platform_t {
  uint8_t location;
  uint8_t last_location;
  uint8_t steps_to_move;
  uint8_t steps_moved;
  uint16_t motor_hall_effect;
  uint16_t end_hall_effect;
  uint16_t motor_hall_effect_debounce;
  uint16_t end_hall_effect_debounce;
  uint8_t direction;
} Platform_t;
 
Platform_t platform = {
  .location = UNKNOWN,
  .last_location = TRAVELED,
  .direction = FORWARD
};
 
static uint32_t platform_state_loops;
 
MOTOR_STATES motor_state = OFF;
MOTOR_STATES last_motor_state = OFF;
bool new_motor_state = true;
static uint32_t motor_state_loops;
UstepperS32 stepper;
 
void motor_service(void);
uint16_t read_pin(uint32_t pin);
 
void setup() {
  stepper.setup();
  stepper.checkOrientation(30.0);       //Check orientation of motor connector with +/- 30 microsteps movement
  Serial.begin(9600);

  Serial.print("platform.direction: ");
  Serial.println(platform.direction);
 
  stepper.setMaxAcceleration(10);
  stepper.setMaxDeceleration(10);
  stepper.setMaxVelocity(1000);
  stepper.stop();
  stepper.setRPM(30);
 
  Serial.println("Setup complete.");
  pinMode(D2, INPUT_PULLDOWN);
  pinMode(D9, INPUT_PULLDOWN);
}
 
void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println("loop");
 
  static uint32_t _last_motor, _last_end;
  _last_motor = platform.motor_hall_effect;
  _last_end = platform.end_hall_effect;
  platform.motor_hall_effect = read_pin(D9);
  platform.end_hall_effect = read_pin(D2);
 
  if (_last_motor != platform.motor_hall_effect) {
        Serial.println("Motor Sensor");
        //platform.location = MOTOR_STOP;
        //stepper.stop();
       
  }
 
  if (_last_end != platform.end_hall_effect) {
        Serial.println("End Sensor");
        //platform.location = END_STOP;
        //stepper.stop();
  }
 
  bool _new_state = false;
 
  if (platform.location != platform.last_location) {
    platform.last_location = platform.location;
    _new_state = true;
  }
 
  switch (platform.location) {
    case UNKNOWN:
      if (_new_state) {
        Serial.println("Unknown position");
 
        if (platform.motor_hall_effect == 1 ) {
          platform.location = TRAVELLING;
          platform.direction = FORWARD;
          stepper.setRPM(30);                  
          }
        //if(platform.end_hall_effect == 1){
          //platform.location = TRAVELLING;
          //platform.direction = REVERSE;
          //stepper.setRPM(-30);                    Change this to change direction when you first start program

        //}
         else {
          platform.location = MOTOR_STOP;
        }
      }
    break;
 
    case TRAVELLING:
      if (_new_state) {
          Serial.println("TRAVELLING position");
      }
 
      if (platform.direction == FORWARD) {
        if (platform.motor_hall_effect == 0) {
          if (platform.motor_hall_effect_debounce++ > 2) {
            platform.location = MOTOR_STOP;
            
          }
        } else {
          platform.motor_hall_effect_debounce = 0;
        }
      } else if (platform.direction == REVERSE) {
        if (platform.end_hall_effect == 0) {
          if (platform.end_hall_effect_debounce++ > 2) {
            platform.location = END_STOP;
          }
        } else {
          platform.end_hall_effect_debounce = 0;
        }
      }
 
      if (!stepper.getMotorState()) {
        platform.location = TRAVELED;
      }
    break;
 
    case TRAVELED:
    {
      if (_new_state) {
        Serial.println("TRAVELED");
        platform_state_loops = 0;
        stepper.stop();
      }
 
      char cmd = Serial.read();

      if (cmd == '2') {
    runLoop = false; // Exit the loop
    platform.direction = FORWARD;
    platform.location = UNKNOWN;
  }

  if (cmd == '1') {
    runLoop = true; // Enter the loop
    platform.direction = REVERSE;
    //platform.location = TRAVELLING;
  }

  // Inside the loop controlled by runLoop
  while (runLoop == true) {
    platform.location = TRAVELLING;
    stepper.setMaxAcceleration(100);
    stepper.setMaxDeceleration(100);
    stepper.setMaxVelocity(3000);

    if (platform.direction == FORWARD) {
      Serial.println("Going forward now");
      stepper.moveSteps(Base);
      
      delay(45000);
     // stepper.moveSteps(-Base);
      Serial.println("Return");
      platform.direction = REVERSE;
    } else if (platform.direction == REVERSE) {
      Serial.println("Going backwards now");
      stepper.moveSteps(-ONETHOUSAND_SIX_HUNDRED);
      
      Serial.println("1600");
      delay(45000);
      stepper.moveSteps(-EIGHT_HUNDRED);
      Serial.println("800");
      delay(45000);
      stepper.moveSteps(-THREE_HUNDRED);
      Serial.println("300");
      delay(45000);
      platform.direction = FORWARD;
    }

    // Check for the '2' command to exit the loop
    if (Serial.available()) {
      char nextCmd = Serial.read();
      if (nextCmd == '2') {
        runLoop = false; // Exit the loop
        platform.direction = FORWARD;
        platform.location = UNKNOWN;
        break; // Add this line to exit the loop immediately
      }
    }
}       
        
      
    }
    break;
    case MOTOR_STOP:
      if (_new_state) {
        Serial.println("MOTOR_STOP position");
        stepper.stop();
        platform.direction = REVERSE;
        platform_state_loops = 0;
      }

      if (platform_state_loops++ == 100) {
        platform.location = TRAVELED;
      }
    break;

    case END_STOP:
      if (_new_state) {
        Serial.println("END_STOP position");
        stepper.stop();
        platform.direction = FORWARD;
        platform_state_loops = 0;
      }

      if (platform_state_loops++ == 100) {
        platform.location = TRAVELED;
      }
    break;
  }

  //motor_service();

  delay(10);
}

void motor_service(void) {
  if (motor_state != last_motor_state) {
    new_motor_state = true;
    last_motor_state = motor_state;
    Serial.println("Changing state");
  }

  switch (motor_state) {
    case OFF:
    {
      while(!Serial.available());
      char cmd = Serial.read();

      if(cmd == '1')                      //Run continous clockwise
      {
        motor_state = RAMPING_UP;
      }
    }
      break;

    case RAMPING_UP:
      if (new_motor_state) {
        stepper.setRPM(100);
        stepper.moveSteps(50000);
        motor_state_loops = 0;
      }

      if (motor_state_loops++ == 1)
        motor_state = MOVING;

      break;

    case MOVING:
    {
      uint16_t _motor_hall_effect = read_pin(D9);
      uint16_t _end_hall_effect = read_pin(D2);

      if (_motor_hall_effect == 0) {
        motor_state = RAMPING_DOWN;
      }

      if (_end_hall_effect == 0) {
        motor_state = RAMPING_DOWN;
      }
    }
      break;

    case RAMPING_DOWN:
      if (new_motor_state) {
        stepper.stop();
        motor_state_loops = 0;
      }

      if (motor_state_loops++ == 1) {
        motor_state = OFF;
      }

      break;

    case MOVING_REVERSE:

      break;
  }

  new_motor_state = false;
}

uint16_t read_pin(uint32_t pin) {
  char str[20] = {0};
  //pinMode(pin, INPUT_PULLUP);
  uint16_t _val = digitalRead(pin);
  //sprintf(str, "Pin: %d = %d", pin, _val);
  //Serial.println(str);

  return _val;
}
