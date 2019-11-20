#include "PC_FileIO.c"

bool map[10][10] =
	{{0,0,0,0,0,0,0,0,0,0},
	 {0,1,1,1,0,0,1,1,1,0},
	 {0,1,1,1,0,0,1,1,1,0},
	 {0,1,1,1,0,0,1,1,1,0},
	 {0,1,1,1,1,1,1,1,1,0},
	 {0,1,1,1,1,1,1,0,1,0},
	 {0,1,1,1,1,1,1,0,1,0},
	 {0,1,0,0,0,0,0,0,1,0},
	 {0,1,1,1,1,1,1,1,1,0},
	 {0,0,0,0,0,0,0,0,0,0}};

int paths[10][10] = {{0}};

typedef struct{
	int pos[2];
	int dir;
	bool interrupt;
}state;

void initialize_paths(){
	for (int i = 0; i < 10; i++){
		for (int a = 0; a < 10; a++){
			paths[i][a] = 0;
		}
	}
}

int min(int a, int b){
	if (a > b)
		return b;
	return a;
}

int max(int a, int b){
	if (a > b)
		return a;
	return b;
}

void drive(float distance, state* cur_state, bool canInterrupt){
	float encoderLimit = - distance * 180 / PI / 4;

	nMotorEncoder[motorA] = 0;
	motor[motorA] = motor[motorD] = -25;
	while(!(cur_state->interrupt && canInterrupt) && nMotorEncoder[motorA] > encoderLimit){
		eraseDisplay();
		displayString(3,"%d",nMotorEncoder[motorA]);
		displayString(4,"%d",!(cur_state->interrupt && canInterrupt) && nMotorEncoder[motorA] > encoderLimit);
		displayString(5,"%d", getGyroDegrees(S2));

		if(getGyroDegrees(S2) < -1){
			motor[motorA] = -27;
		}
		else if(getGyroDegrees(S2) > 1){
			motor[motorD] = -27;
			} else {
			motor[motorA] = motor[motorD] = -25;
		}

	}

	motor[motorA] = motor[motorD] = 0;

	float distanceTravelled = nMotorEncoder[motorA] * 2 * 4 * PI / 360;

	if(cur_state->dir == 0){
		cur_state->pos[1] = cur_state->pos[1] + (int)(distanceTravelled / 25);
	}
	else if(cur_state->dir == 1){
		cur_state->pos[0] = cur_state->pos[0] + (int)(distanceTravelled / 25);
	}
	else if(cur_state->dir == 2){
		cur_state->pos[1] = cur_state->pos[1] - (int)(distanceTravelled / 25);
	}
	else {
		cur_state->pos[0] = cur_state->pos[0] - (int)(distanceTravelled / 25);
	}
}

void turn(int angle, state* cur_state, bool canInterrupt){
	const int OVERSHOOT_CORRECTION = 2;
	int initAngle = getGyroDegrees(S2);

	if(angle > 0){
		motor[motorA] = -10;
		motor[motorD] = 10;
		while(!(cur_state->interrupt && canInterrupt) && (getGyroDegrees(S2) < (initAngle + angle - OVERSHOOT_CORRECTION))){}
	}
	else if(angle < 0){
		motor[motorA] = 10;
		motor[motorD] = -10;
		while(!(cur_state->interrupt && canInterrupt) && (getGyroDegrees(S2) > (initAngle + angle + OVERSHOOT_CORRECTION))){}
	}

	motor[motorA] = motor[motorD] = 0;

	int deltaAngle;
	int numTurns = 0;

	if(angle > 0){
		deltaAngle = getGyroDegrees(S2)- initAngle;
	}
	else if(angle < 0){
		deltaAngle = initAngle - getGyroDegrees(S2);
	}

	if(deltaAngle % 90 < 85){
		deltaAngle -= deltaAngle % 90;
	}
	else if(deltaAngle % 90 >= 85){
		deltaAngle += 90 - (deltaAngle % 90);
	}
	numTurns = deltaAngle % 90;

	if(angle > 0){
		for(int i = 0; i < numTurns; i++){
			cur_state->dir++;
			if(cur_state->dir == 4){
				cur_state->dir = 0;
			}
		}
	}
	else if(angle < 0){
		for(int i = 0; i < numTurns; i++){
			cur_state->dir--;
			if(cur_state->dir == -1){
				cur_state->dir = 3;
			}
		}
	}
}

void search(int cur_x, int cur_y, int dest_x, int dest_y, int weight){
	if (!map[cur_y][cur_x] || paths[cur_y][cur_x] <= weight && paths[cur_y][cur_x])
		return;
	paths[cur_y][cur_x] = weight;
	weight++;
	search(cur_x, cur_y+1, dest_x, dest_y, weight);
	search(cur_x+1, cur_y, dest_x, dest_y, weight);
	search(cur_x, cur_y-1, dest_x, dest_y, weight);
	search(cur_x-1, cur_y, dest_x, dest_y, weight);
}

int find_dir(int x, int y){
	int weight = paths[y][x];
	int dir[4] ={paths[y-1][x], paths[y][x+1], paths[y+1][x], paths[y][x-1]};
	// This is the correct order {up, right, down, left}
	for (int i = 0; i < 4; i++){
		if (dir[i]+1 == weight)
			return i;
	}
}

void find_nav(int dest_x, int dest_y, int initial_dir){
	int temp_x = dest_x, temp_y = dest_y;
	int preferred_dir = find_dir(temp_x, temp_y);
	TFileHandle output;
	openWritePC(output, "goto_temp.txt");
	while (paths[temp_y][temp_x] > 1){
		int temp_disp = 0;
		if (preferred_dir == 0){
			while (paths[temp_y-1][temp_x] == paths[temp_y][temp_x]-1 && paths[temp_y][temp_x] > 1){
				temp_y--;
				temp_disp++;
			}
		}
		else if (preferred_dir == 1){
			while (paths[temp_y][temp_x+1] == paths[temp_y][temp_x]-1 && paths[temp_y][temp_x] > 1){
				temp_x++;
				temp_disp++;
			}
		}
		else if (preferred_dir == 2){
			while (paths[temp_y+1][temp_x] == paths[temp_y][temp_x]-1 && paths[temp_y][temp_x] > 1){
				temp_y++;
				temp_disp++;
			}
		}
		else if (preferred_dir == 3){
			while (paths[temp_y][temp_x-1] == paths[temp_y][temp_x]-1 && paths[temp_y][temp_x] > 1){
				temp_x--;
				temp_disp++;
			}
		}
		// printf("Drive for %d cm\n", temp_disp*25);
		writeLongPC(output, temp_disp*10); // 10 for testing, 25 for actual
		writeEndlPC(output);
		if (paths[temp_y][temp_x] > 1){
			int temp = find_dir(temp_x, temp_y);
			int dir_difference = preferred_dir - temp;
			// printf("Turn %d degrees\n", dir_difference*90);
			writeLongPC(output, dir_difference*90);
			writeEndlPC(output);
			preferred_dir = temp;
		}
	}
	// printf("Turn %d degrees\n", (initial_dir-preferred_dir)*90);
	writeLongPC(output, (initial_dir-preferred_dir)*90);
	writeEndlPC(output);
	closeFilePC(output);
}

void read_instructions(state* cur_state){
	const int MAX_INS_SIZE = 30;
	int args[MAX_INS_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	TFileHandle input;
	openReadPC(input, "goto_temp.txt");
	int arg;
	for (int i = 0; readIntPC(input, arg); i++){
		args[i] = arg;
	}
	for (int i = 0; i < MAX_INS_SIZE/2; i++){
		int itemp = args[i];
		args[i] = args[MAX_INS_SIZE-1-i];
		args[MAX_INS_SIZE-1-i] = itemp;
	}
	for (int i = 0; i < MAX_INS_SIZE; i++){
		if (args[i]){
			turn(args[i], cur_state, 0);
			i++;
			drive(args[i], cur_state, 0);
		}
	}
	closeFilePC(input);
}

void go_to(int x, int y, state* cur_state){
	bool isTrivial = 1;
	for (int delta_x = min(cur_state->pos[0],x); delta_x < max(cur_state->pos[0],x); delta_x++){
		if (!map[cur_state->pos[1]][delta_x])
			isTrivial = 0;
	}
	for (int delta_y = min(cur_state->pos[1],y); delta_y < max(cur_state->pos[1],y); delta_y++){
		if (!map[delta_y][x])
			isTrivial = 0;
	}
	if (isTrivial){
		if (y - cur_state->pos[1] > 0) // This is delta x
			turn((1-cur_state->dir)*90, cur_state, 0);
		else
			turn((3-cur_state->dir)*90, cur_state, 0);
		drive(abs(y-cur_state->pos[1])*10, cur_state, 0); // 10 for testing, 25 for actual
		if (x - cur_state->pos[0] > 0) // This is delta y
			turn(-cur_state->dir*90, cur_state, 0);
		else
			turn((2-cur_state->dir)*90, cur_state, 0);
		drive(abs(x-cur_state->pos[0])*10, cur_state, 0); // 10 for testing, 25 for actual
	}

	initialize_paths();
	search(cur_state->pos[0], cur_state->pos[1], x, y, 1);
	find_nav(x, y, cur_state->dir);
	read_instructions(cur_state);
	//motor functions later lol
	//output the "trivial solution" messages to a file so I can read the first one
}


task main(){
	SensorType[S2] = sensorEV3_Gyro;
	wait1Msec(50);
	SensorMode[S2] = modeEV3Gyro_RateAndAngle;
	wait1Msec(50);
	state current_state;
	current_state.pos[0] = 8;
	current_state.pos[1] = 8;
	current_state.dir = 1;
	go_to(3,2,&current_state);

}