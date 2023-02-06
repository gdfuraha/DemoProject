# Elevator simulation

The simulation is developed in C and is of people using 2 lifts to arrive at the floor in an 8 storeyed building. People are expected to start at any floor going in any destination and exit at their destination. The simulation ends when all people have been served. Threads are used to represent each passenger and the elevator takes one person at a time. The program randomly generates the starting floor and destination for each passenger and the passenger waits for the available elevator to arrive and then they enter and exit when they reach their destination. The passenger only enters the elevator if there's enough room i.e: no other person in the elevator since it only carries one person.

## Development system

OS: Linux
Compiler: GCC

## How to run the Program

1. Clone the repository
2. Compile the file using GCC compiler: gcc elevator.c -o output file name
3. run the output file: ./output file name
