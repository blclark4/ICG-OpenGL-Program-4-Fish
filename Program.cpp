#include <iostream>
#include <Windows.h>
#include <GL/glew.h>

#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/freeglut.h>
#include "OpenGL445Setup-2026.h"
#include <ctime>
/* Graphics Pgm 4 for Brian Clark
*   EXTRA CREDIT:
*               1. Fish Rotation - the fish can be made to rotate at
*                   half speed, which is started with the "H" key and
*                   ended with the "U" key.
*               2. Animated rising bubbles drift up from the teapot
*                   every half second, moving at randomized amounts,
*                   and disappearing after 1.5 seconds.
*
    Program Structure:

    Global Variables:  Position orientation and state of the big fish,
                       along with its movement and rotation parameters,
                       including an array that tracks a variable and
                       rotating set of rising bubbles, are all stored
                       as global variables.

    Display:  The central display event handler is display_func(), which
              sets the background color, and draws each frame, relying
              on subordinate drawing functions: draw_big_fish(),
              draw_small_fish(), draw_decoration(), and draw_bubbles().

    Animation: Movement is accomplished using timer callbacks to
               timer_func(), which handles the side-to-side movement
               of the fish and its rotation, and also handles both the
               callback for the creation of new bubbles from the teapot
               decoration, and the randomize rising action of a
               varying array of bubbles.  The timer_func updates the
               global variables determining big fish position and rotation.
               Rising bubbles are tracked individually in an array of Bubble
               structs.

    Keyboard events:  The keyboard_func() handles all keypresses. These
               include the H to slow the fish rotation, U to use full
               speed rotation, and the Q key to exit the program.

*/

#define canvas_Width 800
#define canvas_Height 800

// Animation handle defines
#define BF_MOV_ACTN 1
#define BF_ROT_ACTN 2
#define TP_BUB_ACTN 3
#define TP_BUB_OFFSET 10

#define BUB_BUF_SIZE 10

// teapot scale multipliers - derived from freeglut source
// on GitHub
float teapot_depth_scale = 1.0f;
float teapot_width_scale = 3.2625f;
float teapot_height_scale = 1.575f;

struct fcolor { float r; float g; float b; };

// EXTRA CREDIT: Tracking bubble age and position
struct Bubble { float x; float y; int age_msec; bool is_active; };

// Global color definitions
fcolor g_c_clear_day = { 0.911f, 1.0f, 0.992f };
fcolor g_c_fl_red_orange = { 0.988f, 0.518f, 0.153f };
fcolor g_c_burnt_orange = { 0.8f, 0.333f, 0.0f };
fcolor g_c_algae_red = { 0.608f, 0.239f, 0.333f };

// Globals fro Big Fish
GLuint g_dl_bigFish = 0;    // dispay list id for Big Fish

// Animation Timeout value Globals
unsigned int g_big_fish_move_timeout = 50;
unsigned int g_big_fish_turn_timeout = 50;
unsigned int g_teapot_new_bub_timeout = 500;
unsigned int g_bubble_float_timeout = 50;


// Global Big Fish status
bool g_big_fish_is_turning = false;
float g_big_fish_turn_to = 0.0f; // destination rotation amt

// Big fish starting position
float g_big_fish_x = 0;
float g_big_fish_y = 0;
float g_big_fish_z = -400.0f;

float g_big_fish_move_x = -4.0f;  // big fish  movment rate

float g_big_fish_rot_rate = 5.0f;  // big fish rotation rate (varies)

float g_big_fish_left_limit = -325.0f;   // defines leftmost allowed
float g_big_fish_right_limit = 325.0f;   // defines rightmost allowed

float g_big_fish_rot = 0;   // current rotational position
bool g_big_fish_slow_rot = false; // flag for slower rotation

double g_teapot_size = 50.0 / teapot_width_scale;  // size for WireTeapot
//     glTranslatef(75.0f, -400.0f, -400.0f); // move to position
float g_teapot_x = 75.0f;
float g_teapot_y = -400 + (g_teapot_size * teapot_height_scale / 2.0f);
float g_teapot_z = -400;

// Globals for Bubbles 
Bubble bubbles[BUB_BUF_SIZE];   // an array for generated bubbles
int g_next_bubble_id = 0;       // next bubble array index
int g_bub_max_age = 1500;       // msecs at which bubble disappears
float g_bub_start_x = g_teapot_x + 25.0f;      // bubble initial x position
float g_bub_start_y = g_teapot_y + (g_teapot_size * teapot_height_scale / 2.0f);  // bubble initial y position
float g_bub_z = -400.0f;        // bubble z position

// retrieves the next id, then moves to next available position
int get_new_bub_id() {
    int newId = g_next_bubble_id;
    g_next_bubble_id++;
    if (g_next_bubble_id >= BUB_BUF_SIZE) {
        g_next_bubble_id = 0;
    }
    return newId;
}

// handles keyboard input events
void keyboard_func(unsigned char value, int x, int y) {
    // H key to start half-speed rotation
    if (value == 'H' || value == 'h') {
        // Extra credit - start slow turn
        g_big_fish_slow_rot = true;
    }
    // U key to stop half-speed rotation
    else if (value == 'U' || value == 'u') {
        g_big_fish_slow_rot = false;
    }
    // Q key to exit
    else if (value == 'Q' || value == 'q') {
        glutLeaveMainLoop();
    }


}

// handles timer callbacks
void timer_func(int value) {
    // Timeouts related to fish movement
    if (value == BF_MOV_ACTN || value == BF_ROT_ACTN) {
        // Left-Right movment for fish
        if (value == BF_MOV_ACTN && !g_big_fish_is_turning) {
            g_big_fish_x += g_big_fish_move_x;  // add x movement
            // test for proximity to edge
            if (g_big_fish_x <= g_big_fish_left_limit) {
                // stop movement, turn left
                g_big_fish_x = g_big_fish_left_limit; // reset any extra
                g_big_fish_turn_to = 180.0f; // set a destination rotation
                g_big_fish_is_turning = true; // set rotation flag
                g_big_fish_rot_rate = 5.0f; // set positive rotation rate
                g_big_fish_move_x = 5.0f;  // set movement to right
            }
            else if (g_big_fish_x >= g_big_fish_right_limit) {
                // stop movement, turn right
                g_big_fish_x = g_big_fish_right_limit; // reset extra
                g_big_fish_turn_to = 0.0f; // set destination rotation
                g_big_fish_is_turning = true; // set rotation flag
                g_big_fish_rot_rate = -5.0f; // set negativer rotation rate
                g_big_fish_move_x = -5.0f; //  set movement to left
            }
            glutPostRedisplay();
        }
        // Rotation movement for fish
        else if (value == BF_ROT_ACTN && g_big_fish_is_turning) {
            float turnFactor = 1.0f;
            // EXTRA CREDIT:  Slow turn rate to half based on user input
            if (g_big_fish_slow_rot) turnFactor = 0.5f;
            g_big_fish_rot += g_big_fish_rot_rate * turnFactor;
            // detect end of counterclockwise rotation
            if (g_big_fish_rot_rate > 0.0f && g_big_fish_rot >= g_big_fish_turn_to)
            {
                g_big_fish_rot = g_big_fish_turn_to; // snap to destination
                g_big_fish_is_turning = false; // turn off rotation flag
            } // detect end of clockwise rotation
            else if (g_big_fish_rot_rate < 0.0f && g_big_fish_rot <= g_big_fish_turn_to) {
                g_big_fish_rot = g_big_fish_turn_to; // snap to destination
                g_big_fish_is_turning = false; // turn off rotation flag
            }
            glutPostRedisplay();
        }

        // set correct fish movement timeout callback
        if (g_big_fish_is_turning)
        {
            glutTimerFunc(g_big_fish_turn_timeout, timer_func, BF_ROT_ACTN);
        }
        else
        {
            glutTimerFunc(g_big_fish_move_timeout, timer_func, BF_MOV_ACTN);
        }

    }
    // Handle bubble generation callback
    else if (value == TP_BUB_ACTN) {
        // EXTRA CREDIT: A Bubble Appears at the Teapot
        // create a new bubble, add it to the event rotation
        int newBubbleId = get_new_bub_id();
        // init bubble starting values
        bubbles[newBubbleId].age_msec = 0;
        bubbles[newBubbleId].is_active = true;
        bubbles[newBubbleId].x = g_bub_start_x;
        bubbles[newBubbleId].y = g_bub_start_y;
        // setup callback for floating upward
        glutTimerFunc(g_bubble_float_timeout, timer_func, newBubbleId + TP_BUB_OFFSET);
        // set timeout for next bubble
        glutTimerFunc(g_teapot_new_bub_timeout, timer_func, TP_BUB_ACTN);
    }
    else
    {
        // Any callback not handled by now is probably a bubble
        int bubbleId = value - TP_BUB_OFFSET;
        if (bubbles[bubbleId].is_active) {
            // randomly determine upward movement (3 - 7)
            int bub_up = (rand() % 4) + 2;
            // apply movement
            bubbles[bubbleId].y += bub_up;
            // setup redisplay
            glutPostRedisplay();
            // age the bubble
            bubbles[bubbleId].age_msec += g_bubble_float_timeout;
            // test bubble age and trigger callback if active
            if (bubbles[bubbleId].age_msec >= g_bub_max_age) {
                // make it inactive
                bubbles[bubbleId].is_active = false;
                bubbles[bubbleId].age_msec = 0;
            }
            else {
                // if it's not disabled, setup callback
                glutTimerFunc(g_bubble_float_timeout, timer_func, value); // callback
            }

        }
    }


}

// setup the script display list
int captureFishDisplayList()
{
    // get handle
    GLuint handle = glGenLists(1);

    // start recording displaylist
    glNewList(handle, GL_COMPILE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPushMatrix();
    // translate to place center of fix at origin
    glTranslatef(-10.0f, 0.0f, 0.0f);
    // size the octohedron torso
    glScalef(60.0f, 25.0f, 12.5f);

    glutWireOctahedron();

    glPopMatrix();
    // draw tail
    glBegin(GL_TRIANGLES);

    glVertex3f(50.0f, 0.0f, 0.0f);
    glVertex3f(70.0f, -10.0f, 0.0f);
    glVertex3f(70.0f, 10.0f, 0.0f);

    glEnd();


    glEndList();

    return handle;
}


// Draws the big fish at position/rotation
void draw_big_fish() {
    // set the fish color
    glColor3f(g_c_fl_red_orange.r, g_c_fl_red_orange.g, g_c_fl_red_orange.b);
    glPushMatrix();

    glTranslatef(g_big_fish_x, g_big_fish_y, g_big_fish_z); // big fish pos.
    glRotatef(g_big_fish_rot, 0.0f, 1.0f, 0.0f); // big fish y rotation

    glCallList(g_dl_bigFish); // play the displaylist

    glPopMatrix();
}

// Draws the small fish, who doesn't move
void draw_small_fish() {
    glColor3f(g_c_burnt_orange.r, g_c_burnt_orange.g, g_c_burnt_orange.b);

    glPushMatrix();
    glTranslatef(-325.0f, -350.0f, -400.0f);
    // draw tail
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_TRIANGLES);
    glVertex3f(21.5f, 0.0f, 0.0f);
    glVertex3f(28.5f, -5.0f, 0.0f);
    glVertex3f(28.5f, 5.0f, 0.0f);
    glEnd();
    // set the body size
    glScalef(21.5f, 10.0f, 5.0f);
    glutWireOctahedron();

    glPopMatrix();
}


// initialize the bubble array
void init_bubbles() {
    for (int i = 0; i < BUB_BUF_SIZE; i++) {
        bubbles[i].age_msec = 0;
        bubbles[i].is_active = false;
        bubbles[i].x = g_bub_start_x;
        bubbles[i].y = g_bub_start_y;
    }
}

// daraw all of the bubbles to the canvas
void draw_bubbles() {
    for (int i = 0; i < BUB_BUF_SIZE; i++) {
        // if the bubble is active
        if (bubbles[i].is_active) {
            // draw the bubble
            glPushMatrix();

            glTranslatef(bubbles[i].x, bubbles[i].y, g_bub_z);
            glutWireSphere(3.5, 20, 20);

            glPopMatrix();
        }
    }
}


// Draw teapot
void draw_decoration() {

    glPushMatrix();
    glColor3f(g_c_algae_red.r, g_c_algae_red.g, g_c_algae_red.b);

    glTranslatef(g_teapot_x, g_teapot_y, g_teapot_z); // move to position

    glutWireTeapot(g_teapot_size); // draw teapot


    glPopMatrix();

}


// Creates all objects in the view area
void display_func()
{
    // set background color
    glClearColor(g_c_clear_day.r, g_c_clear_day.g, g_c_clear_day.b, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw both fish
    draw_big_fish();
    draw_small_fish();

    // draw teapot and bubbles
    draw_decoration();
    draw_bubbles(); // draw all active bubbles

    glutSwapBuffers();
    glFlush();   // may need to uncomment this line!
}

char canvas_Name[] = "Program 4 - Swimming Fish Simulation";

// #define canvas_name "NAME OF CANVAS";


int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    srand(time(NULL)); // init randomizer

    my_setup(canvas_Width, canvas_Height, canvas_Name);

    g_dl_bigFish = captureFishDisplayList();

    // setup fish move callback
    glutTimerFunc(g_big_fish_move_timeout, timer_func, BF_MOV_ACTN);
    // setup bubble creation callback
    glutTimerFunc(g_teapot_new_bub_timeout, timer_func, TP_BUB_ACTN);

    glutDisplayFunc(display_func);  // setup the display function
    glutKeyboardFunc(keyboard_func);  // setup the keyboard function

    glutMainLoop();
    return 0;
}