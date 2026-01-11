/* Copyright (c) 2007 Scott Lembcke
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
/*
	IMPORTANT - READ ME!
	
	This file sets up a simple interface that the individual demos can use to get
	a Chipmunk space running and draw what's in it. In order to keep the Chipmunk
	examples clean and simple, they contain no graphics code. All drawing is done
	by accessing the Chipmunk structures at a very low level. It is NOT
	recommended to write a game or application this way as it does not scale
	beyond simple shape drawing and is very dependent on implementation details
	about Chipmunk which may change with little to no warning.
*/

#include "chipmunkdemo.h"
#include "main.h"
#include "nv3030b.h"
#include "chipmunk/chipmunk_structs.h"
#define WIDTH LCD_W
#define HEIGHT LCD_H
#define BALL_RADIUS 20

#define GRABBABLE_MASK_BIT (1<<31)
cpShapeFilter GRAB_FILTER = {CP_NO_GROUP, GRABBABLE_MASK_BIT, GRABBABLE_MASK_BIT};
cpShapeFilter NOT_GRABBABLE_FILTER = {CP_NO_GROUP, ~GRABBABLE_MASK_BIT, ~GRABBABLE_MASK_BIT};
static cpBody *KinematicBoxBody;

static void draw(cpShape *shape, void *data)
{
	dbmsg("draw");
        cpBody *body = cpShapeGetBody(shape);
        cpVect pos = cpBodyGetPosition(body);
        
	switch(shape->klass->type){
		case CP_CIRCLE_SHAPE: {
            cpFloat radius = cpCircleShapeGetRadius(shape);
            cpVect center = cpCircleShapeGetOffset(shape);
            int16_t x = (int16_t)(pos.x + center.x);
            int16_t y = (int16_t)(pos.y + center.y);
            uint16_t color = 0x00FF;  // 绿色
            NV3030B_DrawCircle(x, y, (uint16_t)radius, color, CIRCLE_DRAW_ALL);  
			break;
		}
		case CP_SEGMENT_SHAPE: {
            cpVect a = cpSegmentShapeGetA(shape);
            cpVect b = cpSegmentShapeGetB(shape);
            NV3030B_DrawPoint((int16_t)(pos.x + a.x), (int16_t)(pos.y + a.y), 0x00FF); // 蓝色
            NV3030B_DrawPoint((int16_t)(pos.x + b.x), (int16_t)(pos.y + b.y), 0x00FF); // 蓝色
			break;
		}
		case CP_POLY_SHAPE: {
            cpBB bb = cpShapeGetBB(shape);
            int16_t x = (int16_t)bb.l;
            int16_t y = (int16_t)bb.b;
            uint16_t width = (uint16_t)(bb.r - bb.l);
            uint16_t height = (uint16_t)(bb.t - bb.b);
            uint16_t color = 0xFF00;  // 红色
            NV3030B_DrawBox(x, y, width, height, color);
			break;
		}
		default: break;
	}
}

static void render(cpSpace *space) {
    NV3030B_ClearBuffer();
    
    cpSpaceEachShape(space, draw, NULL);
    
    NV3030B_SendBuffer();
}

static void
update(cpSpace *space, double dt)
{
	cpSpaceStep(space, dt);
	
}

static void
AddBox(cpSpace *space, cpVect pos, cpFloat mass, cpFloat width, cpFloat height)
{
	cpBody *body = cpSpaceAddBody(space, cpBodyNew(mass, cpMomentForBox(mass, width, height)));
	cpBodySetPosition(body, pos);
	
	cpShape *shape = cpSpaceAddShape(space, cpBoxShapeNew(body, width, height, 0.0));
	cpShapeSetElasticity(shape, 0.0f);
	cpShapeSetFriction(shape, 0.7f);
}

static void
AddSegment(cpSpace *space, cpVect pos, cpFloat mass, cpFloat width, cpFloat height)
{
	cpBody *body = cpSpaceAddBody(space, cpBodyNew(mass, cpMomentForBox(mass, width, height)));
	cpBodySetPosition(body, pos);
	
	cpShape *shape = cpSpaceAddShape(space, cpSegmentShapeNew(body, cpv(0.0, (height - width)/2.0), cpv(0.0, (width - height)/2.0), width/2.0));
	cpShapeSetElasticity(shape, 0.0f);
	cpShapeSetFriction(shape, 0.7f);
}

static void
AddCircle(cpSpace *space, cpVect pos, cpFloat mass, cpFloat radius)
{
	cpBody *body = cpSpaceAddBody(space, cpBodyNew(mass, cpMomentForCircle(mass, 0.0, radius, cpvzero)));
	cpBodySetPosition(body, pos);
	
	cpShape *shape = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));
	cpShapeSetElasticity(shape, 0.0f);
	cpShapeSetFriction(shape, 0.7f);
}

static cpSpace *
init(void)
{
	cpSpace *space = cpSpaceNew();
	cpSpaceSetGravity(space, cpv(0, -600));
	
	cpShape *shape;
	
	// We create an infinite mass rogue body to attach the line segments too
	// This way we can control the rotation however we want.
	KinematicBoxBody = cpSpaceAddBody(space, cpBodyNewKinematic());
	cpBodySetAngularVelocity(KinematicBoxBody, 0.4f);
	
	// Set up the static box.
	cpVect a = cpv(-200, -200);
	cpVect b = cpv(-200,  200);
	cpVect c = cpv( 200,  200);
	cpVect d = cpv( 200, -200);
	
	shape = cpSpaceAddShape(space, cpSegmentShapeNew(KinematicBoxBody, a, b, 0.0f));
	cpShapeSetElasticity(shape, 1.0f);
	cpShapeSetFriction(shape, 1.0f);
	cpShapeSetFilter(shape, NOT_GRABBABLE_FILTER);

	shape = cpSpaceAddShape(space, cpSegmentShapeNew(KinematicBoxBody, b, c, 0.0f));
	cpShapeSetElasticity(shape, 1.0f);
	cpShapeSetFriction(shape, 1.0f);
	cpShapeSetFilter(shape, NOT_GRABBABLE_FILTER);

	shape = cpSpaceAddShape(space, cpSegmentShapeNew(KinematicBoxBody, c, d, 0.0f));
	cpShapeSetElasticity(shape, 1.0f);
	cpShapeSetFriction(shape, 1.0f);
	cpShapeSetFilter(shape, NOT_GRABBABLE_FILTER);

	shape = cpSpaceAddShape(space, cpSegmentShapeNew(KinematicBoxBody, d, a, 0.0f));
	cpShapeSetElasticity(shape, 1.0f);
	cpShapeSetFriction(shape, 1.0f);
	cpShapeSetFilter(shape, NOT_GRABBABLE_FILTER);
	
	cpFloat mass = 1;
	cpFloat width = 30;
	cpFloat height = width*2;
	
	// Add the bricks.
	for(int i=0; i<1; i++){
		for(int j=0; j<1; j++){
			cpVect pos = cpv(i*width - 150, j*height - 150);
			
			int type = (rand()%3000)/1000;
			if(type ==0){
				AddBox(space, pos, mass, width, height);
			} else if(type == 1){
				AddSegment(space, pos, mass, width, height);
			} else {
				AddCircle(space, cpvadd(pos, cpv(0.0, (height - width)/2.0)), mass, width/2.0);
				AddCircle(space, cpvadd(pos, cpv(0.0, (width - height)/2.0)), mass, width/2.0);
			}
		}
	}
	
	return space;
}
cpSpace *sp;
void chipmunk_example_init()
{
	sp = init();
}

void chipmunk_example_update(float timems)
{
	update(sp,timems);
}