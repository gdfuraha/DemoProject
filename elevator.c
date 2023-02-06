#include <stdio.h>    // allow us to input and print from the program
#include <stdlib.h>  // supports memory allocation, and process control among others
#include <unistd.h>  // allows us to use functions like sleep()
#include <pthread.h> // supports use of threads
#include <stdint.h>  // allows us to manipulate integers
 

#define number_of_elevators 2  // setting number of elevators
#define number_of_users 5      // setting number of users
 
int  number_of_storeys = 8; // a variable to hold the maximum number of floors
int tours_per_user = 1; // number of trip each user is allowed to take.
 
static int threads_controller=0; // variable which controls threads i.e: altering its value will stop the threads from running.
 
 
 
 // Defining elevator structure with its variables.
 
struct Elevator { 
	int floor;	//  a variable to track on which floor the elevator drops the user
	int open;	// a variable to track when the elevator opens or closes
	int users;	// a variable for  users(passangers) in the elevator
	int rounds;	// a variable to track the number of trips made.
	int barrier1,barrier2; //barriers for controlling the movement of users when elevator opens	
	int current_floor; // a varibale to track which floor the elevator is.
	int direction; // a variable to track which direction the elevator r is going
	int elevator_holding; // a variable to track users in the elevator
	int pickpoint; // a variable for user floor choice.
	int id; // elevator id
	int to,from; //  variable to track where the elevator is going.
	enum {arrived=1, open=2, closed=3} state;// variables to manipulate which state the elevator is in.
	pthread_mutex_t lock; 
	pthread_barrier_t elevatorBlock1, elevatorBlock2, barrier;	
}elevators[number_of_elevators];
 
 // defining the user structure with its variables.
static struct User {
    int id;	// the identification for users
    int depature_floor;	// variable to hold floor of users before onboarding the elevator
    int destination_floor; // varaible to hold floor where users are going.
    enum {user_waiting, user_entered, user_exited} state; // variable to track whether user entered, exited or waiting.
} users[number_of_users];
 
 //Program to choose an available elevator for a user to use
  int choose_elevator(int passenger_pickup, int passenger_deposit){
 
    while(1) {  //Loop that will run until an elevator with space is found
 
	for(int i = 0; i < number_of_elevators; i++) {		 // looping through all elevators
	    pthread_mutex_lock((void*)&(elevators[i].lock));	// locking the critical point in order to check for one elevator at a time
	    if(elevators[i].pickpoint == -1){ 			// checking if the elevator under iteration is not headed to another floor
    		elevators[i].pickpoint = passenger_pickup;	// updating tthe floor elevator is picking the user
		elevators[i].to = passenger_deposit;		// updating tthe floor elevator is taking the user
		elevators[i].from = passenger_pickup;		// updating where the elvator should move to to pick user
	    	pthread_mutex_unlock((void*)&(elevators[i].lock)); // unlocking the critical point
		return i; 					   // returning the index of the elevator to pick up user
	    }
 
	    pthread_mutex_unlock((void*)&(elevators[i].lock));	// unlocking the critical point
	}
    }
 
}
 
 // a method for onboarding user on the elevator
void user_call(int user, int depature_floor, int destination_floor)
{	
    int elevator_no = choose_elevator(depature_floor, destination_floor); // calling function to choose which elevator will pick up user
    struct Elevator *e = &elevators[elevator_no];			// creating a pointer to the choosen elevator
 
    // wait for the elevator to arrive at our origin floor, then get in
    int waiting = 1;
    while(waiting) {
	pthread_barrier_wait((void*)&(e->elevatorBlock2)); //prohibits user from boarding the elevator before it gets to their floor or if 
	// it is fully occupied
        pthread_mutex_lock((void*)&(e->lock)); // locking critical point
	if(e->current_floor == depature_floor && e->state == open && e->elevator_holding<5) {// allowing user to eneter elevator after checking that the elevator arrived
	     printf("User %d entered elevator %d at floor %d going to floor %d.\n", users[user].id, e->id, e->floor, users[user].destination_floor);  // printing that user entered elevator alongside their destination
	    users[user].state = user_entered; // update the state of user to entered
	    sleep(2);   		      // sleeping program for two seconds
            e->elevator_holding++;	      // increment number of users in elevator
    	    e->pickpoint = destination_floor;  // update the elevator destination to user choice
	    waiting=0;                         // changing waiting status
        }
	pthread_mutex_unlock((void*)&(e->lock)); // unlocking critical point
    }
    pthread_barrier_wait((void*)&(e->elevatorBlock1));  // making sure elevator closes before moving
 
    // user waits for the elevator to reach their destination floor before getting out
    int riding=1;
    while(riding) {
	pthread_mutex_lock((void*)&(e->lock));   // locking critical point
        if(e->current_floor == destination_floor && e->state == open) {  // checking if user arrived destination and elevator is open
        printf("Passenger %d left elevator %d at floor %d.\n", users[user].id, e->id, e->floor); 
	    e->users--;   // decrementing number of users in the elevator
	    e->rounds++;  // incrementing the number of rounds the elevator took
	    users[user].state = user_exited;  // changing user state
 
	    sleep(2); 			      // sleeping program 2 seconds for user to move out
            e->elevator_holding--;            // decrementing number of users in elevator
            e->barrier1 = 0;      // setting variables to make sure that user gets out while elevator is open and closes right after
	    e->barrier2 = 0; 
	    riding=0;	         // changing user riding status
	    pthread_barrier_wait((void*)&(e->barrier)); // WAITS FOR THE PASSENGER TO LEAVE THE ELEVATOR BEFORE COMPLETING PASSENGER TRIP
        }
    	pthread_mutex_unlock((void*)&(e->lock));  // unlocking critical point
    } 
}
 
// function to control elevator operations passed to it are the elevator index, starting floor, and pointer to another function it calls
void run_elevator(int elevator, int at_floor, 
        void(*change_position)(int, int)) {
 
    // ckecking if the elevator has arrived it destination floor before opening it
    if(elevators[elevator].state == arrived && elevators[elevator].pickpoint == elevators[elevator].current_floor) {
 
	printf("Elevator %d opening at floor %d\n",elevator,elevators[elevator].floor);	
    	elevators[elevator].open=1;         // changing elevator open status
        elevators[elevator].state = open;  // changing the state of elevator
	if(elevators[elevator].barrier2 == 0) {
	    pthread_barrier_wait((void*)&(elevators[elevator].elevatorBlock2)); // waits for when the elevator arrives and is unoccupied
	    elevators[elevator].barrier2 = 1;
	}
    }
 
    // ckecking if the elevator has arrived its destination floor and is open before closing it
    if(elevators[elevator].state == open && elevators[elevator].pickpoint == elevators[elevator].current_floor) {
	if(elevators[elevator].barrier1 == 0) {	    
	    pthread_barrier_wait((void*)&(elevators[elevator].elevatorBlock1));  // blocks the elevator from taking off before door closes
	    elevators[elevator].barrier1 = 1;
	}
 
	//checks if elevator has took its assigned user to their destination
	if(elevators[elevator].pickpoint == elevators[elevator].to && elevators[elevator].current_floor == elevators[elevator].to) {
	   pthread_barrier_wait((void*)&(elevators[elevator].barrier));  // WAITS FOR THE PASSENGER TO LEAVE THE ELEVATOR BEFORE COMPLETING PASSENGER TRIP
	   elevators[elevator].pickpoint = -1;  // setting that elevator has no more users to pick up
	}
 
	printf("Elevator %d closing at floor %d\n",elevator,elevators[elevator].floor); 
 
    	elevators[elevator].open=0; // changing elevator open status
 
        elevators[elevator].state=closed;  // changing elevator status to closed
    }
 
    else {  // When elevator hasn't arrieved their destination yet
 
 	// checks if elevator has people to pickup and dropoff
        if(elevators[elevator].pickpoint != -1) {
        	// checks if elevator destination is above the current floor
		if(elevators[elevator].pickpoint > elevators[elevator].current_floor) {
		    elevators[elevator].direction = 1; // set the elevator moving direction to up
		    elevators[elevator].current_floor=at_floor+ elevators[elevator].direction; // increments the elevator's current floor by 1
		    change_position(elevator, elevators[elevator].direction);  // call function to change elevator position
		}
		// checks if elevator destination is below the current floor
		else if(elevators[elevator].pickpoint < elevators[elevator].current_floor) {
		    elevators[elevator].direction = -1; // set the elevator moving direction to down
		    elevators[elevator].current_floor=at_floor+ elevators[elevator].direction; // decrements the elevator's current floor by 1
		    change_position(elevator, elevators[elevator].direction);  // call function to change elevator position
		}
		else {  // if the elevator current floor is not less or greater than the destination it means that it has arrived
		    elevators[elevator].state = arrived;  // change elevator status
		}
	}
	else { 
		// checking if the elevator is at the bottom or the top of the building
		if(at_floor == 0 || at_floor == number_of_storeys-1) 
			elevators[elevator].direction *= -1;  // changing its direction
        	change_position(elevator, elevators[elevator].direction); // call function to change elevator position
        	elevators[elevator].current_floor=at_floor+ elevators[elevator].direction; // incrementing or decrementing current floor by 1 depending on its direction
	}
    }
}
 

// function that changes the elevator floor position whether moving up or down
void elevator_change_position(int elevator, int direction) { 
	// display to indicate the direction of the elevator and the floor it is on
    printf("Elevator %d moving %s from %d\n",elevator,(direction==-1?"down":"up"),elevators[elevator].floor);  
    // sleep to stimulate travel time
    sleep(2);
    // increments or decrements the floor the elevator is on as it moves
    elevators[elevator].floor+=direction;
}
 
 
// function that starts the user and user call 
void* generate_user(void *arg) {
	// gets the id of the user being generated
    int user=(uintptr_t)arg; 
    
    // diplays to indicate that the user was generated
    printf("Generated user %d\n", user);
    
    // initialize user details including the floor the are on and their id 
    users[user].depature_floor = random() % number_of_storeys; 
    users[user].id = user;
    int rounds = tours_per_user;
    
    // iterates to put in a request for elevator
    while(!threads_controller && rounds-- > 0) {
    	// chooses a random floor to head to
        users[user].destination_floor = random() % number_of_storeys;
        
        // displays to indicate the user's request
        printf("User %d requesting to move from %d to %d\n", user, users[user].depature_floor, users[user].destination_floor); 
        // sets the user state to waiting since the user hasn't found available elevator
        users[user].state=user_waiting;
        
        //calls user_call function to execute the user's request
        user_call(user, users[user].depature_floor, users[user].destination_floor); 
 
    }
}


// the function that starts the elevator 
void* initialize_elevator(void *arg) {
	// gets the id of the elevator getting initialized
        int elevator = (uintptr_t)arg;  
    	// initialize the mutex 
    	pthread_mutex_init((void*)&(elevators[elevator].lock), NULL);
    	
    	// sets the initial state of the elevator
	elevators[elevator].current_floor=0;
	elevators[elevator].direction=-1;
	elevators[elevator].elevator_holding=0;
	elevators[elevator].pickpoint = -1;
	elevators[elevator].id = elevator;
	elevators[elevator].state=closed;  
    	elevators[elevator].rounds = 0;
    	elevators[elevator].floor = 0;
	// initialize the barriers 
	pthread_barrier_init((void *)&(elevators[elevator].elevatorBlock1), NULL, 2);
	pthread_barrier_init((void *)&(elevators[elevator].elevatorBlock2), NULL , 2);
	pthread_barrier_init((void*)&(elevators[elevator].barrier), NULL, 2);
    
    // display to indicate that the elevator was initialized
    printf("Starting elevator %d\n", elevator);
    
    // iterates to run the elevator so that it starts taking people to their destinations 
    while(!threads_controller) { 
        run_elevator(elevator, elevators[elevator].floor,elevator_change_position);
    }
 
 	// destroys the mutex and all barriers
    pthread_mutex_destroy(&elevators[elevator].lock);
    pthread_barrier_destroy(&elevators[elevator].elevatorBlock1);
    pthread_barrier_destroy(&elevators[elevator].elevatorBlock2);
    pthread_barrier_destroy(&elevators[elevator].barrier);
 
} 


// main function that executes the whole program 
int main(int argc, char** argv) {  
 	// creates an array of initialized user threads 
    pthread_t user_t[number_of_users];
    
    // iterates to create the user threads
    for(uintptr_t i=0;i<number_of_users;i++)  {
    	// creates and checks if the threads were successfully created
        if (pthread_create(&user_t[i],NULL, generate_user,(void*)i) != 0){
        	printf("Failed to create the thread.");
        }	
    } 
    
 	// creates an array of initialized elevator threads
    pthread_t elevator_t[number_of_elevators];
    
    // iterates to create the elevator threads
    for(uintptr_t i=0;i<number_of_elevators;i++) {
        // creates and checks if the threads were successfully created
        
        if (pthread_create(&elevator_t[i],NULL, initialize_elevator,(void*)i) != 0){
        	printf("Failed to create the thread.");
        }	
    }
    
    // iterates to join the user threads to join the main thread and will wait for all thread to finish running
    for(int i=0;i<number_of_users;i++) {
    	// joins and checks if the thread joined successfully
        if(pthread_join(user_t[i],NULL) != 0){
        	printf("Failed to join thread.");
        }
    }
    
    // sets the threads_controller 
    threads_controller=1;
     
    // iterates to join the elevator threads to join the main thread and will wait for all threads to finish running
    for(int i=0;i<number_of_elevators;i++) {
    	// joins and checks if the thread joined successfully
        if(pthread_join(elevator_t[i],NULL) != 0){
        	printf("Failed to join thread.");
        }
    }
 	
 	// display to inidicate the end of execution of the whole program
    printf("All %d Users arrived at their destinations\n",number_of_users); 
}
