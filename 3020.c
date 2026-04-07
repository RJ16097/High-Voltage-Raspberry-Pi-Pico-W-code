#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"

#define PWM_PIN 16                                              //Define the PWM used to generate the signal to the MOSFET
#define UART_ID uart0                                           
#define Baud_Rate 115200                                        //Define baud rate for UART communication
#define TX_Pin 0                                                //Define Transmission pin for UART 
#define RX_Pin 1                                                //Define Transmission pin for UART 
const bool IsInverterDriver = true;                             //Defines if an inverting driver is used in the power supply

void UART_Setup(){ 
    uart_init(UART_ID, Baud_Rate);                              //Initialise UART for uart0
    gpio_set_function(TX_Pin, UART_FUNCSEL_NUM(uart0, 0));      //Set general pin for uart TX
    gpio_set_function(RX_Pin, UART_FUNCSEL_NUM(uart0, 1));      //Set general pin for uart RX
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);           //Set length of message 8-bits, number of bits to end message, and parity on or off
    uart_set_fifo_enabled(UART_ID, true);                       //Enable first in first out to prevent character loss
}

void PWM_MOSFET_Setup(){
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);                  //Set pin as a PWM output

}

int Calculate_Percent_Duty_Cycle(int voltage, int frequency) {  //Uses derived equation to calculate duty cycle to achieve a particular output voltage at a specfied frequency
    int duty_cycle_percent = 0; 
    double tmp = 0;
    double gradient = 0; double y_intercept = 0;

    switch (frequency) {                                        //Contains the gradient and y-intercept for equation at usable frequency
        case 10000:
            gradient = 0.787;
            y_intercept = 2.992;
            break;
        case 11000:
            gradient = 0.738;
            y_intercept = 3.148;
            break;
        case 12000:
            gradient = 0.672;
            y_intercept = 3;
            break;
        case 13000:
            gradient = 0.639;
            y_intercept = 2.946;
            break;
        case 14000:
            gradient = 0.618;
            y_intercept = 3.294;
            break;
        case 15000:
            gradient = 0.571;
            y_intercept = 3.294;
            break;
        case 16000:
            gradient = 0.505;
            y_intercept = 2.33;
            break;
        case 17000:
            gradient = 0.503;
            y_intercept = 3.045;
            break;
        case 18000:
            gradient = 0.457;
            y_intercept = 2.259;
            break;
        case 19000:
            gradient = 0.443;
            y_intercept = 2.244;
            break;
        case 20000:
            gradient = 0.403;
            y_intercept = 1.505;
            break;
        case 21000:
            gradient = 0.394;
            y_intercept = 2.161;
            break;
        case 22000:
            gradient = 0.372;
            y_intercept = 1.65;
            break;
        case 23000:
            gradient = 0.354;
            y_intercept = 1.442;
            break;
        case 24000:
            gradient = 0.339;
            y_intercept = 1.412;
            break;
        case 25000:
            gradient = 0.323;
            y_intercept = 1.483;
            break;
        case 26000:
            gradient = 0.357;
            y_intercept = 2.594;
            break;
        case 27000:
            gradient = 0.346;
            y_intercept = 2.436;
            break;
        case 28000:
            gradient = 0.353;
            y_intercept = 3.067;
            break;
        case 29000:
            gradient = 0.345;
            y_intercept = 2.714;
            break;
        case 30000:
            gradient = 0.327;
            y_intercept = 2.411;
            break;
        default:
            gradient = voltage;
            y_intercept = 0;
    }

    tmp = (voltage+y_intercept)/gradient;                       //The equation is of the form: Duty_Cycle = (voltage + y_intercept) / gradient
    duty_cycle_percent = tmp;

    if(duty_cycle_percent > 50 && frequency < 20000){           //Limit the value of duty cycle to remain in DCM mode
        duty_cycle_percent = 50;
    } else if(duty_cycle_percent > 60 && frequency >= 20000) {
        duty_cycle_percent = 60;
    }
    return duty_cycle_percent;                                  //Return duty cycle as a percentage
}

int Calculate_Number_of_Cycles_for_Frequency(int freq) {        //Calculate number of cycles to produce the frequency
    int wrap_point = (125000000)/freq;;                         //Calculate the number of cycle done by the Pico 8ns clock to achieve the request frequency
    wrap_point = wrap_point - 1;
    return wrap_point;
}

int Calc_Number_of_Cycle_for_Duty_Cycles(int wrap_point, int percent) {             //Calculate number of cycles to duty cycle
    int tmp = (wrap_point)*(percent);                                               //The number of cycles to achieve the frequency and multiplied by the duty cycle
    tmp = tmp/100;                                                                  //This gives the number of cycles for the PWM to be high

    if(IsInverterDriver == true) {                                                  //This determines the number of cycle based on if an inverting or non-inverting driver is used/
        return wrap_point - tmp;                                            
    } else{
        return tmp;
    }
}

int char_to_int(char value[]) {                                 //Convert a char array to a interger
    int num = 0;
    for(int i = 0; i < 6; i++) {
        if(value[i] == 'V' || value[i] == 'H') {                //The message has V and H to signify the voltage and frequency numbers
            num = num / 10;                                     //This is used to break the loop
            break;
        }
        num = num + (value[i] - '0');
        num = num * 10;
    }
    num = num;
    return num;
}

int main() {

    stdio_init_all();                                       
    UART_Setup();
    PWM_MOSFET_Setup();

    int Requested_Voltage = 0;                              //This stores the entered voltage from the desktop application after conversion
    int Requested_Frequency = 0;                            //This stores the entered frequency from the desktop application after conversion
    int i = 0, j = 0, count = 0;                            
    int Cycles_for_Frequency = 0;                                //This stores the number of cycles of 8ns clock to achieve requested frequency
    int Cycles_for_Duty_Cycle = 0;                          //This stores the number of cycles of 8ns clock to achieve a certain duty cycle
    int Required_Duty_Cycle = 0;                            //This stores the percentage duty cycle from the function Calculate_Percent_Duty_Cycle
    char Message_Voltage[6];                                           //This character array stores the voltage part of message from the desktop application 
    char Message_Frequency[6];                                           //This character array stores the frequency part of message from the desktop application 


    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    if(IsInverterDriver == true) {                          //If circuit uses inverting driver
        pwm_set_wrap(slice_num, 6249);                      //set frequency as 20kHz but can be any frequency
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 6429);    //set duty cycle of PWM as 100%
        pwm_set_enabled(slice_num, true);                   //Enables PWM
    } else {                                                //If circuit uses non-inverting driver
        pwm_set_wrap(slice_num, 6249);                      //set frequency as 20kHz but can be any frequency
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);       //set duty cycle of PWM as 0%
        pwm_set_enabled(slice_num, true);                   //Enables PWM
    }
   
    
    while(1) {
        while(uart_is_readable(UART_ID)) {                  //This checks if there is a message in the RX register
            char Character = uart_getc(UART_ID);            //This take one character from the message
                                                            //The message is the following form: 10V10H
            if (count == 0) {                               //count is zero means the voltage part of the message is being processed
                if (Character == 'V') {                     //'V' is used to signify the end of the voltage value
                    count++;
                    Message_Voltage[i] = Character;
                    Requested_Voltage = char_to_int(Message_Voltage);   //Converts the character voltage to an integer voltage value
                }
                else {
                    Message_Voltage[i] = Character;
                }
            } 
            else if (count == 1) {
                if (Character == 'H') {                     //'H' is used to signify the end of the frequency value
                    Message_Frequency[j] = Character;
                    Requested_Frequency = (1000*(char_to_int(Message_Frequency)));  //Converts the character frequency to an integer frequency value
                                                                                    //The value is multipled to go from kHz to Hz
                    break;
                }
                else {
                    Message_Frequency[j] = Character;
                }
                j++;
            }
            i++;
        }
       
        if(count == 1) {
            if(Requested_Voltage < 5 || Requested_Voltage > 20) {               //Checks that the voltage integer is out of range for 5kV to 20kV
                Requested_Voltage = 0;
                Requested_Frequency = 0;
            }
            if(Requested_Frequency < 10000 || Requested_Frequency > 300000) {   //Checks that the frequency integer is out of range for 10000Hz to 20000Hz
                Requested_Voltage = 0;
                Requested_Frequency = 0;
            }
            
            if(Requested_Voltage == 0 && Requested_Frequency == 0) {            //This works with the stop button in the desktop application to turn off the supply
                pwm_set_wrap(slice_num, 6249);
                pwm_set_chan_level(slice_num, PWM_CHAN_A, 6429);
            } else {                                                            //This calculates the number of cycle for frequency and duty cycle required.
                Cycles_for_Frequency = Calculate_Number_of_Cycles_for_Frequency(Requested_Frequency);
                pwm_set_wrap(slice_num, Cycles_for_Frequency);
                Required_Duty_Cycle = Calculate_Percent_Duty_Cycle(Requested_Voltage, Requested_Frequency);
                Cycles_for_Duty_Cycle = Calc_Number_of_Cycle_for_Duty_Cycles(Cycles_for_Frequency, Required_Duty_Cycle);
                pwm_set_chan_level(slice_num, PWM_CHAN_A, Cycles_for_Duty_Cycle); 
            }
                      
        }
        count = 0;
        i = 0;
        j = 0;

        sleep_ms(1000);
    }
    
    return 0;
}