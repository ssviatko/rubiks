/******************************************
 Rubik's Cube Puzzle Solver
 By Stephen Sviatko - ssviatko@gmail.com
 
 Created for CSC471 - Parallel Programming
 A course I taught at UAT - University of
 Advancing Technology, in Tempe AZ
 Summer 2009 Semester

 Students were invited to parallelize this
 algorithm to achieve speedup as part of
 a final project. They had a choice between
 this program, an N-Queens implementation,
 a fractal generator, or a variant of the
 common "game of life" simulation.

 There were no takers.

 30/May/2009 - (C) 2009 Stephen Sviatko
 Good Neighbors LLC

 Permission is granted to use this code in
 whole or in part, as long as the original
 author is notified and properly credited.
 ******************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

#define LOGGING true
#define NUM_CUBES 1

char colors[] = "BYOWRG";

// facing white with blue top:
enum { BLU, YEL, ORG, WHT, RED, GRN };
enum { UP, BACK, LEFT, FRONT, RIGHT, DOWN };

// move list
enum { ROTU, ROTUI, ROTB, ROTBI, ROTL, ROTLI, ROTF, ROTFI, ROTR, ROTRI, ROTD, ROTDI };

typedef struct {
	int tile[3][3];
} face_t;

typedef struct {
	face_t face[6];
	unsigned int totalMoves; // keep track of total moves
	// and moves for each function
	unsigned int solveGreenCrossMoves;
	unsigned int solveGreenCornersMoves;
	unsigned int solveMiddleEdgesMoves;
	unsigned int solveBlueCrossMoves;
	unsigned int alignBlueCornersMoves;
} cube_t;

typedef struct {
	int faceid1;
	int tilex1;
	int tiley1;
	int faceid2;
	int tilex2;
	int tiley2;
} block2_t;

typedef struct {
	int faceid1;
	int tilex1;
	int tiley1;
	int faceid2;
	int tilex2;
	int tiley2;
	int faceid3;
	int tilex3;
	int tiley3;
} block3_t;

// all possible locations for a 2-block
block2_t block2pairs[] = {
	// first four is the green cross
	{ FRONT, 2, 1, DOWN, 0, 1 },
	{ LEFT, 2, 1, DOWN, 1, 0 },
	{ BACK, 2, 1, DOWN, 2, 1 },
	{ RIGHT, 2, 1, DOWN, 1, 2 },
	// next four are the middle edges
	{ FRONT, 1, 0, LEFT, 1, 2 },
	{ BACK, 1, 2, LEFT, 1, 0 },
	{ BACK, 1, 0, RIGHT, 1, 2 },
	{ FRONT, 1, 2, RIGHT, 1, 0 },
	// and finally the blue cross
	{ FRONT, 0, 1, UP, 2, 1 },
	{ LEFT, 0, 1, UP, 1, 0 },
	{ BACK, 0, 1, UP, 0, 1 },
	{ RIGHT, 0, 1, UP, 1, 2 }
};

// Block2 position tags
enum {
	GREENCROSSFRONT, GREENCROSSLEFT, GREENCROSSBACK, GREENCROSSRIGHT,
	MIDDLEFRONTLEFT, MIDDLEBACKLEFT, MIDDLEBACKRIGHT, MIDDLEFRONTRIGHT,
	BLUECROSSFRONT, BLUECROSSLEFT, BLUECROSSBACK, BLUECROSSRIGHT
};

// all possible locations for a 3-block
block3_t block3triplets[] = {
	// first four are the bottom corners
	{ FRONT, 2, 0, LEFT, 2, 2, DOWN, 0, 0 },
	{ BACK, 2, 2, LEFT, 2, 0, DOWN, 2, 0 },
	{ BACK, 2, 0, RIGHT, 2, 2, DOWN, 2, 2 },
	{ FRONT, 2, 2, RIGHT, 2, 0, DOWN, 0, 2 },
	// next four are the top corners
	{ FRONT, 0, 0, LEFT, 0, 2, UP, 2, 0 },
	{ BACK, 0, 2, LEFT, 0, 0, UP, 0, 0 },
	{ BACK, 0, 0, RIGHT, 0, 2, UP, 0, 2 },
	{ FRONT, 0, 2, RIGHT, 0, 0, UP, 2, 2 }
};

// Block3 position tags
enum {
	GREENCORNERFRONTLEFT, GREENCORNERBACKLEFT, GREENCORNERBACKRIGHT, GREENCORNERFRONTRIGHT,
	BLUECORNERFRONTLEFT, BLUECORNERBACKLEFT, BLUECORNERBACKRIGHT, BLUECORNERFRONTRIGHT
};

// Blue cross states
enum {
	BLUECROSSSTATENONE,
	BLUECROSSSTATELFRONTLEFT, BLUECROSSSTATELBACKLEFT, BLUECROSSSTATELBACKRIGHT, BLUECROSSSTATELFRONTRIGHT,
	BLUECROSSSTATELINEH, BLUECROSSSTATELINEV,
	BLUECROSSSTATECROSS
};

cube_t cube[NUM_CUBES];

// Face Rotators
void cwface(int index, int face)
{
	// utility function to rotate an absolute face clockwise: does not affect side tiles
	face_t save;
	
	// buffer the face
	memcpy(&save, &cube[index].face[face], sizeof(face_t));
	
	// set the cube face to the buffer, rotated. Center tile does not change as it just rotates on its own axis.
	cube[index].face[face].tile[0][0] = save.tile[2][0];
	cube[index].face[face].tile[0][1] = save.tile[1][0];
	cube[index].face[face].tile[0][2] = save.tile[0][0];
	cube[index].face[face].tile[1][0] = save.tile[2][1];
	cube[index].face[face].tile[1][2] = save.tile[0][1];
	cube[index].face[face].tile[2][0] = save.tile[2][2];
	cube[index].face[face].tile[2][1] = save.tile[1][2];
	cube[index].face[face].tile[2][2] = save.tile[0][2];
	
	// bump move pointer
	cube[index].totalMoves++;
}

void ccwface(int index, int face)
{
	// utility function to rotate an absolute face counterclockwise: does not affect side tiles
	face_t save;
	
	// buffer the face
	memcpy(&save, &cube[index].face[face], sizeof(face_t));
	
	// set the cube face to the buffer, rotated. Center tile does not change as it just rotates on its own axis.
	cube[index].face[face].tile[0][0] = save.tile[0][2];
	cube[index].face[face].tile[0][1] = save.tile[1][2];
	cube[index].face[face].tile[0][2] = save.tile[2][2];
	cube[index].face[face].tile[1][0] = save.tile[0][1];
	cube[index].face[face].tile[1][2] = save.tile[2][1];
	cube[index].face[face].tile[2][0] = save.tile[0][0];
	cube[index].face[face].tile[2][1] = save.tile[1][0];
	cube[index].face[face].tile[2][2] = save.tile[2][0];
	
	// bump move pointer
	cube[index].totalMoves++;
}

// Absolute Rotate functions
void abs_rotf(int index)
{
	if (LOGGING)
		printf("rotate: front\n");
	
	cwface(index, FRONT);
	// buffer the upside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[UP].tile[2][i];
	// preform the rotation of side tiles
	cube[index].face[UP].tile[2][0] = cube[index].face[LEFT].tile[2][2];
	cube[index].face[UP].tile[2][1] = cube[index].face[LEFT].tile[1][2];
	cube[index].face[UP].tile[2][2] = cube[index].face[LEFT].tile[0][2];
	cube[index].face[LEFT].tile[0][2] = cube[index].face[DOWN].tile[0][0];
	cube[index].face[LEFT].tile[1][2] = cube[index].face[DOWN].tile[0][1];
	cube[index].face[LEFT].tile[2][2] = cube[index].face[DOWN].tile[0][2];
	cube[index].face[DOWN].tile[0][0] = cube[index].face[RIGHT].tile[2][0];
	cube[index].face[DOWN].tile[0][1] = cube[index].face[RIGHT].tile[1][0];
	cube[index].face[DOWN].tile[0][2] = cube[index].face[RIGHT].tile[0][0];
	cube[index].face[RIGHT].tile[0][0] = savetile[0];
	cube[index].face[RIGHT].tile[1][0] = savetile[1];
	cube[index].face[RIGHT].tile[2][0] = savetile[2];
}

void abs_rotfi(int index)
{
	if (LOGGING)
		printf("rotate: front inverted\n");
	
	ccwface(index, FRONT);
	// buffer the upside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[UP].tile[2][i];
	// preform the rotation of side tiles
	cube[index].face[UP].tile[2][0] = cube[index].face[RIGHT].tile[0][0];
	cube[index].face[UP].tile[2][1] = cube[index].face[RIGHT].tile[1][0];
	cube[index].face[UP].tile[2][2] = cube[index].face[RIGHT].tile[2][0];
	cube[index].face[RIGHT].tile[0][0] = cube[index].face[DOWN].tile[0][2];
	cube[index].face[RIGHT].tile[1][0] = cube[index].face[DOWN].tile[0][1];
	cube[index].face[RIGHT].tile[2][0] = cube[index].face[DOWN].tile[0][0];
	cube[index].face[DOWN].tile[0][0] = cube[index].face[LEFT].tile[0][2];
	cube[index].face[DOWN].tile[0][1] = cube[index].face[LEFT].tile[1][2];
	cube[index].face[DOWN].tile[0][2] = cube[index].face[LEFT].tile[2][2];
	cube[index].face[LEFT].tile[0][2] = savetile[2];
	cube[index].face[LEFT].tile[1][2] = savetile[1];
	cube[index].face[LEFT].tile[2][2] = savetile[0];
}

void abs_rotb(int index)
{
	if (LOGGING)
		printf("rotate: back\n");
	
	cwface(index, BACK);
	// buffer the upside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[UP].tile[0][i];
	// preform the rotation of side tiles
	cube[index].face[UP].tile[0][0] = cube[index].face[RIGHT].tile[0][2];
	cube[index].face[UP].tile[0][1] = cube[index].face[RIGHT].tile[1][2];
	cube[index].face[UP].tile[0][2] = cube[index].face[RIGHT].tile[2][2];
	cube[index].face[RIGHT].tile[0][2] = cube[index].face[DOWN].tile[2][2];
	cube[index].face[RIGHT].tile[1][2] = cube[index].face[DOWN].tile[2][1];
	cube[index].face[RIGHT].tile[2][2] = cube[index].face[DOWN].tile[2][0];
	cube[index].face[DOWN].tile[2][0] = cube[index].face[LEFT].tile[0][0];
	cube[index].face[DOWN].tile[2][1] = cube[index].face[LEFT].tile[1][0];
	cube[index].face[DOWN].tile[2][2] = cube[index].face[LEFT].tile[2][0];
	cube[index].face[LEFT].tile[0][0] = savetile[2];
	cube[index].face[LEFT].tile[1][0] = savetile[1];
	cube[index].face[LEFT].tile[2][0] = savetile[0];
}

void abs_rotbi(int index)
{
	if (LOGGING)
		printf("rotate: back inverted\n");
	
	ccwface(index, BACK);
	// buffer the upside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[UP].tile[0][i];
	// preform the rotation of side tiles
	cube[index].face[UP].tile[0][0] = cube[index].face[LEFT].tile[2][0];
	cube[index].face[UP].tile[0][1] = cube[index].face[LEFT].tile[1][0];
	cube[index].face[UP].tile[0][2] = cube[index].face[LEFT].tile[0][0];
	cube[index].face[LEFT].tile[0][0] = cube[index].face[DOWN].tile[2][0];
	cube[index].face[LEFT].tile[1][0] = cube[index].face[DOWN].tile[2][1];
	cube[index].face[LEFT].tile[2][0] = cube[index].face[DOWN].tile[2][2];
	cube[index].face[DOWN].tile[2][0] = cube[index].face[RIGHT].tile[2][2];
	cube[index].face[DOWN].tile[2][1] = cube[index].face[RIGHT].tile[1][2];
	cube[index].face[DOWN].tile[2][2] = cube[index].face[RIGHT].tile[0][2];
	cube[index].face[RIGHT].tile[0][2] = savetile[0];
	cube[index].face[RIGHT].tile[1][2] = savetile[1];
	cube[index].face[RIGHT].tile[2][2] = savetile[2];
}

void abs_rotr(int index)
{
	if (LOGGING)
		printf("rotate: right\n");
	
	cwface(index, RIGHT);
	// buffer the upside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[UP].tile[i][2];
	// preform the rotation of side tiles
	cube[index].face[UP].tile[0][2] = cube[index].face[FRONT].tile[0][2];
	cube[index].face[UP].tile[1][2] = cube[index].face[FRONT].tile[1][2];
	cube[index].face[UP].tile[2][2] = cube[index].face[FRONT].tile[2][2];
	cube[index].face[FRONT].tile[0][2] = cube[index].face[DOWN].tile[0][2];
	cube[index].face[FRONT].tile[1][2] = cube[index].face[DOWN].tile[1][2];
	cube[index].face[FRONT].tile[2][2] = cube[index].face[DOWN].tile[2][2];
	cube[index].face[DOWN].tile[0][2] = cube[index].face[BACK].tile[2][0];
	cube[index].face[DOWN].tile[1][2] = cube[index].face[BACK].tile[1][0];
	cube[index].face[DOWN].tile[2][2] = cube[index].face[BACK].tile[0][0];
	cube[index].face[BACK].tile[0][0] = savetile[2];
	cube[index].face[BACK].tile[1][0] = savetile[1];
	cube[index].face[BACK].tile[2][0] = savetile[0];
}

void abs_rotri(int index)
{
	if (LOGGING)
		printf("rotate: right inverted\n");
	
	ccwface(index, RIGHT);
	// buffer the upside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[UP].tile[i][2];
	// preform the rotation of side tiles
	cube[index].face[UP].tile[0][2] = cube[index].face[BACK].tile[2][0];
	cube[index].face[UP].tile[1][2] = cube[index].face[BACK].tile[1][0];
	cube[index].face[UP].tile[2][2] = cube[index].face[BACK].tile[0][0];
	cube[index].face[BACK].tile[0][0] = cube[index].face[DOWN].tile[2][2];
	cube[index].face[BACK].tile[1][0] = cube[index].face[DOWN].tile[1][2];
	cube[index].face[BACK].tile[2][0] = cube[index].face[DOWN].tile[0][2];
	cube[index].face[DOWN].tile[0][2] = cube[index].face[FRONT].tile[0][2];
	cube[index].face[DOWN].tile[1][2] = cube[index].face[FRONT].tile[1][2];
	cube[index].face[DOWN].tile[2][2] = cube[index].face[FRONT].tile[2][2];
	cube[index].face[FRONT].tile[0][2] = savetile[0];
	cube[index].face[FRONT].tile[1][2] = savetile[1];
	cube[index].face[FRONT].tile[2][2] = savetile[2];
}

void abs_rotl(int index)
{
	if (LOGGING)
		printf("rotate: left\n");
	
	cwface(index, LEFT);
	// buffer the upside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[UP].tile[i][0];
	// preform the rotation of side tiles
	cube[index].face[UP].tile[0][0] = cube[index].face[BACK].tile[2][2];
	cube[index].face[UP].tile[1][0] = cube[index].face[BACK].tile[1][2];
	cube[index].face[UP].tile[2][0] = cube[index].face[BACK].tile[0][2];
	cube[index].face[BACK].tile[0][2] = cube[index].face[DOWN].tile[2][0];
	cube[index].face[BACK].tile[1][2] = cube[index].face[DOWN].tile[1][0];
	cube[index].face[BACK].tile[2][2] = cube[index].face[DOWN].tile[0][0];
	cube[index].face[DOWN].tile[0][0] = cube[index].face[FRONT].tile[0][0];
	cube[index].face[DOWN].tile[1][0] = cube[index].face[FRONT].tile[1][0];
	cube[index].face[DOWN].tile[2][0] = cube[index].face[FRONT].tile[2][0];
	cube[index].face[FRONT].tile[0][0] = savetile[0];
	cube[index].face[FRONT].tile[1][0] = savetile[1];
	cube[index].face[FRONT].tile[2][0] = savetile[2];
}

void abs_rotli(int index)
{
	if (LOGGING)
		printf("rotate: left inverted\n");
	
	ccwface(index, LEFT);
	// buffer the upside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[UP].tile[i][0];
	// preform the rotation of side tiles
	cube[index].face[UP].tile[0][0] = cube[index].face[FRONT].tile[0][0];
	cube[index].face[UP].tile[1][0] = cube[index].face[FRONT].tile[1][0];
	cube[index].face[UP].tile[2][0] = cube[index].face[FRONT].tile[2][0];
	cube[index].face[FRONT].tile[0][0] = cube[index].face[DOWN].tile[0][0];
	cube[index].face[FRONT].tile[1][0] = cube[index].face[DOWN].tile[1][0];
	cube[index].face[FRONT].tile[2][0] = cube[index].face[DOWN].tile[2][0];
	cube[index].face[DOWN].tile[0][0] = cube[index].face[BACK].tile[2][2];
	cube[index].face[DOWN].tile[1][0] = cube[index].face[BACK].tile[1][2];
	cube[index].face[DOWN].tile[2][0] = cube[index].face[BACK].tile[0][2];
	cube[index].face[BACK].tile[0][2] = savetile[2];
	cube[index].face[BACK].tile[1][2] = savetile[1];
	cube[index].face[BACK].tile[2][2] = savetile[0];
}

void abs_rotu(int index)
{
	if (LOGGING)
		printf("rotate: up\n");
	
	cwface(index, UP);
	// buffer the frontside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[FRONT].tile[0][i];
	// preform the rotation of side tiles
	cube[index].face[FRONT].tile[0][0] = cube[index].face[RIGHT].tile[0][0];
	cube[index].face[FRONT].tile[0][1] = cube[index].face[RIGHT].tile[0][1];
	cube[index].face[FRONT].tile[0][2] = cube[index].face[RIGHT].tile[0][2];
	cube[index].face[RIGHT].tile[0][0] = cube[index].face[BACK].tile[0][0];
	cube[index].face[RIGHT].tile[0][1] = cube[index].face[BACK].tile[0][1];
	cube[index].face[RIGHT].tile[0][2] = cube[index].face[BACK].tile[0][2];
	cube[index].face[BACK].tile[0][0] = cube[index].face[LEFT].tile[0][0];
	cube[index].face[BACK].tile[0][1] = cube[index].face[LEFT].tile[0][1];
	cube[index].face[BACK].tile[0][2] = cube[index].face[LEFT].tile[0][2];
	cube[index].face[LEFT].tile[0][0] = savetile[0];
	cube[index].face[LEFT].tile[0][1] = savetile[1];
	cube[index].face[LEFT].tile[0][2] = savetile[2];
}

void abs_rotui(int index)
{
	if (LOGGING)
		printf("rotate: up inverted\n");
	
	ccwface(index, UP);
	// buffer the frontside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[FRONT].tile[0][i];
	// preform the rotation of side tiles
	cube[index].face[FRONT].tile[0][0] = cube[index].face[LEFT].tile[0][0];
	cube[index].face[FRONT].tile[0][1] = cube[index].face[LEFT].tile[0][1];
	cube[index].face[FRONT].tile[0][2] = cube[index].face[LEFT].tile[0][2];
	cube[index].face[LEFT].tile[0][0] = cube[index].face[BACK].tile[0][0];
	cube[index].face[LEFT].tile[0][1] = cube[index].face[BACK].tile[0][1];
	cube[index].face[LEFT].tile[0][2] = cube[index].face[BACK].tile[0][2];
	cube[index].face[BACK].tile[0][0] = cube[index].face[RIGHT].tile[0][0];
	cube[index].face[BACK].tile[0][1] = cube[index].face[RIGHT].tile[0][1];
	cube[index].face[BACK].tile[0][2] = cube[index].face[RIGHT].tile[0][2];
	cube[index].face[RIGHT].tile[0][0] = savetile[0];
	cube[index].face[RIGHT].tile[0][1] = savetile[1];
	cube[index].face[RIGHT].tile[0][2] = savetile[2];
}

void abs_rotd(int index)
{
	if (LOGGING)
		printf("rotate: down\n");
	
	cwface(index, DOWN);
	// buffer the frontside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[FRONT].tile[2][i];
	// preform the rotation of side tiles
	cube[index].face[FRONT].tile[2][0] = cube[index].face[LEFT].tile[2][0];
	cube[index].face[FRONT].tile[2][1] = cube[index].face[LEFT].tile[2][1];
	cube[index].face[FRONT].tile[2][2] = cube[index].face[LEFT].tile[2][2];
	cube[index].face[LEFT].tile[2][0] = cube[index].face[BACK].tile[2][0];
	cube[index].face[LEFT].tile[2][1] = cube[index].face[BACK].tile[2][1];
	cube[index].face[LEFT].tile[2][2] = cube[index].face[BACK].tile[2][2];
	cube[index].face[BACK].tile[2][0] = cube[index].face[RIGHT].tile[2][0];
	cube[index].face[BACK].tile[2][1] = cube[index].face[RIGHT].tile[2][1];
	cube[index].face[BACK].tile[2][2] = cube[index].face[RIGHT].tile[2][2];
	cube[index].face[RIGHT].tile[2][0] = savetile[0];
	cube[index].face[RIGHT].tile[2][1] = savetile[1];
	cube[index].face[RIGHT].tile[2][2] = savetile[2];
}

void abs_rotdi(int index)
{
	if (LOGGING)
		printf("rotate: down inverted\n");
	
	ccwface(index, DOWN);
	// buffer the frontside tiles
	int savetile[3];
	for (int i = 0; i < 3; i++)
		savetile[i] = cube[index].face[FRONT].tile[2][i];
	// preform the rotation of side tiles
	cube[index].face[FRONT].tile[2][0] = cube[index].face[RIGHT].tile[2][0];
	cube[index].face[FRONT].tile[2][1] = cube[index].face[RIGHT].tile[2][1];
	cube[index].face[FRONT].tile[2][2] = cube[index].face[RIGHT].tile[2][2];
	cube[index].face[RIGHT].tile[2][0] = cube[index].face[BACK].tile[2][0];
	cube[index].face[RIGHT].tile[2][1] = cube[index].face[BACK].tile[2][1];
	cube[index].face[RIGHT].tile[2][2] = cube[index].face[BACK].tile[2][2];
	cube[index].face[BACK].tile[2][0] = cube[index].face[LEFT].tile[2][0];
	cube[index].face[BACK].tile[2][1] = cube[index].face[LEFT].tile[2][1];
	cube[index].face[BACK].tile[2][2] = cube[index].face[LEFT].tile[2][2];
	cube[index].face[LEFT].tile[2][0] = savetile[0];
	cube[index].face[LEFT].tile[2][1] = savetile[1];
	cube[index].face[LEFT].tile[2][2] = savetile[2];
}

// Absolute Indexed Rotate
void abs_rot_indrot(int index, int rottype)
{
	switch (rottype)
	{
		case ROTU:	abs_rotu(index);	break;
		case ROTUI:	abs_rotui(index);	break;
		case ROTB:	abs_rotb(index);	break;
		case ROTBI:	abs_rotbi(index);	break;
		case ROTL:	abs_rotl(index);	break;
		case ROTLI:	abs_rotli(index);	break;
		case ROTF:	abs_rotf(index);	break;
		case ROTFI:	abs_rotfi(index);	break;
		case ROTR:	abs_rotr(index);	break;
		case ROTRI:	abs_rotri(index);	break;
		case ROTD:	abs_rotd(index);	break;
		case ROTDI:	abs_rotdi(index);	break;
		default: printf("Unknown rotation type!\n"); exit(-1); break;
	}
}

// Locate a 2-block on the cube; return a block2pair index
int locate_2block(int index, int color1, int color2)
{
	int i;
	
	for (i = 0; i < 12; i++)
	{
		// okay... block2pairs describes 12 pairs of adjoining tiles that form the 2-edged blocks on the cube.
		// we're iterating through the list of pairs to see if each tile contains either color1 or color2.
		if (((cube[index].face[block2pairs[i].faceid1].tile[block2pairs[i].tilex1][block2pairs[i].tiley1] == color1) &&
			(cube[index].face[block2pairs[i].faceid2].tile[block2pairs[i].tilex2][block2pairs[i].tiley2] == color2)) ||
			((cube[index].face[block2pairs[i].faceid1].tile[block2pairs[i].tilex1][block2pairs[i].tiley1] == color2) &&
			 (cube[index].face[block2pairs[i].faceid2].tile[block2pairs[i].tilex2][block2pairs[i].tiley2] == color1)))
			break;
	}
	
	return i;
}

// find a 3-block on the cube, return a block3triplet index
int locate_3block(int index, int color1, int color2, int color3)
{
	int i;
	
	for (i = 0; i < 8; i++)
	{
		// same deal as above except we are matching 3 colors.
		if (((cube[index].face[block3triplets[i].faceid1].tile[block3triplets[i].tilex1][block3triplets[i].tiley1] == color1) ||
			(cube[index].face[block3triplets[i].faceid1].tile[block3triplets[i].tilex1][block3triplets[i].tiley1] == color2) ||
			(cube[index].face[block3triplets[i].faceid1].tile[block3triplets[i].tilex1][block3triplets[i].tiley1] == color3)) &&
			((cube[index].face[block3triplets[i].faceid2].tile[block3triplets[i].tilex2][block3triplets[i].tiley2] == color1) ||
			 (cube[index].face[block3triplets[i].faceid2].tile[block3triplets[i].tilex2][block3triplets[i].tiley2] == color2) ||
			 (cube[index].face[block3triplets[i].faceid2].tile[block3triplets[i].tilex2][block3triplets[i].tiley2] == color3)) &&
			((cube[index].face[block3triplets[i].faceid3].tile[block3triplets[i].tilex3][block3triplets[i].tiley3] == color1) ||
			 (cube[index].face[block3triplets[i].faceid3].tile[block3triplets[i].tilex3][block3triplets[i].tiley3] == color2) ||
			 (cube[index].face[block3triplets[i].faceid3].tile[block3triplets[i].tilex3][block3triplets[i].tiley3] == color3)))
			break;
	}
	
	return i;
}

void solve_green_cross(int index)
{
	if (LOGGING)
		printf("solve_green_cross: solving green cross:\n");

	// find the green/white block
	int gw2blockloc = locate_2block(index, GRN, WHT);
	if (LOGGING)
		printf("solve_green_cross: solve GRN/WHT: 2-block found at block2pair #%d.\n", gw2blockloc);
	// swing it around to position GREENCROSSFRONT
	switch (gw2blockloc)
	{
		case GREENCROSSFRONT:
			// in the right spot, no need to do anything
			break;
		case GREENCROSSLEFT:
			abs_rotd(index);
			break;
		case GREENCROSSBACK:
			abs_rotd(index);
			abs_rotd(index);
			break;
		case GREENCROSSRIGHT:
			abs_rotdi(index);
			break;
		case MIDDLEFRONTLEFT:
			abs_rotfi(index);
			break;
		case MIDDLEBACKLEFT:
			abs_rotli(index);
			abs_rotd(index);
			break;
		case MIDDLEBACKRIGHT:
			abs_rotr(index);
			abs_rotdi(index);
			break;
		case MIDDLEFRONTRIGHT:
			abs_rotf(index);
			break;
		case BLUECROSSFRONT:
			abs_rotf(index);
			abs_rotf(index);
			break;
		case BLUECROSSLEFT:
			abs_rotl(index);
			abs_rotl(index);
			abs_rotd(index);
			break;
		case BLUECROSSBACK:
			abs_rotu(index);
			abs_rotu(index);
			abs_rotf(index);
			abs_rotf(index);
			break;
		case BLUECROSSRIGHT:
			abs_rotri(index);
			abs_rotri(index);
			abs_rotdi(index);
	}
	// check if the GRN/WHT piece is inverted; if it is, fUlu it
	if (cube[index].face[DOWN].tile[0][1] == WHT)
	{
		if (LOGGING)
			printf("solve_green_cross: solve GRN/WHT: fUlu\n");
		abs_rotfi(index);
		abs_rotd(index);
		abs_rotri(index);
		abs_rotdi(index);
	}

	// find the green/orange block
	int go2blockloc = locate_2block(index, GRN, ORG);
	if (LOGGING)
		printf("solve_green_cross: solve GRN/ORG: 2-block found at block2pair #%d.\n", go2blockloc);
	// swing it around to position GREENCROSSLEFT
	switch (go2blockloc)
	{
		// not going to be at pos 0 since we solved GRN/WHT already
		case GREENCROSSLEFT:
			// do nothing, its in the right place
			break;
		case GREENCROSSBACK:
			abs_rotbi(index);
			abs_rotli(index);
			break;
		case GREENCROSSRIGHT:
			abs_rotri(index);
			abs_rotb(index);
			abs_rotb(index);
			abs_rotli(index);
			break;
		case MIDDLEFRONTLEFT:
			abs_rotl(index);
			break;
		case MIDDLEBACKLEFT:
			abs_rotli(index);
			break;
		case MIDDLEBACKRIGHT:
			abs_rotb(index);
			abs_rotui(index);
			abs_rotl(index);
			abs_rotl(index);
			break;
		case MIDDLEFRONTRIGHT:
			abs_rotr(index);
			abs_rotu(index);
			abs_rotu(index);
			abs_rotl(index);
			abs_rotl(index);
			break;
		case BLUECROSSFRONT:
			abs_rotu(index);
			abs_rotl(index);
			abs_rotl(index);
			break;
		case BLUECROSSLEFT:
			abs_rotl(index);
			abs_rotl(index);
			break;
		case BLUECROSSBACK:
			abs_rotui(index);
			abs_rotl(index);
			abs_rotl(index);
			break;
		case BLUECROSSRIGHT:
			abs_rotu(index);
			abs_rotu(index);
			abs_rotl(index);
			abs_rotl(index);
			break;			
	}
	// check if the GRN/ORG piece is inverted; if it is, fUlu it
	if (cube[index].face[DOWN].tile[1][0] == ORG)
	{
		if (LOGGING)
			printf("solve_green_cross: solve GRN/ORG: fUlu\n");
		abs_rotli(index);
		abs_rotd(index);
		abs_rotfi(index);
		abs_rotdi(index);
	}

	// find the green/red block
	int gr2blockloc = locate_2block(index, GRN, RED);
	if (LOGGING)
		printf("solve_green_cross: solve GRN/RED: 2-block found at block2pair #%d.\n", gr2blockloc);
	// swing it around to position 3
	switch (gr2blockloc)
	{
		// not going to be at pos 0 or 1 since we solved GRN/WHT and GRN/ORG already
		case GREENCROSSBACK:
			abs_rotb(index);
			abs_rotr(index);
			break;
		case GREENCROSSRIGHT:
			// nothing to do
			break;
		case MIDDLEFRONTLEFT:
			abs_rotli(index);
			abs_rotui(index);
			abs_rotl(index);
			abs_rotui(index);
			abs_rotri(index);
			abs_rotri(index);
			break;
		case MIDDLEBACKLEFT:
			abs_rotbi(index);
			abs_rotu(index);
			abs_rotri(index);
			abs_rotri(index);
			break;
		case MIDDLEBACKRIGHT:
			abs_rotr(index);
			break;
		case MIDDLEFRONTRIGHT:
			abs_rotri(index);
			break;
		case BLUECROSSFRONT:
			abs_rotui(index);
			abs_rotri(index);
			abs_rotri(index);
			break;
		case BLUECROSSLEFT:
			abs_rotui(index);
			abs_rotui(index);
			abs_rotri(index);
			abs_rotri(index);
			break;
		case BLUECROSSBACK:
			abs_rotu(index);
			abs_rotri(index);
			abs_rotri(index);
			break;
		case BLUECROSSRIGHT:
			abs_rotri(index);
			abs_rotri(index);
			break;
	}
	// check if the GRN/RED piece is inverted; if it is, fUlu it
	if (cube[index].face[DOWN].tile[1][2] == RED)
	{
		if (LOGGING)
			printf("solve_green_cross: solve GRN/RED: fUlu\n");
		abs_rotri(index);
		abs_rotd(index);
		abs_rotbi(index);
		abs_rotdi(index);
	}

	// find the green/yellow block
	int gy2blockloc = locate_2block(index, GRN, YEL);
	if (LOGGING)
		printf("solve_green_cross: solve GRN/YEL: 2-block found at block2pair #%d.\n", gy2blockloc);
	// swing it around to position 2
	switch (gy2blockloc)
	{
			// not going to be at pos 0, 1 or 3
		case GREENCROSSBACK:
			// nothing to do
			break;
		case MIDDLEFRONTLEFT:
			abs_rotli(index);
			abs_rotu(index);
			abs_rotl(index);
			abs_rotb(index);
			abs_rotb(index);
			break;
		case MIDDLEBACKLEFT:
			abs_rotb(index);
			break;
		case MIDDLEBACKRIGHT:
			abs_rotbi(index);
			break;
		case MIDDLEFRONTRIGHT:
			abs_rotr(index);
			abs_rotui(index);
			abs_rotri(index);
			abs_rotb(index);
			abs_rotb(index);
			break;
		case BLUECROSSFRONT:
			abs_rotu(index);
			abs_rotu(index);
			abs_rotb(index);
			abs_rotb(index);
			break;
		case BLUECROSSLEFT:
			abs_rotu(index);
			abs_rotb(index);
			abs_rotb(index);
			break;
		case BLUECROSSBACK:
			abs_rotb(index);
			abs_rotb(index);
			break;
		case BLUECROSSRIGHT:
			abs_rotui(index);
			abs_rotb(index);
			abs_rotb(index);
			break;
	}
	// check if the GRN/YEL piece is inverted; if it is, fUlu it
	if (cube[index].face[DOWN].tile[2][1] == YEL)
	{
		if (LOGGING)
			printf("solve_green_cross: solve GRN/YEL: fUlu\n");
		abs_rotbi(index);
		abs_rotd(index);
		abs_rotli(index);
		abs_rotdi(index);
	}

	cube[index].solveGreenCrossMoves = cube[index].totalMoves;
	
	if (LOGGING)
		printf("solve_green_cross: solved green cross.\n");
}

void solve_green_corners(int index)
{
	// find the green/orange/white block
	int gow3blockloc = locate_3block(index, GRN, ORG, WHT);
	if (LOGGING)
		printf("solve_green_corners: solve GRN/ORG/WHT: 3-block found at block3triplet #%d.\n", gow3blockloc);
	// swing it around to either positions 0 or 4 (GREENCORNERFRONTLEFT or BLUECORNERFRONTLEFT)
	switch (gow3blockloc)
	{
		case GREENCORNERFRONTLEFT:
			// cool, nothing to do
			break;
		case GREENCORNERBACKLEFT:
			abs_rotbi(index);
			abs_rotui(index);
			abs_rotb(index);
			break;
		case GREENCORNERBACKRIGHT:
			abs_rotb(index);
			abs_rotu(index);
			abs_rotu(index);
			abs_rotbi(index);
			break;
		case GREENCORNERFRONTRIGHT:
			abs_rotr(index);
			abs_rotu(index);
			abs_rotri(index);
			break;
		case BLUECORNERFRONTLEFT:
			// this position is cool too
			break;
		case BLUECORNERBACKLEFT:
			abs_rotui(index);
			break;
		case BLUECORNERBACKRIGHT:
			abs_rotui(index);
			abs_rotui(index);
			break;
		case BLUECORNERFRONTRIGHT:
			abs_rotu(index);
			break;
	}
	// rdRD the cube up to 6 times to get the piece in place
	for (int i = 0; i < 6; i++)
	{
		// check to see if we got it
		if ((cube[index].face[FRONT].tile[2][0] == WHT) &&
			(cube[index].face[LEFT].tile[2][2] == ORG) &&
			(cube[index].face[DOWN].tile[0][0] == GRN))
			break;
		if (LOGGING)
			printf("solve_green_corners: solve GRN/ORG/WHT: rdRD\n");
		// rdRD the cube
		abs_rotli(index);
		abs_rotui(index);
		abs_rotl(index);
		abs_rotu(index);
	}

	// find the green/orange/yellow block
	int goy3blockloc = locate_3block(index, GRN, ORG, YEL);
	if (LOGGING)
		printf("solve_green_corners: solve GRN/ORG/YEL: 3-block found at block3triplet #%d.\n", goy3blockloc);
	// swing it around to either positions 1 or 5 (GREENCORNERBACKLEFT or BLUECORNERBACKLEFT)
	switch (goy3blockloc)
	{
		case GREENCORNERBACKLEFT:
			// nothing to do
			break;
		case GREENCORNERBACKRIGHT:
			abs_rotb(index);
			abs_rotu(index);
			abs_rotbi(index);
			abs_rotu(index);
			abs_rotu(index);
			break;
		case GREENCORNERFRONTRIGHT:
			abs_rotr(index);
			abs_rotui(index);
			abs_rotui(index);
			abs_rotri(index);
			break;
		case BLUECORNERFRONTLEFT:
			abs_rotu(index);
			break;
		case BLUECORNERBACKLEFT:
			// nothing to do
			break;
		case BLUECORNERBACKRIGHT:
			abs_rotui(index);
			break;
		case BLUECORNERFRONTRIGHT:
			abs_rotu(index);
			abs_rotu(index);
			break;
	}
	// rdRD the cube up to 6 times to get the piece in place
	for (int i = 0; i < 6; i++)
	{
		// check to see if we got it
		if ((cube[index].face[BACK].tile[2][2] == YEL) &&
			(cube[index].face[LEFT].tile[2][0] == ORG) &&
			(cube[index].face[DOWN].tile[2][0] == GRN))
			break;
		if (LOGGING)
			printf("solve_green_corners: solve GRN/ORG/YEL: rdRD\n");
		// rdRD the cube
		abs_rotbi(index);
		abs_rotui(index);
		abs_rotb(index);
		abs_rotu(index);
	}

	// find the green/yellow/red block
	int gyr3blockloc = locate_3block(index, GRN, YEL, RED);
	if (LOGGING)
		printf("solve_green_corners: solve GRN/YEL/RED: 3-block found at block3triplet #%d.\n", gyr3blockloc);
	// swing it around to either positions 2 or 6 (GREENCORNERBACKRIGHT or BLUECORNERBACKRIGHT)
	switch (gyr3blockloc)
	{
		case GREENCORNERBACKRIGHT:
			// nothing to do
			break;
		case GREENCORNERFRONTRIGHT:
			abs_rotr(index);
			abs_rotu(index);
			abs_rotu(index);
			abs_rotri(index);
			abs_rotu(index);
			break;
		case BLUECORNERFRONTLEFT:
			abs_rotu(index);
			abs_rotu(index);
			break;
		case BLUECORNERBACKLEFT:
			abs_rotu(index);
			break;
		case BLUECORNERBACKRIGHT:
			// nothing to do
			break;
		case BLUECORNERFRONTRIGHT:
			abs_rotui(index);
			break;
	}
	// rdRD the cube up to 6 times to get the piece in place
	for (int i = 0; i < 6; i++)
	{
		// check to see if we got it
		if ((cube[index].face[BACK].tile[2][0] == YEL) &&
			(cube[index].face[RIGHT].tile[2][2] == RED) &&
			(cube[index].face[DOWN].tile[2][2] == GRN))
			break;
		if (LOGGING)
			printf("solve_green_corners: solve GRN/YEL/RED: rdRD\n");
		// rdRD the cube
		abs_rotri(index);
		abs_rotui(index);
		abs_rotr(index);
		abs_rotu(index);
	}

	// find the green/white/red block
	int gwr3blockloc = locate_3block(index, GRN, WHT, RED);
	if (LOGGING)
		printf("solve_green_corners: solve GRN/WHT/RED: 3-block found at block3triplet #%d.\n", gwr3blockloc);
	// swing it around to either positions 3 or 7 (GREENCORNERFRONTRIGHT or BLUECORNERFRONTRIGHT)
	switch (gwr3blockloc)
	{
		case GREENCORNERFRONTRIGHT:
			// nothing to do
			break;
		case BLUECORNERFRONTLEFT:
			abs_rotui(index);
			break;
		case BLUECORNERBACKLEFT:
			abs_rotu(index);
			abs_rotu(index);
			break;
		case BLUECORNERBACKRIGHT:
			abs_rotu(index);
			break;
		case BLUECORNERFRONTRIGHT:
			// nothing to do
			break;
	}
	// rdRD the cube up to 6 times to get the piece in place
	for (int i = 0; i < 6; i++)
	{
		// check to see if we got it
		if ((cube[index].face[FRONT].tile[2][2] == WHT) &&
			(cube[index].face[RIGHT].tile[2][0] == RED) &&
			(cube[index].face[DOWN].tile[0][2] == GRN))
			break;
		if (LOGGING)
			printf("solve_green_corners: solve GRN/WHT/RED: rdRD\n");
		// rdRD the cube
		abs_rotfi(index);
		abs_rotui(index);
		abs_rotf(index);
		abs_rotu(index);
	}
	
	cube[index].solveGreenCornersMoves = cube[index].totalMoves - cube[index].solveGreenCrossMoves;
	
	if (LOGGING)
		printf("solve_green_corners: solved green corners and bottom stack of cube.\n");
}

void solve_middle_edges(int index)
{
	if (LOGGING)
		printf("solve_middle_edges: solving middle edges:\n");
	
	// locate WHT/ORG piece
	int wo2blockloc = locate_2block(index, WHT, ORG);
	if (LOGGING)
		printf("solve_middle_edges: solve WHT/ORG: 2-block found at block2pair #%d.\n", wo2blockloc);
	// if it's not magically in place, then we need to process it
	if (!((cube[index].face[FRONT].tile[1][0] == WHT) && (cube[index].face[LEFT].tile[1][2] == ORG)))
	{
		// if the desired block is in the middle stack, we need to get it up top.
		if ((wo2blockloc >= MIDDLEFRONTLEFT) && (wo2blockloc <= MIDDLEFRONTRIGHT))
		{
			if (LOGGING)
				printf("solve_middle_edges: move WHT/ORG 2-block to top stack.\n");
			switch (wo2blockloc)
			{
				case MIDDLEFRONTLEFT:
					// left-lay facing front
					abs_rotui(index);
					abs_rotli(index);
					abs_rotu(index);
					abs_rotl(index);
					abs_rotu(index);
					abs_rotf(index);
					abs_rotui(index);
					abs_rotfi(index);
					break;
				case MIDDLEBACKLEFT:
					// right-lay facing back
					abs_rotu(index);
					abs_rotl(index);
					abs_rotui(index);
					abs_rotli(index);
					abs_rotui(index);
					abs_rotbi(index);
					abs_rotu(index);
					abs_rotb(index);
					break;
				case MIDDLEBACKRIGHT:
					// left-lay facing back
					abs_rotui(index);
					abs_rotri(index);
					abs_rotu(index);
					abs_rotr(index);
					abs_rotu(index);
					abs_rotb(index);
					abs_rotui(index);
					abs_rotbi(index);
					break;
				case MIDDLEFRONTRIGHT:
					// right-lay facing front
					abs_rotu(index);
					abs_rotr(index);
					abs_rotui(index);
					abs_rotri(index);
					abs_rotui(index);
					abs_rotfi(index);
					abs_rotu(index);
					abs_rotf(index);
					break;
			} // switch
			// locate the piece again since we moved it. it should be on top now.
			wo2blockloc = locate_2block(index, WHT, ORG);
			if (LOGGING)
				printf("solve_middle_edges: WHT/ORG 2-block now at block2pair %d.\n", wo2blockloc);
		} // if block is in middle stack
		// if block is not at BLUECROSSFRONT, flip it around so it is
		if (wo2blockloc != BLUECROSSFRONT)
		{
			if (LOGGING)
				printf("solve_middle_edges: move WHT/ORG 2-block to BLUECROSSFRONT.\n");
			switch (wo2blockloc)
			{
				case BLUECROSSFRONT:
					// do nothing
					break;
				case BLUECROSSLEFT:
					abs_rotui(index);
					break;
				case BLUECROSSBACK:
					abs_rotu(index);
					abs_rotu(index);
					break;
				case BLUECROSSRIGHT:
					abs_rotu(index);
					break;
			}
		}
		// lay the square down into place according to it's orientation
		if (LOGGING)
			printf("solve_middle_edges: lay WHT/ORG 2-block down into place.\n");
		if (cube[index].face[FRONT].tile[0][1] == WHT)
		{
			// lay it down to the left
			abs_rotui(index);
			abs_rotli(index);
			abs_rotu(index);
			abs_rotl(index);
			abs_rotu(index);
			abs_rotf(index);
			abs_rotui(index);
			abs_rotfi(index);
		}
		else
		{
			// lay it down to the right from orange side
			abs_rotu(index);
			abs_rotu(index);
			abs_rotf(index);
			abs_rotui(index);
			abs_rotfi(index);
			abs_rotui(index);
			abs_rotli(index);
			abs_rotu(index);
			abs_rotl(index);			
		}
	}

	// locate WHT/RED piece
	int wr2blockloc = locate_2block(index, WHT, RED);
	if (LOGGING)
		printf("solve_middle_edges: solve WHT/RED: 2-block found at block2pair #%d.\n", wr2blockloc);
	// if it's not magically in place, then we need to process it
	if (!((cube[index].face[FRONT].tile[1][2] == WHT) && (cube[index].face[RIGHT].tile[1][0] == RED)))
	{
		// if the desired block is in the middle stack, we need to get it up top.
		if ((wr2blockloc >= MIDDLEFRONTLEFT) && (wr2blockloc <= MIDDLEFRONTRIGHT))
		{
			if (LOGGING)
				printf("solve_middle_edges: move WHT/RED 2-block to top stack.\n");
			switch (wr2blockloc)
			{
				case MIDDLEBACKLEFT:
					// right-lay facing back
					abs_rotu(index);
					abs_rotl(index);
					abs_rotui(index);
					abs_rotli(index);
					abs_rotui(index);
					abs_rotbi(index);
					abs_rotu(index);
					abs_rotb(index);
					break;
				case MIDDLEBACKRIGHT:
					// left-lay facing back
					abs_rotui(index);
					abs_rotri(index);
					abs_rotu(index);
					abs_rotr(index);
					abs_rotu(index);
					abs_rotb(index);
					abs_rotui(index);
					abs_rotbi(index);
					break;
				case MIDDLEFRONTRIGHT:
					// right-lay facing front
					abs_rotu(index);
					abs_rotr(index);
					abs_rotui(index);
					abs_rotri(index);
					abs_rotui(index);
					abs_rotfi(index);
					abs_rotu(index);
					abs_rotf(index);
					break;
			} // switch
			// locate the piece again since we moved it. it should be on top now.
			wr2blockloc = locate_2block(index, WHT, RED);
			if (LOGGING)
				printf("solve_middle_edges: WHT/RED 2-block now at block2pair %d.\n", wr2blockloc);
		} // if block is in middle stack
		// if block is not at BLUECROSSFRONT, flip it around so it is
		if (wr2blockloc != BLUECROSSFRONT)
		{
			if (LOGGING)
				printf("solve_middle_edges: move WHT/RED 2-block to BLUECROSSFRONT.\n");
			switch (wr2blockloc)
			{
				case BLUECROSSFRONT:
					// do nothing
					break;
				case BLUECROSSLEFT:
					abs_rotui(index);
					break;
				case BLUECROSSBACK:
					abs_rotu(index);
					abs_rotu(index);
					break;
				case BLUECROSSRIGHT:
					abs_rotu(index);
					break;
			}
		}
		// lay the square down into place according to it's orientation
		if (LOGGING)
			printf("solve_middle_edges: lay WHT/RED 2-block down into place.\n");
		if (cube[index].face[FRONT].tile[0][1] == WHT)
		{
			// lay it down to the right
			abs_rotu(index);
			abs_rotr(index);
			abs_rotui(index);
			abs_rotri(index);
			abs_rotui(index);
			abs_rotfi(index);
			abs_rotu(index);
			abs_rotf(index);
		}
		else
		{
			// lay it down to the left from red side
			abs_rotui(index);
			abs_rotui(index);
			abs_rotfi(index);
			abs_rotu(index);
			abs_rotf(index);
			abs_rotu(index);
			abs_rotr(index);
			abs_rotui(index);
			abs_rotri(index);			
		}
	}

	// locate YEL/ORG piece
	int yo2blockloc = locate_2block(index, YEL, ORG);
	if (LOGGING)
		printf("solve_middle_edges: solve YEL/ORG: 2-block found at block2pair #%d.\n", yo2blockloc);
	// if it's not magically in place, then we need to process it
	if (!((cube[index].face[BACK].tile[1][2] == YEL) && (cube[index].face[LEFT].tile[1][0] == ORG)))
	{
		// if the desired block is in the middle stack, we need to get it up top.
		if ((yo2blockloc >= MIDDLEFRONTLEFT) && (yo2blockloc <= MIDDLEFRONTRIGHT))
		{
			if (LOGGING)
				printf("solve_middle_edges: move YEL/ORG 2-block to top stack.\n");
			switch (yo2blockloc)
			{
				case MIDDLEBACKLEFT:
					// right-lay facing back
					abs_rotu(index);
					abs_rotl(index);
					abs_rotui(index);
					abs_rotli(index);
					abs_rotui(index);
					abs_rotbi(index);
					abs_rotu(index);
					abs_rotb(index);
					break;
				case MIDDLEBACKRIGHT:
					// left-lay facing back
					abs_rotui(index);
					abs_rotri(index);
					abs_rotu(index);
					abs_rotr(index);
					abs_rotu(index);
					abs_rotb(index);
					abs_rotui(index);
					abs_rotbi(index);
					break;
			} // switch
			// locate the piece again since we moved it. it should be on top now.
			yo2blockloc = locate_2block(index, YEL, ORG);
			if (LOGGING)
				printf("solve_middle_edges: YEL/ORG 2-block now at block2pair %d.\n", yo2blockloc);
		} // if block is in middle stack
		// if block is not at BLUECROSSBACK, flip it around so it is
		if (yo2blockloc != BLUECROSSBACK)
		{
			if (LOGGING)
				printf("solve_middle_edges: move YEL/ORG 2-block to BLUECROSSBACK.\n");
			switch (yo2blockloc)
			{
				case BLUECROSSFRONT:
					abs_rotu(index);
					abs_rotu(index);
					break;
				case BLUECROSSLEFT:
					abs_rotu(index);
					break;
				case BLUECROSSBACK:
					// do nothing
					break;
				case BLUECROSSRIGHT:
					abs_rotui(index);
					break;
			}
		}
		// lay the square down into place according to it's orientation
		if (LOGGING)
			printf("solve_middle_edges: lay YEL/ORG 2-block down into place.\n");
		if (cube[index].face[BACK].tile[0][1] == YEL)
		{
			// lay it down to the right
			abs_rotu(index);
			abs_rotl(index);
			abs_rotui(index);
			abs_rotli(index);
			abs_rotui(index);
			abs_rotbi(index);
			abs_rotu(index);
			abs_rotb(index);
		}
		else
		{
			// lay it down to the left from orange side
			abs_rotui(index);
			abs_rotui(index);
			abs_rotbi(index);
			abs_rotu(index);
			abs_rotb(index);
			abs_rotu(index);
			abs_rotl(index);
			abs_rotui(index);
			abs_rotli(index);			
		}
	}
	
	// locate YEL/RED piece
	int yr2blockloc = locate_2block(index, YEL, RED);
	if (LOGGING)
		printf("solve_middle_edges: solve YEL/RED: 2-block found at block2pair #%d.\n", yr2blockloc);
	// if it's not magically in place, then we need to process it
	if (!((cube[index].face[BACK].tile[1][0] == YEL) && (cube[index].face[RIGHT].tile[1][2] == RED)))
	{
		// if the desired block is in the middle stack, we need to get it up top.
		if ((yr2blockloc >= MIDDLEFRONTLEFT) && (yr2blockloc <= MIDDLEFRONTRIGHT))
		{
			if (LOGGING)
				printf("solve_middle_edges: move YEL/RED 2-block to top stack.\n");
			// left-lay facing back
			abs_rotui(index);
			abs_rotri(index);
			abs_rotu(index);
			abs_rotr(index);
			abs_rotu(index);
			abs_rotb(index);
			abs_rotui(index);
			abs_rotbi(index);
			// locate the piece again since we moved it. it should be on top now.
			yr2blockloc = locate_2block(index, YEL, RED);
			if (LOGGING)
				printf("solve_middle_edges: YEL/RED 2-block now at block2pair %d.\n", yr2blockloc);
		} // if block is in middle stack
		// if block is not at BLUECROSSBACK, flip it around so it is
		if (yr2blockloc != BLUECROSSBACK)
		{
			if (LOGGING)
				printf("solve_middle_edges: move YEL/RED 2-block to BLUECROSSBACK.\n");
			switch (yr2blockloc)
			{
				case BLUECROSSFRONT:
					abs_rotu(index);
					abs_rotu(index);
					break;
				case BLUECROSSLEFT:
					abs_rotu(index);
					break;
				case BLUECROSSBACK:
					// do nothing
					break;
				case BLUECROSSRIGHT:
					abs_rotui(index);
					break;
			}
		}
		// lay the square down into place according to it's orientation
		if (LOGGING)
			printf("solve_middle_edges: lay YEL/RED 2-block down into place.\n");
		if (cube[index].face[BACK].tile[0][1] == YEL)
		{
			// lay it down to the left
			abs_rotui(index);
			abs_rotri(index);
			abs_rotu(index);
			abs_rotr(index);
			abs_rotu(index);
			abs_rotb(index);
			abs_rotui(index);
			abs_rotbi(index);
		}
		else
		{
			// lay it down to the right from red side
			abs_rotu(index);
			abs_rotu(index);
			abs_rotb(index);
			abs_rotui(index);
			abs_rotbi(index);
			abs_rotui(index);
			abs_rotri(index);
			abs_rotu(index);
			abs_rotr(index);			
		}
	}
	
	cube[index].solveMiddleEdgesMoves = cube[index].totalMoves - cube[index].solveGreenCornersMoves - cube[index].solveGreenCrossMoves;

	if (LOGGING)
		printf("solve_middle_edges: solved middle edges and bottom/middle stacks of cube.\n");
}

int identify_blue_cross_state(int index)
{
	// identify if we have an L, a line, or a cross, and which way they are pointing.
	
	if ((cube[index].face[UP].tile[0][1] == BLU) && (cube[index].face[UP].tile[1][0] == BLU) && (cube[index].face[UP].tile[1][2] == BLU) && (cube[index].face[UP].tile[2][1] == BLU))
		return BLUECROSSSTATECROSS;
	
	if ((cube[index].face[UP].tile[1][0] == BLU) && (cube[index].face[UP].tile[1][2] == BLU))
		return BLUECROSSSTATELINEH;
	
	if ((cube[index].face[UP].tile[0][1] == BLU) && (cube[index].face[UP].tile[2][1] == BLU))
		return BLUECROSSSTATELINEV;
	
	if ((cube[index].face[UP].tile[1][0] == BLU) && (cube[index].face[UP].tile[2][1] == BLU))
		return BLUECROSSSTATELFRONTLEFT;
	
	if ((cube[index].face[UP].tile[1][0] == BLU) && (cube[index].face[UP].tile[0][1] == BLU))
		return BLUECROSSSTATELBACKLEFT;
	
	if ((cube[index].face[UP].tile[0][1] == BLU) && (cube[index].face[UP].tile[1][2] == BLU))
		return BLUECROSSSTATELBACKRIGHT;
	
	if ((cube[index].face[UP].tile[1][2] == BLU) && (cube[index].face[UP].tile[2][1] == BLU))
		return BLUECROSSSTATELFRONTRIGHT;
	
	return BLUECROSSSTATENONE;
}

void solve_blue_cross(int index)
{
	if (LOGGING)
		printf("solve_blue_cross: solving blue cross:\n");
	
	int bluecrosstype = identify_blue_cross_state(index);
	while (bluecrosstype != BLUECROSSSTATECROSS)
	{
		if (LOGGING)
			printf("solve_blue_cross: found blue cross state to be %d.\n", bluecrosstype);
		switch (bluecrosstype)
		{
			case BLUECROSSSTATENONE:
				abs_rotf(index);
				abs_rotr(index);
				abs_rotu(index);
				abs_rotri(index);
				abs_rotui(index);
				abs_rotfi(index);
				break;
			case BLUECROSSSTATELFRONTLEFT:
				abs_rotr(index);
				abs_rotb(index);
				abs_rotu(index);
				abs_rotbi(index);
				abs_rotui(index);
				abs_rotri(index);
				break;
			case BLUECROSSSTATELBACKLEFT:
				abs_rotf(index);
				abs_rotr(index);
				abs_rotu(index);
				abs_rotri(index);
				abs_rotui(index);
				abs_rotfi(index);
				break;
			case BLUECROSSSTATELBACKRIGHT:
				abs_rotl(index);
				abs_rotf(index);
				abs_rotu(index);
				abs_rotfi(index);
				abs_rotui(index);
				abs_rotli(index);
				break;
			case BLUECROSSSTATELFRONTRIGHT:
				abs_rotb(index);
				abs_rotl(index);
				abs_rotu(index);
				abs_rotli(index);
				abs_rotui(index);
				abs_rotbi(index);
				break;
			case BLUECROSSSTATELINEH:
				abs_rotf(index);
				abs_rotr(index);
				abs_rotu(index);
				abs_rotri(index);
				abs_rotui(index);
				abs_rotfi(index);
				break;
			case BLUECROSSSTATELINEV:
				abs_rotl(index);
				abs_rotf(index);
				abs_rotu(index);
				abs_rotfi(index);
				abs_rotui(index);
				abs_rotli(index);
				break;
		}
		
		// get it again now that we twiddled it
		bluecrosstype = identify_blue_cross_state(index);
	}
	
	// get the blue/white piece in front and aligned
	while (locate_2block(index, BLU, WHT) != BLUECROSSFRONT)
		abs_rotu(index);
	
	// get the blu/red piece in it's proper spot on the left
	while (locate_2block(index, BLU, RED) != BLUECROSSRIGHT)
	{
		if (LOGGING)
			printf("solve_blue_cross: aligning BLU/RED piece.\n");
		abs_rotr(index);
		abs_rotu(index);
		abs_rotri(index);
		abs_rotu(index);
		abs_rotr(index);
		abs_rotu(index);
		abs_rotu(index);
		abs_rotri(index);
	}
	
	// if blu/yel and blu/org are in a parity state around the left and back, then we want to
	// repeat previous algorithm looking at the orange side (blu/white and blu/red facing the adjusted "back right corner")
	// do this until the white, red, yellow and orange pieces line up in sequence (but not necessarily in the right spots)
	// the yellow piece will stay put (in it's wrong spot) on the left (orange) side, while the white, red and orange pieces will cascade
	// around the front, right and back. When they are in order of yellow, orange, white and red (going CCW) starting on the left,
	// then we will give it one final up-rotation to align them.
	if ((locate_2block(index, BLU, YEL) == BLUECROSSLEFT) && (locate_2block(index, BLU, ORG) == BLUECROSSBACK))
	{
		while ((locate_2block(index, BLU, ORG) != BLUECROSSFRONT) ||
			   (locate_2block(index, BLU, WHT) != BLUECROSSRIGHT) ||
			   (locate_2block(index, BLU, RED) != BLUECROSSBACK))
		{
			if (LOGGING)
				printf("solve_blue_cross: fixing BLU/YEL and BLU/ORG parity.\n");
			abs_rotf(index);
			abs_rotu(index);
			abs_rotfi(index);
			abs_rotu(index);
			abs_rotf(index);
			abs_rotu(index);
			abs_rotu(index);
			abs_rotfi(index);
		}
		// and one final up-rotation to fix it
		abs_rotu(index);
	}
	
	cube[index].solveBlueCrossMoves = cube[index].totalMoves - cube[index].solveMiddleEdgesMoves - cube[index].solveGreenCornersMoves - cube[index].solveGreenCrossMoves;
	
	if (LOGGING)
		printf("solve_blue_cross: solved blue cross.\n");
}

bool check_blue_corner_alignment(int index, int corner)
{
	// returns TRUE if the specified corner is aligned properly with all the colors pointing the right way
	bool retval = false;
	
	switch (corner)
	{
		case BLUECORNERFRONTLEFT:
			if ((cube[index].face[FRONT].tile[0][0] == WHT) && (cube[index].face[LEFT].tile[0][2] == ORG) && (cube[index].face[UP].tile[2][0] == BLU))
				retval = true;
			break;
		case BLUECORNERBACKLEFT:
			if ((cube[index].face[BACK].tile[0][2] == YEL) && (cube[index].face[LEFT].tile[0][0] == ORG) && (cube[index].face[UP].tile[0][0] == BLU))
				retval = true;
			break;
		case BLUECORNERBACKRIGHT:
			if ((cube[index].face[BACK].tile[0][0] == YEL) && (cube[index].face[RIGHT].tile[0][2] == RED) && (cube[index].face[UP].tile[0][2] == BLU))
				retval = true;
			break;
		case BLUECORNERFRONTRIGHT:
			if ((cube[index].face[FRONT].tile[0][2] == WHT) && (cube[index].face[RIGHT].tile[0][0] == RED) && (cube[index].face[UP].tile[2][2] == BLU))
				retval = true;
			break;
	}
	return retval;
}

bool check_working_corner_alignment(int index, int working_corner, int piece_of_interest)
{
	// this function checks corner working_corner for a proper alignment of piece_of_interest, as the top later
	// is rotated during the final solve. piece_of_interest is the corner where the proper color normally resides.
	// for instance, if we call this function with working_corner = BLUECORNERFRONTLEFT and piece_of_interest is
	// BLUECORNERBACKRIGHT, it will check the front-left corner for a proper alignment of the red/yellow/blue piece,
	// if it were rotated into position into the front-left corner.
	
	// returns TRUE if the specified corner is aligned properly with all the colors pointing the right way
	bool retval = false;
	
	switch (working_corner)
	{
		case BLUECORNERFRONTLEFT:
			switch (piece_of_interest)
			{
				case BLUECORNERFRONTLEFT:
					if ((cube[index].face[FRONT].tile[0][0] == WHT) && (cube[index].face[LEFT].tile[0][2] == ORG) && (cube[index].face[UP].tile[2][0] == BLU))
						retval = true;
					break;
				case BLUECORNERBACKLEFT:
					if ((cube[index].face[FRONT].tile[0][0] == ORG) && (cube[index].face[LEFT].tile[0][2] == YEL) && (cube[index].face[UP].tile[2][0] == BLU))
						retval = true;
					break;					
				case BLUECORNERBACKRIGHT:
					if ((cube[index].face[FRONT].tile[0][0] == YEL) && (cube[index].face[LEFT].tile[0][2] == RED) && (cube[index].face[UP].tile[2][0] == BLU))
						retval = true;
					break;
				case BLUECORNERFRONTRIGHT:
					if ((cube[index].face[FRONT].tile[0][0] == RED) && (cube[index].face[LEFT].tile[0][2] == WHT) && (cube[index].face[UP].tile[2][0] == BLU))
						retval = true;
					break;					
			}
			break;
		case BLUECORNERBACKLEFT:
			switch (piece_of_interest)
			{
				case BLUECORNERFRONTLEFT:
					if ((cube[index].face[BACK].tile[0][2] == ORG) && (cube[index].face[LEFT].tile[0][0] == WHT) && (cube[index].face[UP].tile[0][0] == BLU))
						retval = true;
					break;
				case BLUECORNERBACKLEFT:
					if ((cube[index].face[BACK].tile[0][2] == YEL) && (cube[index].face[LEFT].tile[0][0] == ORG) && (cube[index].face[UP].tile[0][0] == BLU))
						retval = true;
					break;
				case BLUECORNERBACKRIGHT:
					if ((cube[index].face[BACK].tile[0][2] == RED) && (cube[index].face[LEFT].tile[0][0] == YEL) && (cube[index].face[UP].tile[0][0] == BLU))
						retval = true;
					break;
				case BLUECORNERFRONTRIGHT:
					if ((cube[index].face[BACK].tile[0][2] == WHT) && (cube[index].face[LEFT].tile[0][0] == RED) && (cube[index].face[UP].tile[0][0] == BLU))
						retval = true;
					break;
			}
			break;
		case BLUECORNERBACKRIGHT:
			switch (piece_of_interest)
			{
				case BLUECORNERFRONTLEFT:
					if ((cube[index].face[BACK].tile[0][0] == WHT) && (cube[index].face[RIGHT].tile[0][2] == ORG) && (cube[index].face[UP].tile[0][2] == BLU))
						retval = true;
					break;
				case BLUECORNERBACKLEFT:
					if ((cube[index].face[BACK].tile[0][0] == ORG) && (cube[index].face[RIGHT].tile[0][2] == YEL) && (cube[index].face[UP].tile[0][2] == BLU))
						retval = true;
					break;
				case BLUECORNERBACKRIGHT:
					if ((cube[index].face[BACK].tile[0][0] == YEL) && (cube[index].face[RIGHT].tile[0][2] == RED) && (cube[index].face[UP].tile[0][2] == BLU))
						retval = true;
					break;
				case BLUECORNERFRONTRIGHT:
					if ((cube[index].face[BACK].tile[0][0] == RED) && (cube[index].face[RIGHT].tile[0][2] == WHT) && (cube[index].face[UP].tile[0][2] == BLU))
						retval = true;
					break;
			}
			break;
		case BLUECORNERFRONTRIGHT:
			switch (piece_of_interest)
			{
				case BLUECORNERFRONTLEFT:
					if ((cube[index].face[FRONT].tile[0][2] == ORG) && (cube[index].face[RIGHT].tile[0][0] == WHT) && (cube[index].face[UP].tile[2][2] == BLU))
						retval = true;
					break;
				case BLUECORNERBACKLEFT:
					if ((cube[index].face[FRONT].tile[0][2] == YEL) && (cube[index].face[RIGHT].tile[0][0] == ORG) && (cube[index].face[UP].tile[2][2] == BLU))
						retval = true;
					break;
				case BLUECORNERBACKRIGHT:
					if ((cube[index].face[FRONT].tile[0][2] == RED) && (cube[index].face[RIGHT].tile[0][0] == YEL) && (cube[index].face[UP].tile[2][2] == BLU))
						retval = true;
					break;
				case BLUECORNERFRONTRIGHT:
					if ((cube[index].face[FRONT].tile[0][2] == WHT) && (cube[index].face[RIGHT].tile[0][0] == RED) && (cube[index].face[UP].tile[2][2] == BLU))
						retval = true;
					break;
			}
			break;
	}
	return retval;
}

void align_blue_corners(int index)
{
	if (LOGGING)
		printf("align_blue_corners: aligning blue corners:\n");
	
	// repeat the corner alignment algorithm until we have at least one corner piece in the right spot (although not necessarily flipped the right way)
	while (!((locate_3block(index, BLU, WHT, ORG) == BLUECORNERFRONTLEFT) ||
		   (locate_3block(index, BLU, YEL, ORG) == BLUECORNERBACKLEFT) ||
		   (locate_3block(index, BLU, YEL, RED) == BLUECORNERBACKRIGHT) ||
		   (locate_3block(index, BLU, WHT, RED) == BLUECORNERFRONTRIGHT)))
	{
		if (LOGGING)
			printf("align_blue_corners: no corners aligned; trying to get initial corner piece aligned\n");
		abs_rotu(index);
		abs_rotr(index);
		abs_rotui(index);
		abs_rotli(index);
		abs_rotu(index);
		abs_rotri(index);
		abs_rotui(index);
		abs_rotl(index);
	}
	
	// now, at least one of the corners is in the right spot. take a walk around the top of the cube and find one.
	int good_corner = -1;
	
	if (locate_3block(index, BLU, WHT, ORG) == BLUECORNERFRONTLEFT)
		good_corner = BLUECORNERFRONTLEFT;
	else if (locate_3block(index, BLU, YEL, ORG) == BLUECORNERBACKLEFT)
		good_corner = BLUECORNERBACKLEFT;
	else if (locate_3block(index, BLU, YEL, RED) == BLUECORNERBACKRIGHT)
		good_corner = BLUECORNERBACKRIGHT;
	else if (locate_3block(index, BLU, WHT, RED) == BLUECORNERFRONTRIGHT)
		good_corner = BLUECORNERFRONTRIGHT;
	
	// repeat corner alignment again, this time with the good corner in the front right while facing the appropriate side
	// do it until all four corners are in the right spots
	while (!((locate_3block(index, BLU, WHT, ORG) == BLUECORNERFRONTLEFT) &&
			 (locate_3block(index, BLU, YEL, ORG) == BLUECORNERBACKLEFT) &&
			 (locate_3block(index, BLU, YEL, RED) == BLUECORNERBACKRIGHT) &&
			 (locate_3block(index, BLU, WHT, RED) == BLUECORNERFRONTRIGHT)))
	{
		if (LOGGING)
			printf("align_blue_corners: trying to get corner pieces in the right spots\n");
		switch (good_corner)
		{
			case BLUECORNERFRONTLEFT:
				abs_rotu(index);
				abs_rotf(index);
				abs_rotui(index);
				abs_rotbi(index);
				abs_rotu(index);
				abs_rotfi(index);
				abs_rotui(index);
				abs_rotb(index);
				break;
			case BLUECORNERBACKLEFT:
				abs_rotu(index);
				abs_rotl(index);
				abs_rotui(index);
				abs_rotri(index);
				abs_rotu(index);
				abs_rotli(index);
				abs_rotui(index);
				abs_rotr(index);
				break;
			case BLUECORNERBACKRIGHT:
				abs_rotu(index);
				abs_rotb(index);
				abs_rotui(index);
				abs_rotfi(index);
				abs_rotu(index);
				abs_rotbi(index);
				abs_rotui(index);
				abs_rotf(index);
				break;
			case BLUECORNERFRONTRIGHT:
				abs_rotu(index);
				abs_rotr(index);
				abs_rotui(index);
				abs_rotli(index);
				abs_rotu(index);
				abs_rotri(index);
				abs_rotui(index);
				abs_rotl(index);
				break;
		}
	}
	
	// find out how many, and which blue corners, are twiddled
	int bad_corners[4]; // array to hold corner tags
	int num_bad_corners = -1; // zero based
	for (int i = BLUECORNERFRONTLEFT; i <= BLUECORNERFRONTRIGHT; i++)
	{
		if (!check_blue_corner_alignment(index, i))
		{
			num_bad_corners++;
			bad_corners[num_bad_corners] = i;
		}
	}
	
	if (LOGGING)
	{
		printf("align_blue_corners: %d twiddled blue corners.\n", num_bad_corners + 1);
		for (int i = 0; i <= num_bad_corners; i++)
			printf("align_blue_corners: --> corner %d is twiddled.\n", bad_corners[i]);
	}
	
	// if there are no twiddled corners, the puzzle is solved! however, if there are,
	// enter our "final solution" loop
	if (num_bad_corners > -1)
	{
		// look at the side face with the first twiddled corner in the top right.
		// do a series rdRD's until the corner is untwiddled.
		// advance to the next twiddled corner (still looking at the first side face) by rotating up-inverted until the next
		// twiddled face is in the top right. continue doing this until we are out of twiddled corners.
		
		// find out which way to look
		int look_at;
		switch (bad_corners[0])
		{
			case BLUECORNERFRONTLEFT:
				look_at = LEFT;
				break;
			case BLUECORNERBACKLEFT:
				look_at = BACK;
				break;
			case BLUECORNERBACKRIGHT:
				look_at = RIGHT;
				break;
			case BLUECORNERFRONTRIGHT:
				look_at = FRONT;
				break;
		}
		
		for (int i = 0; i <= num_bad_corners; i++)
		{
			if (LOGGING)
			{
				printf("align_blue_corners: fixing twiddle on corner %d.\n", bad_corners[i]);
			}
			
			// rdRD the working corner (bad_corners[0]) until it's in proper alignment
			while (!check_working_corner_alignment(index, bad_corners[0], bad_corners[i]))
			{
				switch (look_at)
				{
					case LEFT:
						abs_rotfi(index);
						abs_rotdi(index);
						abs_rotf(index);
						abs_rotd(index);
						break;
					case BACK:
						abs_rotli(index);
						abs_rotdi(index);
						abs_rotl(index);
						abs_rotd(index);
						break;
					case RIGHT:
						abs_rotbi(index);
						abs_rotdi(index);
						abs_rotb(index);
						abs_rotd(index);
						break;
					case FRONT:
						abs_rotri(index);
						abs_rotdi(index);
						abs_rotr(index);
						abs_rotd(index);
						break;
				}
			}
			
			// if bad corners remain, rotate up-inverted (the next corner # - the current corner #) times.
			if (i < num_bad_corners)
			{
				for (int j = bad_corners[i]; j < bad_corners[i + 1]; j++)
					abs_rotui(index);
			}
		} // for int i = 0 to <= num_bad_corners
		
		// if the top layers is shifted, fix it
		while (cube[index].face[FRONT].tile[0][1] != WHT)
			abs_rotu(index);
	} // if num_bad_corners > -1
	
	cube[index].alignBlueCornersMoves = cube[index].totalMoves - cube[index].solveBlueCrossMoves - cube[index].solveMiddleEdgesMoves - cube[index].solveGreenCornersMoves - cube[index].solveGreenCrossMoves;
	
	if (LOGGING)
		printf("align_blue_corners: aligned blue corners and solved cube.\n");
}

// Initialize a cube
void init_cube(int index)
{
	// clear the faces
	for (int i = 0; i < 6; i++)
		for (int j = 0; j < 3; j++)
			for (int k = 0; k < 3; k++)
				cube[index].face[i].tile[j][k] = i;	
}

// Scramble
void scramble_cube(int index)
{
	if (LOGGING)
		printf("scramble_cube:\n");
	
	// 40 random moves
	for (int i = 0; i < 40; i++)
		abs_rot_indrot(index, random() % 12);
	
	// set up move counters
	cube[index].totalMoves = 0;
	cube[index].solveGreenCrossMoves = 0;
	cube[index].solveGreenCornersMoves = 0;
	cube[index].solveMiddleEdgesMoves = 0;
	cube[index].solveBlueCrossMoves = 0;
	cube[index].alignBlueCornersMoves = 0;
}

// Display a cube
void show_cube(int index)
{
	printf("Cube %03d --------------------\n", index);
	
	// show UP
	for (int row = 0; row < 3; row++)
	{
		printf("                ");
		for (int j = 0; j < 3; j++)
			printf("%c ", colors[cube[index].face[UP].tile[row][j]]);
		printf("\n");
	}
	printf("                | | |\n");
	
	// show back, left, front, right
	for (int row = 0; row < 3; row++)
	{
		for (int i = BACK; i <= RIGHT; i++)
		{
			for (int j = 0; j < 3; j++)
				printf("%c ", colors[cube[index].face[i].tile[row][j]]);
			if (i != RIGHT) // suppress dash at end
				printf("- ");
		}
		printf("\n");
	}

	// show DOWN
	printf("                | | |\n");
	for (int row = 0; row < 3; row++)
	{
		printf("                ");
		for (int j = 0; j < 3; j++)
			printf("%c ", colors[cube[index].face[DOWN].tile[row][j]]);
		printf("\n");
	}
}

int main (int argc, char * const argv[])
{
	// setup
	srandom(time(NULL));
	
	// police our average move counts for each function
	unsigned int totalMoves = 0;
	unsigned int solveGreenCrossMoves = 0;
	unsigned int solveGreenCornersMoves = 0;
	unsigned int solveMiddleEdgesMoves = 0;
	unsigned int solveBlueCrossMoves = 0;
	unsigned int alignBlueCornersMoves = 0;
	
	// keep track of our time
	struct timeval start_time, end_time;
	gettimeofday(&start_time, NULL);
	
	printf("Solving %d cubes...\n", NUM_CUBES);
	
	for (int i = 0; i < NUM_CUBES; i++)
	{
		init_cube(i);
		scramble_cube(i);
		if (LOGGING)
		{
			printf("*** Scrambled Cube\n");
			show_cube(i);
		}
	
		// solve it!
		solve_green_cross(i);
		solve_green_corners(i);
		solve_middle_edges(i);
		solve_blue_cross(i);
		align_blue_corners(i);

		if (LOGGING)
		{
			printf("*** Solved Cube in %d moves.\n", cube[i].totalMoves);
			show_cube(i);
		}
		
		totalMoves += cube[i].totalMoves;
		solveGreenCrossMoves += cube[i].solveGreenCrossMoves;
		solveGreenCornersMoves += cube[i].solveGreenCornersMoves;
		solveMiddleEdgesMoves += cube[i].solveMiddleEdgesMoves;
		solveBlueCrossMoves += cube[i].solveBlueCrossMoves;
		alignBlueCornersMoves += cube[i].alignBlueCornersMoves;
	}
	
	double averageTotalMoves = (double)totalMoves / (double)NUM_CUBES;
	double averageSolveGreenCrossMoves = (double)solveGreenCrossMoves / (double)NUM_CUBES;
	double averageSolveGreenCornersMoves = (double)solveGreenCornersMoves / (double)NUM_CUBES;
	double averageSolveMiddleEdgesMoves = (double)solveMiddleEdgesMoves / (double)NUM_CUBES;
	double averageSolveBlueCrossMoves = (double)solveBlueCrossMoves / (double)NUM_CUBES;
	double averageAlignBlueCornersMoves = (double)alignBlueCornersMoves / (double)NUM_CUBES;

	gettimeofday(&end_time, NULL);

	printf("Solved %d cubes.\n", NUM_CUBES);
	printf("Move count averages:\n");
	printf("--> Total Moves        : %f.\n", averageTotalMoves);
	printf("--> Solve Green Cross  : %f.\n", averageSolveGreenCrossMoves);
	printf("--> Solve Green Corners: %f.\n", averageSolveGreenCornersMoves);
	printf("--> Solve Middle Edges : %f.\n", averageSolveMiddleEdgesMoves);
	printf("--> Solve Blue Cross   : %f.\n", averageSolveBlueCrossMoves);
	printf("--> Align Blue Corners : %f.\n", averageAlignBlueCornersMoves);
	printf("\nElapsed time: %ld seconds %ld usecs.\n",
		   end_time.tv_sec - start_time.tv_sec - ((end_time.tv_usec - start_time.tv_usec < 0) ? 1 : 0), // subtract 1 if there was a usec rollover
		   end_time.tv_usec - start_time.tv_usec + ((end_time.tv_usec - start_time.tv_usec < 0) ? 1000000 : 0) // bump usecs by 1 million usec for rollover
	);
	
    return 0;
}
