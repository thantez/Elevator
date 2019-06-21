/*
* asansor.c
*
* Created: 6/11/2019 8:56:32 PM
* Author : HomePC
*/

// defines
#define F_CPU 8000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

// A structure to represent a stack
struct Stack {
	int top;
	unsigned capacity;
	int* array;
};

enum state {
	moving,
	stoped,
	slow_move,
	before_move
};

// functions declare
void init_ports();
unsigned int get_pressed_key();
unsigned int key_convert(unsigned int);
void adc_init();
int button(unsigned int pin);
int button_with_input(unsigned int pin, int input);
unsigned int floor_key_check();
unsigned int ir_key_check();
void show_floor(unsigned int);
struct Stack* createStack(unsigned capacity);
int isFull(struct Stack* stack);
int isEmpty(struct Stack* stack);
void push(struct Stack* stack, int item);
int pop(struct Stack* stack);
int peek(struct Stack* stack);
int search(struct Stack* stack, int item);


// interrupts declare
ISR(ADC_vect){
	int temp = (unsigned int)((ADCW * 4.88)/10);;
	if(temp > 35){
		PORTA |= (1 << 3);
	} else {
		PORTA &= 0xF7;
	}
	ADCSRA |= (1 << ADSC);
}



int main(void)
{
	unsigned int user_number, floor_key;
	
	// floor and destination
	unsigned int f = 0;
	int next_dest;
	
	int turn_flag = 1; // 1 = up, 0 = down
	
	int ir_pressed, temp_count = 0, motor = 0;
	
	//floors_stack
	struct Stack* floors_stack = createStack(100);
	
	enum state s = stoped;
	
	init_ports();
	adc_init();
	sei();
	while (1) {
		show_floor(f);
		
		
		floor_key = floor_key_check();
		user_number = key_convert(get_pressed_key());
		
		if(floor_key!=0xFF && floor_key != f) {
			push(floors_stack, floor_key);
		}
		
		if(user_number != 0xFF && user_number != f) {
			push(floors_stack, user_number);
		}
		
		switch(s){
			case stoped:{
				if(isEmpty(floors_stack)){
					continue;
				}
				
				next_dest = pop(floors_stack);
				
				if(next_dest == f) {
					continue;
				} else {
					s = before_move;
				}
				break;
			}
			
			case before_move:{
				
				if(!(PINB & (1 << PINB7))) {
					PORTA |= (1 << PINA5);
					continue;
				}
				
				PORTA &= ~(1<<PINA5);
				PORTB |= (1 << PINB3);
				_delay_ms(50);
				PORTB &= ~(1 << PINB3);
				
				// turn motor volt (B5, B6)
				if (next_dest > f) {
					turn_flag = 1;
						
					// turn motor for up
					motor = (1 << PINB5);
				} else {
					turn_flag = 0;
						
					// turn motor for down
					motor = (1 << PINB6);
				}
				
				PORTB |= motor;
				
				s = moving;
				break;
			}
			
			case moving:{
				ir_pressed = ir_key_check();
				if(ir_pressed == 0xFF || ir_pressed == f) {
					continue;
				} else if(ir_pressed == next_dest) {
					temp_count = 0;
					s = slow_move;
					_delay_ms(20);
				} else if(turn_flag && ir_pressed == f+1) {
					f++;
				} else if (!turn_flag && ir_pressed == f-1){
					f--;
				} else {
					show_floor(6);
					_delay_ms(20);
				}
				break;
			}
			
			case slow_move:{
				ir_pressed = ir_key_check();
				
				if(temp_count == 0){
					PORTB &= ~((1 << PINB6) | (1 << PINB5));
					temp_count = 1;
				} else {
					PORTB |= motor;
					temp_count = 0;
				}			
					
				if(ir_pressed == next_dest) {
					f = next_dest;
					// stop motor
					PORTB &= ~((1 << PINB5) | (1 << PINB6));
					
					// opening of door
					PORTB |= (1 << PINB4);
					PORTB &= ~(1 << PINB3);
					_delay_ms(50);
					PORTB &= ~((1 << PINB3) | (1 << PINB4));
					
					s = stoped;
				}
				break;
			}
			
			default:{
				PORTC = 0x09;
				break;
			}
		}
	}
	PORTC = 0x19;
	return 1;
}

void init_ports(){
	DDRA = (1 << DDA7) | (1 << DDA6) | (1 << DDA5) | (1 << DDA4) | (1 << DDA3) | (0 << DDA2) | (0 << DDA1) | (0 << DDA0);
	// State: Bit7=0 Bit6=0 Bit5=0 Bit4=0 Bit3=0 Bit2=T Bit1=T Bit0=T
	PORTA = (0 << PINA7) | (0 << PINA6) | (0 << PINA5) | (0 << PINA4) | (0 << PINA3) | (0 << PINA2) | (0 << PINA1) | (0 << PINA0);

	// Port B initialization
	// Function: Bit7=In Bit6=Out Bit5=Out Bit4=Out Bit3=Out Bit2=In Bit1=In Bit0=In
	DDRB = (0 << DDB7) | (1 << DDB6) | (1 << DDB5) | (1 << DDB4) | (1 << DDB3) | (0 << DDB2) | (0 << DDB1) | (0 << DDB0);
	// State: Bit7=T Bit6=0 Bit5=0 Bit4=0 Bit3=0 Bit2=T Bit1=T Bit0=T
	PORTB = (0 << PINB7) | (0 << PINB6) | (0 << PINB5) | (0 << PINB4) | (0 << PINB3) | (0 << PINB2) | (0 << PINB1) | (0 << PINB0);

	// Port C initialization
	// Function: Bit7=Out Bit6=Out Bit5=Out Bit4=Out Bit3=Out Bit2=Out Bit1=Out Bit0=Out
	DDRC = (1 << DDC7) | (1 << DDC6) | (1 << DDC5) | (1 << DDC4) | (1 << DDC3) | (1 << DDC2) | (1 << DDC1) | (1 << DDC0);
	// State: Bit7=0 Bit6=0 Bit5=0 Bit4=0 Bit3=0 Bit2=0 Bit1=0 Bit0=0
	PORTC = (0 << PINC7) | (0 << PINC6) | (0 << PINC5) | (0 << PINC4) | (0 << PINC3) | (0 << PINC2) | (0 << PINC1) | (0 << PINC0);

	// Port D initialization
	// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In
	DDRD = (0 << DDD7) | (0 << DDD6) | (0 << DDD5) | (0 << DDD4) | (0 << DDD3) | (0 << DDD2) | (0 << DDD1) | (0 << DDD0);
	// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T
	PORTD = (0 << PIND7) | (0 << PIND6) | (0 << PIND5) | (0 << PIND4) | (0 << PIND3) | (0 << PIND2) | (0 << PIND1) | (0 << PIND0);
}

unsigned int get_pressed_key()
{
	unsigned int r,c;

	PORTD|= 0X0F;

	for(c=0;c<3;c++)
	{
		DDRD&=~(0X7F);

		DDRD|=(0X40>>c);
		for(r=0;r<4;r++)
		{
			if(!(PIND & (0X08>>r)))
			{
				return (r*3+c);
			}
		}
	}

	return 0XFF;//Indicate No key pressed
}

unsigned int key_convert(unsigned int key){
	switch(key){
		case 11:
			return 0x00;
		case 0:
			return 0x01;
		case 1:
			return 0x02;
		case 2:
			return 0x03;
		case 3:
			return 0x04;
		default:
			return 0xFF;
	}
}

void adc_init(){
	ADMUX = 0x00 | (1 << MUX2) | (1 << REFS0);
	ADCSRA = 0x00 | (1 << ADEN) | (1 << ADSC) | (1 << ADIE) | (1 << ADATE);
}

int button(unsigned int pin){
	return button_with_input(pin, 1);
}

int button_with_input(unsigned int pin, int input){
	_delay_us(250);
	if(pin == input){
		while(pin == input){
			_delay_us(250);
		}
		return 1;
	}
	return 0;
}

unsigned int floor_key_check(){
	// A0 - A2
	unsigned int key = PINA & 0x07;
	if(key == 0 || key > 7){
		return 0xFF;
	}
	return ((unsigned int)key-1);
}

unsigned int ir_key_check(){
	// B0 - B2
	unsigned int key = PINB & 0x07;
	
	if(key == 0 || key > 7){
		return 0xFF;
	}
	return ((unsigned int)key-1);
}

void show_floor(unsigned int floor){
	PORTC = floor;
}


// function to create a stack of given capacity. It initializes size of
// stack as 0
struct Stack* createStack(unsigned capacity)
{
	struct Stack* stack = (struct Stack*)malloc(sizeof(struct Stack));
	stack->capacity = capacity;
	stack->top = -1;
	stack->array = (int*)malloc(stack->capacity * sizeof(int));
	return stack;
}

// Stack is full when top is equal to the last index
int isFull(struct Stack* stack)
{
	return stack->top == stack->capacity - 1;
}

// Stack is empty when top is equal to -1
int isEmpty(struct Stack* stack)
{
	return stack->top == -1;
}

// Function to add an item to stack.  It increases top by 1
void push(struct Stack* stack, int item)
{
	if (isFull(stack))
	return;
	stack->array[++stack->top] = item;
	printf("%d pushed to stack\n", item);
}

// Function to remove an item from stack.  It decreases top by 1
int pop(struct Stack* stack)
{
	if (isEmpty(stack))
	return INT_MIN;
	return stack->array[stack->top--];
}

// Function to return the top from stack without removing it
int peek(struct Stack* stack)
{
	if (isEmpty(stack))
	return INT_MIN;
	return stack->array[stack->top];
}

int search(struct Stack* stack, int item){
	for (int i = 0; i < stack->top; i++) {
		if(stack->array[i] == item)
			return 1;
	}
	return 0;
}