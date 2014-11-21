#define MOVING_STEPS 10
#define LEARN_STEPS 500
#define BURN_IN 100
#define VARIANCE 1000
#include <Wire.h>

int step = 0, burn_in = BURN_IN, zeros_learn_steps = LEARN_STEPS;
int led = 13, transistor = 3;
byte addr_map[6] = {0x28, 0x2A, 0x2C, 0x29, 0x2B, 0x2D};
float zeros_axis[3] = {0.0, 0.0, 0.0};
unsigned long zeros_time, run_time;
const int L3GD20_ADDR = 0xD4 >> 1;

unsigned int readRegister(byte reg)
{
    Wire.beginTransmission(L3GD20_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    
    Wire.requestFrom(L3GD20_ADDR, 1, false);
    Wire.endTransmission(true);
    return Wire.read();
}

void writeRegister(byte reg, byte data)
{
    Wire.beginTransmission(L3GD20_ADDR);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission(false);
}

void setup()
{
    Wire.begin();
    Serial.begin(9600);
    pinMode(led, OUTPUT);
    pinMode(transistor, OUTPUT);
    
    int res = readRegister(0x0F);
    if(res == 0xD4){ Serial.println("Success."); } // Connection success.
        else { Serial.println(res, HEX); } // Connection failed.
    
    writeRegister(0x20, 0x0F);
    Serial.print("Burn ining.");
}

void loop()
{
    float axis[3];
    String xyz_value = "",  zeros_xyz_value = "";
    int moving_steps[3][MOVING_STEPS];
    int h, l;
    
    if(zeros_learn_steps == 0)
    {// scanning block.
        for(int local_axis=0; local_axis<3; local_axis++)
        {
            l = readRegister(addr_map[local_axis]);
            h = readRegister(addr_map[local_axis + 3]);
            moving_steps[local_axis][step] = zeros_axis[local_axis] - float(h << 8 | l);
            
            axis[local_axis] = 0.0;
            
            for(int local_step=0; local_step<MOVING_STEPS; local_step++)
            {
                axis[local_axis] += moving_steps[local_axis][local_step] / float(MOVING_STEPS);
            }
            
            xyz_value += String(int(axis[local_axis]));
            if(local_axis < 2) xyz_value += ",";
                //else xyz_value += "!";
        }
        
        /*** Detector process -> ***/
        
        /* 下向くとLED点灯 */
        /*if(zeros_axis[0] + VARIANCE < axis[0]) digitalWrite(led, HIGH);
            else digitalWrite(led, LOW);
        */
        
        /* 上向くとLED点灯 */
        if(axis[0] < -VARIANCE)
        {
            digitalWrite(led, LOW);
            digitalWrite(transistor, HIGH);
        }
        else
        {
            digitalWrite(led, HIGH);
            digitalWrite(transistor, LOW);
        }
        
        /* 右旋回でLED点灯 */
        /*if(zeros_axis[2] + VARIANCE < axis[2]) digitalWrite(led, HIGH);
            else digitalWrite(led, LOW);
        */
        
        /* 左旋回でLED点灯 */
        /*if(axis[2] < zeros_axis[2] - VARIANCE) digitalWrite(led, HIGH);
            else digitalWrite(led, LOW);
        */
            
        /*** <- Detector process ***/
        run_time = (micros() - zeros_time);
        Serial.println(String(run_time) + "," + xyz_value);
    }
    else
    {// learning block.
        if(burn_in == 0)
        {
            //if(zeros_learn_steps % 10 == 0) Serial.print('.');
            for(int local_axis=0; local_axis<3; local_axis++)
            {
                l = readRegister(addr_map[local_axis]);
                h = readRegister(addr_map[local_axis + 3]);
                zeros_axis[local_axis] += float(h << 8 | l) / float(LEARN_STEPS);
            }
            
            zeros_learn_steps -= 1;
            if(zeros_learn_steps == 0)
            {
                //Serial.println("\nLearning Finish.");
                
                for(int local_axis=0; local_axis<3; local_axis++)
                {
                    zeros_xyz_value += String(int(zeros_axis[local_axis]));
                    if(local_axis < 2) zeros_xyz_value += ",";
                }
                
                //Serial.println("zeros_axis: " + zeros_xyz_value);
                
                for(int x=0; x<5; x++)
                {
                    digitalWrite(led, HIGH);
                    delay(100);
                    digitalWrite(led, LOW);
                    delay(100);
                }
                zeros_time = micros();
            }
        }
        else
        {
            //if(burn_in % 10 == 0) Serial.print('.');
            for(int local_axis=0; local_axis<3; local_axis++)
            {
                l = readRegister(addr_map[local_axis]);
                h = readRegister(addr_map[local_axis + 3]);
            }
            burn_in -= 1;
            if(burn_in == 0)
            {
                //Serial.print('\n');
                //Serial.print("Learning.");
            }
        }
    }
    
    if(step < MOVING_STEPS - 1) step += 1;
            else step = 0;
}
