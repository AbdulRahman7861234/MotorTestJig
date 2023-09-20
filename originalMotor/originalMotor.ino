//**********************************************************************
// Includes

#include <UstepperS32.h>

//**********************************************************************
// Defines

#define TEN_MM_IN_STEPS 77000 //100MM
#define ONETHOUSAND_SIX_HUNDRED 77000
#define EIGHT_HUNDRED 680000
#define THREE_HUNDRED 425000
#define Base (THREE_HUNDRED + EIGHT_HUNDRED + ONETHOUSAND_SIX_HUNDRED)


//**********************************************************************
// Typedefs

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

//**********************************************************************
// Variables

int Test = 0;
bool runLoop = false; // Initialize the flag to false 
static uint32_t platform_state_loops;
 
MOTOR_STATES motor_state = OFF;
MOTOR_STATES last_motor_state = OFF;
bool new_motor_state = true;
static uint32_t motor_state_loops;
UstepperS32 stepper;

//**********************************************************************
// Prototypes
 
uint16_t read_pin(uint32_t pin, uint16_t* debouce_timer);

//**********************************************************************
// Setup
 
void setup() {
  stepper.setup();
  stepper.checkOrientation(30.0);       //Check orientation of motor connector with +/- 30 microsteps movement
  Serial.begin(9600);

  Serial.print("platform.direction: ");
  Serial.println(platform.direction);

  Serial.print(Test);
 
  stepper.setMaxAcceleration(10);
  stepper.setMaxDeceleration(10);
  stepper.setMaxVelocity(1000);
  stepper.stop();
  stepper.setRPM(30);
 
  Serial.println("Setup complete.");
  pinMode(D2, INPUT_PULLDOWN);
  pinMode(D9, INPUT_PULLDOWN);
}

//**********************************************************************
// Main
 
void loop() {
  static uint32_t driveSteps;

  //Serial.print(Test);

  platform.motor_hall_effect = read_pin(D9, &platform.motor_hall_effect_debounce);
  platform.end_hall_effect = read_pin(D2, &platform.end_hall_effect_debounce);
 
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
        } else {
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
          platform.location = MOTOR_STOP;
        } 
      } else if (platform.direction == REVERSE) {
        if (platform.end_hall_effect == 0) {
            platform.location = END_STOP;
        }
      }

      if (!stepper.getMotorState()) {
        platform.location = TRAVELED;
        
      }
    break;
 
    case TRAVELED:
{
  if (_new_state)
  {
    Serial.println("TRAVELED");
    platform_state_loops = 0;
    stepper.stop();
    Test += 1;
    if (Test > 4)
    {
      Test = 1; // Reset to 1 if it goes beyond 4
    }
  }

  char cmd = Serial.read();

  if (cmd == '2')
  {
    runLoop = false; // Exit the loop
    platform.direction = FORWARD;
    platform.location = UNKNOWN;
  }

  if (cmd == '1')
  {
    runLoop = true; // Start the loop
    Test = 1;
  }

  if (runLoop)
  {
    Serial.print("Test: ");
    Serial.println(Test); // Print the current Test value
    if (Test == 1)
    {
      platform.direction = REVERSE;
      platform.location = TRAVELLING;
      stepper.setMaxAcceleration(100);
      stepper.setMaxDeceleration(100);
      stepper.setMaxVelocity(3000);
      stepper.moveSteps(-ONETHOUSAND_SIX_HUNDRED);
    }
    else if (Test == 2)
    {
      platform.direction = REVERSE;
      platform.location = TRAVELLING;
      stepper.setMaxAcceleration(100);
      stepper.setMaxDeceleration(100);
      stepper.setMaxVelocity(3000);
      stepper.moveSteps(-EIGHT_HUNDRED);
    }
    else if (Test == 3)
    {
      platform.direction = REVERSE;
      platform.location = TRAVELLING;
      stepper.setMaxAcceleration(100);
      stepper.setMaxDeceleration(100);
      stepper.setMaxVelocity(3000);
      stepper.moveSteps(-THREE_HUNDRED);
    }
    else if (Test == 4)
    {
      platform.direction = FORWARD;
      platform.location = TRAVELLING;
      stepper.setMaxAcceleration(100);
      stepper.setMaxDeceleration(100);
      stepper.setMaxVelocity(3000);
      stepper.moveSteps(Base);
    }

    if (Test > 4)
    {
      Test = 1; // Reset to 1 if it goes beyond 4
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

  delay(10);
}

//**********************************************************************
// Function

uint16_t read_pin(uint32_t pin, uint16_t* debounce_timer) {
  uint16_t _val = digitalRead(pin);
  if (_val == 0) {
    *debounce_timer += 1;

    if (*debounce_timer > 2) {
      return 0;
    }
  } else {
    *debounce_timer = 0;
  }

  return 1;
}
