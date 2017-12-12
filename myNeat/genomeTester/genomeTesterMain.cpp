// This program is for testing NEAT genomes that have been evolved in
// a different program. This is a support program to competitive coevolution
// experiments in Linux.
// It's an openGL program that will show competitions
// between two genomes. Genomes are loaded in as .dna files
// Drew Kirkpatrick, drew.kirkpatrick@gmail.com



#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>
#include "time.h"
#include <signal.h>
#include <queue>

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/freeglut_ext.h>

#include <QtCore/QCoreApplication>
#include <QDataStream>
#include <QFile>

using namespace std;

#include "SVector2D.h"
#include "timer.h"
#include "mathVector.h"
#include "mathMatrix.h"

#include "utils.h"
#include "robotAgent.h"
#include "gameworld.h"
#include "genotype.h"#include "phenotype.h"
#include "parameters.h"


// Menu actions
#define EXIT_ACTION               1
#define HELP_ACTION               2
#define PAUSE_ACTION              3
#define SET_STANDARD_MATCH_ACTION 4
#define SET_TOURNY_MATCH_ACTION   5
#define GREEN_START_LEFT          6
#define GREEN_START_RIGHT         7
#define START_ACTION              8
#define REALTIME_ACTION           9
#define MAX_SPEED_ACTION          10
#define FAST_FORWARD_ACTION       11
#define DRAW_GENOME1_ACTION       12
#define DRAW_GENOME2_ACTION       13
#define RESET_MODE_ACTION         14
#define DRAW_FOOD_SENSOR_ACTION   15
#define DRAW_ENERGY_SENSOR_ACTION 16
#define DRAW_ENEMY_SENSOR_ACTION  17





enum timeType
{
    realtime,
    accelerated,
    fastforward
};

const float realtimeDelay    = 0.04;
const float fastForwardDelay = 0.002;


enum runMode
{
    LOADING,
    LOADED_READY,
    STANDARD_EVAL,
    TOURNY_EVAL,
    DRAW_GENOME1,
    DRAW_GENOME2,
    VCR_REPLAY,
    ROUND_ROBIN,
    END_MODES
};


enum lastMatchWonBy
{
    GREEN_WINNER,
    RED_WINNER,
    NO_WINNER
};



struct tournamentData
{
    bool           greenRobotStartLeft;
    foodLayoutType foodLayout;
    int            firstExtraFoodPosition;
    int            secondExtraFoodPosition;
};


struct drawingPositions
{
    int posX;
    int posY;
};



struct neuronDrawingData
{
    int         posX;
    int         posY;
    neuron_type neuronType;
    double      activationResponse;
    int         neuronID;
};



struct linkDrawingData
{
    int    startX;
    int    startY;
    int    endX;
    int    endY;
    bool   recurrent;
    double weight;
};


struct roundRobinPlayerData
{
    string     dnaFileName;
    CGenome    *genome;
    CNeuralNet *brain;
    RobotAgent *robotAgent;

    int wins;
    int losses;
    int ties;
};



struct roundRobinJob
{
    int playerOneIndex;
    int playerTwoIndex;
};



vector <tournamentData> tournyMatchData;


runMode mode = LOADING;

bool greenRobotStartLeft = true;



int firstGenomeID  = 0;
int secondGenomeID = 0;




timeType     timeMode   = realtime;
//timeType     timeMode   = accelerated;
bool         PAUSE      = false;

bool DONE               = false;
bool START              = false;

bool DRAW_ALL_FOOD_TEST = false;

const int nanoSleepAmount = 1000;       // millionth of a second



// Data for rendering
agentPositionData   renderGreenAgent;
agentPositionData   renderRedAgent;

singlePieceFoodData renderFoodData[MAX_FOOD_PIECES];

brainInputData      greenAgentSensorData;
brainInputData      redAgentSensorData;

unsigned int        numFoodPieces          = 0;

unsigned int        greenAgentEnergy       = 0;
unsigned int        redAgentEnergy         = 0;

unsigned int        greenAgentNumFoodEaten = 0;
unsigned int        redAgentNumFoodEaten   = 0;

unsigned int        greenAgentScore        = 0;
unsigned int        redAgentScore          = 0;


int                 ticks                  = 0;
unsigned int        tournamentStep         = 0;
unsigned int        numTournySteps         = 0;

bool                drawFoodSensorData     = false;
bool                drawEnemySensorData    = false;
bool                drawEnergySensorData   = false;


lastMatchWonBy lastWinner = NO_WINNER;


vector <neuronDrawingData> genome1NeuronRenderData;
vector <linkDrawingData>   genome1LinkRenderData;
vector <neuronDrawingData> genome2NeuronRenderData;
vector <linkDrawingData>   genome2LinkRenderData;


const int    neuronRadiusSize         = 6;
const int    selectedNeuronRadiusSize = 14;
unsigned int selectedNeuronIndex      = 0;
bool         neuronSelected           = false;




string dnaFileName1;
string dnaFileName2;

string vcrFileName;

string roundRobinPlayerFileName;


GameWorld  *gameWorld;

CGenome    *genome1;
CGenome    *genome2;

CNeuralNet *genome1Brain;
CNeuralNet *genome2Brain;

RobotAgent *robotAgent1;
RobotAgent *robotAgent2;


vector <roundRobinPlayerData> roundRobinPlayerVec;
queue  <roundRobinJob>        roundRobinJobQueue;

int numRoundRobinPlayers;
int numRoundRobinGames;







/**********************************************************************
             OpenGL Graphics and Glut callback functions
*********************************************************************/

// Keyboard presses end up here for processing
void keyboardFunction(unsigned char key, int x, int y)
{
//      cout<<"In keyboard callback with: "<<key<<endl;

    switch (key)
    {
    case 27: // Escape key
        cout<<"Exiting the program."<<endl;
        DONE = true;
        glutLeaveMainLoop();
        break;

    case 'p':
        if (PAUSE)
        {
            cout<<"Unpausing..."<<endl;
            PAUSE = false;
        }
        else
        {
            cout<<"Pausing..."<<endl;
            PAUSE = true;
        }
        break;

    case 'f':
        timeMode = fastforward;
        break;

    case 'r':
        timeMode = realtime;
        break;

    case 'm':
        timeMode = accelerated;
        break;

    case 't':
        if (mode != VCR_REPLAY)
        {
            tournamentStep = 0;
            mode = TOURNY_EVAL;
        }
        break;

    case 's':
        if (mode != VCR_REPLAY)
        {
            if ((mode == LOADED_READY) ||
                    (mode == DRAW_GENOME1) ||
                    (mode == DRAW_GENOME2))
            {
                return;
            }

            if (PAUSE)
            {
                PAUSE = false;
            }

            START = true;
        }
        break;

    case '1':
        mode = DRAW_GENOME1;
        break;

    case '2':
        mode = DRAW_GENOME2;
        break;

    default:
        break;
    }
}



// Sees if a neurons was left clicked on in brain
// drawing mode
void checkForNeuronClick(int mouseX, int mouseY)
{
    vector <neuronDrawingData> *neuronData;
    CVec3 clickPos, neuronPos;

    clickPos.x = mouseX;
    clickPos.y = mouseY;
    clickPos.z = 0.0;

    neuronSelected = false;



    switch (mode)
    {
    case DRAW_GENOME1:
        neuronData = &genome1NeuronRenderData;
        break;

    case DRAW_GENOME2:
        neuronData = &genome2NeuronRenderData;
        break;

    default:
        cout<<"In checkForNeuronClick, invalid mode!"<<endl;
        DONE = true;
        exit(1);
    }

    int numNeurons = neuronData->size();

    for (int i = 0; i < numNeurons; i++)
    {
        neuronPos.x = neuronData->at(i).posX;
        neuronPos.y = neuronData->at(i).posY;
        neuronPos.z = 0.0;

        // We selected a neuron!
        if (clickPos.Distance(neuronPos) <= neuronRadiusSize)
        {
            cout<<"Clicked on neuron: "<<neuronData->at(i).neuronID<<endl;
            selectedNeuronIndex = i;
            neuronSelected      = true;
        }
    }
}



// Mouse clicks end up here for processing
void mouseFunction(GLint button, GLint action, GLint xMouse, GLint yMouse)
{
    switch (mode)
    {
    case LOADING:
    case LOADED_READY:
    case STANDARD_EVAL:
    case TOURNY_EVAL:
    case VCR_REPLAY:
    case ROUND_ROBIN:
        break;


    case DRAW_GENOME1: // Allow left clicking on neurons in brain drawing mode
    case DRAW_GENOME2:
        if ((button == GLUT_LEFT_BUTTON) && (action == GLUT_DOWN))
        {
            // Y axis is reversed from glut to my software....
            checkForNeuronClick(xMouse, abs(yMouse - (BOARDSIZE + (2 * BOARDBUFFER))));
        }
        break;

    default:
        cout<<"Invalid mode in mouseFunctions"<<endl;
        DONE = true;
        exit(1);
    }
}






// React to menu option selection
void processMenuEvents(int menuOption)
{
    switch (menuOption)
    {
    case EXIT_ACTION:
        cout<<"Got menu exit!"<<endl;
        DONE = true;
        glutLeaveMainLoop();
        break;

    case SET_STANDARD_MATCH_ACTION:
        mode = STANDARD_EVAL;

        // Green robot == host in this programs context
        if (greenRobotStartLeft)
        {
            gameWorld->resetForNewRun(hostStartLeft, robotAgent1, robotAgent2, standardFoodLayout, 0, 0);
        }
        else
        {
            gameWorld->resetForNewRun(hostStartRight, robotAgent1, robotAgent2, standardFoodLayout, 0, 0);
        }
        break;

    case GREEN_START_LEFT:
        greenRobotStartLeft = true;
        break;

    case GREEN_START_RIGHT:
        greenRobotStartLeft = false;
        break;

    case SET_TOURNY_MATCH_ACTION:
        tournamentStep = 0;
        mode = TOURNY_EVAL;
        break;

    case START_ACTION:
        if ((mode == LOADED_READY) ||
            (mode == DRAW_GENOME1) ||
            (mode == DRAW_GENOME2))
        {
            return;
        }

        if (PAUSE)
        {
            PAUSE = false;
        }

        START = true;
        break;

    case PAUSE_ACTION:
        if (PAUSE)
        {
            PAUSE = false;
        }
        else
        {
            PAUSE = true;
        }
        break;

    case REALTIME_ACTION:
        timeMode = realtime;
        break;

    case FAST_FORWARD_ACTION:
        timeMode = fastforward;
        break;

    case MAX_SPEED_ACTION:
        timeMode = accelerated;
        break;

    case DRAW_GENOME1_ACTION:
        mode = DRAW_GENOME1;
        break;

    case DRAW_GENOME2_ACTION:
        mode = DRAW_GENOME2;
        break;

    case RESET_MODE_ACTION:
        mode = LOADED_READY;
        break;

    case DRAW_FOOD_SENSOR_ACTION:
        if (drawFoodSensorData)
        {
            drawFoodSensorData = false;
        }
        else
        {
            drawFoodSensorData   = true;
            drawEnemySensorData  = false;
            drawEnergySensorData = false;
        }
        break;

    case DRAW_ENERGY_SENSOR_ACTION:
        if (drawEnergySensorData)
        {
            drawEnergySensorData = false;
        }
        else
        {
            drawEnergySensorData = true;
            drawEnemySensorData  = false;
            drawFoodSensorData   = false;
        }
        break;

    case DRAW_ENEMY_SENSOR_ACTION:
        if (drawEnemySensorData)
        {
            drawEnemySensorData = false;
        }
        else
        {
            drawEnemySensorData  = true;
            drawEnergySensorData = false;
            drawFoodSensorData   = false;
        }
        break;

    default:
        cout<<"In processMenuEvents, invalid menu option: "<<menuOption<<endl;
        DONE = true;
        exit(1);
    }
}



// This creates the menu
void createGlutMenu()
{
    int menu;

    // Create the menu, and tell glut
    // that processMenuEvents handles
    // events
    menu = glutCreateMenu(processMenuEvents);

    // add entries to menu
    glutAddMenuEntry("Pause/Unpause", PAUSE_ACTION);
    glutAddMenuEntry("Set Realtime Speed", REALTIME_ACTION);
    glutAddMenuEntry("Set Fast Forward Speed", FAST_FORWARD_ACTION);
    glutAddMenuEntry("Set Max Speed", MAX_SPEED_ACTION);

    if (mode != VCR_REPLAY)
    {
        glutAddMenuEntry("Green Robot Start Left", GREEN_START_LEFT);
        glutAddMenuEntry("Green Robot Start Right", GREEN_START_RIGHT);
        glutAddMenuEntry("Set Standard Match Mode", SET_STANDARD_MATCH_ACTION);
        glutAddMenuEntry("Set Tournament Mode", SET_TOURNY_MATCH_ACTION);
        glutAddMenuEntry("Start Match/Matches", START_ACTION);
    }

    glutAddMenuEntry("Draw Enemy Sensor Data", DRAW_ENEMY_SENSOR_ACTION);
    glutAddMenuEntry("Draw Food Sensor Data", DRAW_FOOD_SENSOR_ACTION);
    glutAddMenuEntry("Draw Energy Sensor Data", DRAW_ENERGY_SENSOR_ACTION);


    if (mode != VCR_REPLAY)
    {
        glutAddMenuEntry("Render Genome 1", DRAW_GENOME1_ACTION);
        glutAddMenuEntry("Render Genome 2", DRAW_GENOME2_ACTION);
        glutAddMenuEntry("Reset Mode", RESET_MODE_ACTION);
    }

    glutAddMenuEntry("Exit", EXIT_ACTION);

    // Attache the menu to the right button
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}




void graphicsInitialization()
{
    // Sets the display window color to grey
    glClearColor (0.5, 0.5, 0.5, 0.0);

    // Sets the projection parameters
    glMatrixMode (GL_PROJECTION);

    // Specifies the coordinate of the clipping planes
    gluOrtho2D (0.0, (float)(BOARDSIZE + (2 * BOARDBUFFER)), 0.0, (float)(BOARDSIZE + (2 * BOARDBUFFER)));
}






// Draw all the food items
void drawFood()
{
    // Set food color
    glColor3f(1.0, 0.584, 0.475);

    if (DRAW_ALL_FOOD_TEST)
    {
        // Test mode to draw all possible food positions
        CVec3 allFoodData[21];

        for (int i = 0; i < 21; i++)
        {
            allFoodData[i].z = 0.0;
        }

        allFoodData[0].x = 300.0;
        allFoodData[0].y = 540.0;

        allFoodData[1].x = 180.0;
        allFoodData[1].y = 420.0;

        allFoodData[2].x = 420.0;
        allFoodData[2].y = 420.0;

        allFoodData[3].x = 60.0;
        allFoodData[3].y = 300.0;

        allFoodData[4].x = 540.0;
        allFoodData[4].y = 300.0;

        allFoodData[5].x = 180.0;
        allFoodData[5].y = 180.0;

        allFoodData[6].x = 420.0;
        allFoodData[6].y = 180.0;

        allFoodData[7].x = 240.0;
        allFoodData[7].y = 60.0;

        allFoodData[8].x = 360.0;
        allFoodData[8].y = 60.0;

        allFoodData[9].x  =  60.0;
        allFoodData[9].y  = 480.0;

        allFoodData[10].x  = 180.0;
        allFoodData[10].y  = 480.0;

        allFoodData[11].x  = 420.0;
        allFoodData[11].y  = 480.0;

        allFoodData[12].x  = 540.0;
        allFoodData[12].y  = 480.0;

        allFoodData[13].x  = 180.0;
        allFoodData[13].y  = 360.0;

        allFoodData[14].x  = 420.0;
        allFoodData[14].y  = 360.0;

        allFoodData[15].x  = 180.0;
        allFoodData[15].y  = 240.0;

        allFoodData[16].x  = 420.0;
        allFoodData[16].y  = 240.0;

        allFoodData[17].x  =  60.0;
        allFoodData[17].y  = 120.0;

        allFoodData[18].x  = 180.0;
        allFoodData[18].y  = 120.0;

        allFoodData[19].x = 420.0;
        allFoodData[19].y = 120.0;

        allFoodData[20].x = 540.0;
        allFoodData[20].y = 120.0;

        glPushMatrix();
        {
            for (int i = 0; i < 21; i++)
            {
                // Make sure the "extra" food pieces are a
                // slightly different color
                if (i >= 9)
                {
                    glColor3f(0.75, 0.784, 0.475);
                }


                if (!renderFoodData[i].foodEaten)
                {
                    glRecti(allFoodData[i].x + BOARDBUFFER - 10,
                            allFoodData[i].y + BOARDBUFFER - 4,
                            allFoodData[i].x + BOARDBUFFER + 10,
                            allFoodData[i].y + BOARDBUFFER + 4);
                }
            }
        }
        glPopMatrix();
    }
    else
    {
        glPushMatrix();
        {
            for (int i = 0; i < numFoodPieces; i++)
            {
                // Make sure the "extra" food pieces are a
                // slightly different color
                if (i >= 9)
                {
                    glColor3f(0.75, 0.784, 0.475);
                }


                if (!renderFoodData[i].foodEaten)
                {
                    glRecti(renderFoodData[i].foodPosition.x + BOARDBUFFER - 10,
                            renderFoodData[i].foodPosition.y + BOARDBUFFER - 4,
                            renderFoodData[i].foodPosition.x + BOARDBUFFER + 10,
                            renderFoodData[i].foodPosition.y + BOARDBUFFER + 4);
                }
            }
        }
        glPopMatrix();
    }
}



// Cycle through all the
// agents, and draw them
void drawAgents()
{
    static int    num_segments     = 50;
    static float  radius           = 10.0;

    static double sensorLineLength = 50.0;

    // Set agent color
    glColor3f(0.0, 1.0, 0.0);

    // green agent
    glPushMatrix();
    {
        glTranslatef(renderGreenAgent.agentPos.x + BOARDBUFFER, renderGreenAgent.agentPos.y + BOARDBUFFER, 0.0);
        glRotatef(renderGreenAgent.agentHdg, 0.0, 0.0, -1.0);

        glBegin(GL_LINE_LOOP);
        {
            for (int i = 0; i < num_segments; i++)
            {
                // Get the current angle
                float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);

                // Calc the x and y component
                float x = radius * cosf(theta);
                float y = radius * sinf(theta);

                glVertex2f(x, y);
            }
        }
        glEnd();

        // Draw the nose line
        glBegin(GL_LINES);
        {
            glVertex2i(0, radius);
            glVertex2i(0, radius+7);
        }
        glEnd();


        // Draw representations of the brain inputs for
        // enemy sensor
        if (drawEnemySensorData)
        {
            glColor3f(0.0, 0.0, 1.0);

            // front
            glBegin(GL_LINES);
            {
                glVertex2f(1.0, 0.0);
                glVertex2f(1.0, greenAgentSensorData.enemySensorForward * sensorLineLength);
            }
            glEnd();

            // Front left
            glRotatef(-45.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, greenAgentSensorData.enemySensorLeft * sensorLineLength);
            }
            glEnd();

            // Front right
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, greenAgentSensorData.enemySensorRight * sensorLineLength);
            }
            glEnd();

            // back right
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, greenAgentSensorData.enemySensorBackRight * sensorLineLength);
            }
            glEnd();

            // back left
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, greenAgentSensorData.enemySensorBackLeft * sensorLineLength);
            }
            glEnd();
        }

        if (drawFoodSensorData)
        {
            // Set food color
            glColor3f(1.0, 0.584, 0.475);

            // front
            glBegin(GL_LINES);
            {
                glVertex2f(1.0, 0.0);
                glVertex2f(1.0, greenAgentSensorData.foodSensorForward * sensorLineLength);
            }
            glEnd();

            // Front left
            glRotatef(-45.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, greenAgentSensorData.foodSensorLeft * sensorLineLength);
            }
            glEnd();

            // Front right
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, greenAgentSensorData.foodSensorRight * sensorLineLength);
            }
            glEnd();

            // back right
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, greenAgentSensorData.foodSensorBackRight * sensorLineLength);
            }
            glEnd();

            // back left
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, greenAgentSensorData.foodSensorBackLeft * sensorLineLength);
            }
            glEnd();
        }

        if (drawEnergySensorData)
        {
            glColor3f(0.0, 0.0, 1.0);

            glBegin(GL_LINES);
            {
                glVertex2f(1.0, radius);
                glVertex2f(1.0, radius + (greenAgentSensorData.energyDifferenceSensor * sensorLineLength));
            }
            glEnd();
        }
    }
    glPopMatrix();



    // Set agent color
    glColor3f(1.0, 0.0, 0.0);


    // red agent
    glPushMatrix();
    {
        glTranslatef(renderRedAgent.agentPos.x + BOARDBUFFER, renderRedAgent.agentPos.y + BOARDBUFFER, 0.0);
        glRotatef(renderRedAgent.agentHdg, 0.0, 0.0, -1.0);

        glBegin(GL_LINE_LOOP);
        {
            for (int i = 0; i < num_segments; i++)
            {
                // Get the current angle
                float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);

                // Calc the x and y component
                float x = radius * cosf(theta);
                float y = radius * sinf(theta);

                glVertex2f(x, y);
            }
        }
        glEnd();

        // Draw the nose line
        glBegin(GL_LINES);
        {
            glVertex2i(0, radius);
            glVertex2i(0, radius+7);
        }
        glEnd();

        // Draw representations of the brain inputs for
        // enemy sensor
        if (drawEnemySensorData)
        {
            glColor3f(0.0, 0.0, 1.0);

            // front
            glBegin(GL_LINES);
            {
                glVertex2f(1.0, 0.0);
                glVertex2f(1.0, redAgentSensorData.enemySensorForward * sensorLineLength);
            }
            glEnd();

            // Front left
            glRotatef(-45.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, redAgentSensorData.enemySensorLeft * sensorLineLength);
            }
            glEnd();

            // Front right
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, redAgentSensorData.enemySensorRight * sensorLineLength);
            }
            glEnd();

            // back right
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, redAgentSensorData.enemySensorBackRight * sensorLineLength);
            }
            glEnd();

            // back left
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, redAgentSensorData.enemySensorBackLeft * sensorLineLength);
            }
            glEnd();

            glRotatef(135.0, 0.0, 0.0, -1.0);
        }

        if (drawFoodSensorData)
        {
            // Set food color
            glColor3f(1.0, 0.584, 0.475);

            // front
            glBegin(GL_LINES);
            {
                glVertex2f(1.0, 0.0);
                glVertex2f(1.0, redAgentSensorData.foodSensorForward * sensorLineLength);
            }
            glEnd();

            // Front left
            glRotatef(-45.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, redAgentSensorData.foodSensorLeft * sensorLineLength);
            }
            glEnd();

            // Front right
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, redAgentSensorData.foodSensorRight * sensorLineLength);
            }
            glEnd();

            // back right
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, redAgentSensorData.foodSensorBackRight * sensorLineLength);
            }
            glEnd();

            // back left
            glRotatef(90.0, 0.0, 0.0, -1.0);
            glBegin(GL_LINES);
            {
                glVertex2f(0.0, 0.0);
                glVertex2f(0.0, redAgentSensorData.foodSensorBackLeft * sensorLineLength);
            }
            glEnd();

            glRotatef(135.0, 0.0, 0.0, -1.0);

        }

        if (drawEnergySensorData)
        {
            glColor3f(0.0, 0.0, 1.0);

            glBegin(GL_LINES);
            {
                glVertex2f(1.0, radius);
                glVertex2f(1.0, radius + (redAgentSensorData.energyDifferenceSensor * sensorLineLength));
            }
            glEnd();
        }
    }
    glPopMatrix();
}






// Draw the board, and stats
void drawWorld()
{
    // Draw the text data
    // green agent food info
    glColor3f(1.0, 1.0, 1.0);
    stringstream greenAgentFoodString;
    greenAgentFoodString << "Green Agent food eaten: " << greenAgentNumFoodEaten;

    glRasterPos2i(10, 35);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)greenAgentFoodString.str().c_str());


    // red agent food info
    stringstream redAgentFoodString;
    redAgentFoodString << "Red Agent food eaten: " << redAgentNumFoodEaten;

    glRasterPos2i(10, 10);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)redAgentFoodString.str().c_str());





    // Draw the text showing the genome ID's loaded
    stringstream firstGenomeIdString;
    stringstream secondGenomeIdString;

    firstGenomeIdString << "Genome 1 ID: "<<firstGenomeID;
    secondGenomeIdString<< "Genome 2 ID: "<<secondGenomeID;

    glColor3f(0.0, 1.0, 0.0);
    glRasterPos2i(250, 35);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)firstGenomeIdString.str().c_str());

    glColor3f(1.0, 0.2, 0.2);
    glRasterPos2i(250, 10);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)secondGenomeIdString.str().c_str());





    glColor3f(1.0, 1.0, 1.0);
    // If the system is paused, display it on the top center
    if (PAUSE)
    {
        string pauseString = "Paused";
        glRasterPos2i(310, 680);
        glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)pauseString.c_str());
    }






    // Display the winner of the last match
    string winnerLabel = "Last match won by: ";
    string winnerName;

    glRasterPos2i(250, 660);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)winnerLabel.c_str());

    switch (lastWinner)
    {
    case GREEN_WINNER:
        winnerName = "GREEN Robot";
        break;

    case RED_WINNER:
        winnerName = "RED Robot";
        break;

    case NO_WINNER:
        winnerName = "No last winner";
        break;
    }

    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)winnerName.c_str());



    // Display the system mode on top left
    string modeString;
    string modeLabelString = "MODE: ";
    switch (mode)
    {
    case LOADING:
        modeString = "Loading";
        break;

    case LOADED_READY:
        modeString = "Robots Ready";
        break;

    case STANDARD_EVAL:
        modeString = "Standard Evaluation";
        break;

    case TOURNY_EVAL:
        modeString = "Tournament Evaluation";
        break;

    case VCR_REPLAY:
        modeString = "VCR Replay";
        break;

    case ROUND_ROBIN:
        modeString = "Round Robin background";

    default:
        cout<<"Error in drawWorld, invalid mode: "<<(int)mode<<endl;
        DONE = true;
        exit(1);
    }
    glRasterPos2i(60, 665);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)modeLabelString.c_str());
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)modeString.c_str());






    // Display the time mode on the top right
    string timeModeString;
    string timeModeLabelString = "TIME MODE: ";
    switch (timeMode)
    {
    case realtime:
        timeModeString = "Realtime";
        break;

    case accelerated:
        timeModeString = "Maximum Speed";
        break;

    case fastforward:
        timeModeString = "Fast Forward";
        break;

    default:
        cout<<"Error in drawWorld, invalid time mode: "<<(int)timeMode<<endl;
        DONE = true;
        exit(1);
    }

    glRasterPos2i(500, 685);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)timeModeLabelString.c_str());
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)timeModeString.c_str());

    // And display calculated time in seconds
    glRasterPos2i(500, 655);
    stringstream timeSecondsStream;

    timeSecondsStream <<setprecision(4) << "Seconds: "<<((float)ticks/(float)param_numTicks) * 30,0;
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)timeSecondsStream.str().c_str());



    // If mode is tournament evaluation, show progress through match steps
    if (mode == TOURNY_EVAL || mode == VCR_REPLAY)
    {
        stringstream tournyStepStringStream;
        tournyStepStringStream<<"Tournament step: "<<tournamentStep<<" of "<<numTournySteps<<" steps";
        glRasterPos2i(500, 670);
        glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)tournyStepStringStream.str().c_str());
    }





    // Draw the Green agent score data
    stringstream greenAgentScoreStringStream;
    greenAgentScoreStringStream << "Green Agent Score: "<<greenAgentScore;
    glRasterPos2i(520, 35);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)greenAgentScoreStringStream.str().c_str());

    // Draw the Red agent score data
    stringstream redAgentScoreStringStream;
    redAgentScoreStringStream << "Red Agent Score: "<<redAgentScore;
    glRasterPos2i(520, 10);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)redAgentScoreStringStream.str().c_str());







    // Tell which, if any, sensors are being rendered on the robots
    glColor3f(1.0, 1.0, 1.0);
    string sensorDrawLabel = "Sensor";

    glRasterPos2i(652, 620);
    glutBitmapString(GLUT_BITMAP_HELVETICA_10, (const unsigned char*)sensorDrawLabel.c_str());

    sensorDrawLabel = "Drawn:";
    glRasterPos2i(652, 605);
    glutBitmapString(GLUT_BITMAP_HELVETICA_10, (const unsigned char*)sensorDrawLabel.c_str());

    if (drawFoodSensorData)
    {
        sensorDrawLabel = "FOOD";
    }
    else if (drawEnemySensorData)
    {
        sensorDrawLabel = "ENEMY";
    }
    else if (drawEnergySensorData)
    {
        sensorDrawLabel = "ENERGY";
    }
    else
    {
        sensorDrawLabel = "NONE";
    }

    glRasterPos2i(652, 590);
    glutBitmapString(GLUT_BITMAP_HELVETICA_10, (const unsigned char*)sensorDrawLabel.c_str());







    // Board wall color
    glColor3f(0.0, 0.0, 1.0);

    // And draw the rectangle walls
    glPushMatrix();
    {
        glBegin(GL_LINE_LOOP);
        {
            glVertex2i(BOARDBUFFER, BOARDBUFFER);
            glVertex2i(BOARDSIZE + BOARDBUFFER, BOARDBUFFER);
            glVertex2i(BOARDSIZE + BOARDBUFFER, BOARDSIZE + BOARDBUFFER);
            glVertex2i(BOARDBUFFER, BOARDSIZE + BOARDBUFFER);
        }
        glEnd();
    }
    glPopMatrix();


    // Draw side energy bar
    // label
    glColor3f(1.0, 1.0, 1.0);
    string energyBarLabel = "Energy";
    glRasterPos2i(10, 620);
    glutBitmapString(GLUT_BITMAP_HELVETICA_10, (const unsigned char*)energyBarLabel.c_str());

    // Draw the side data
    // The green agent energy bar
    glColor3f(0.0, 1.0, 0.0);
    // Bar outline
    glBegin(GL_LINE_LOOP);
    {
        glVertex2i(5,  100);
        glVertex2i(5,  600);
        glVertex2i(20, 600);
        glVertex2i(20, 100);
    }
    glEnd();

    // green agent energy bar filled
    glRecti(5, 100, 20, (int)(greenAgentEnergy/FULLENERGY * 500 + 100));


    // The red agent energy bar
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    {
        glVertex2i(30, 100);
        glVertex2i(30, 600);
        glVertex2i(45, 600);
        glVertex2i(45, 100);
    }
    glEnd();

    // red agent energy bar filled
    glRecti(30, 100, 45, (int)(redAgentEnergy/FULLENERGY * 500 + 100));
}




// Meh. It's ugly, but it works.
void drawCircle(int circleX, int circleY, bool selected)
{
    GLUquadricObj *quadric;

    quadric = gluNewQuadric();

    glTranslated(circleX, circleY, 0);

    if (selected)
    {
        gluDisk(quadric, selectedNeuronRadiusSize - 2.0, selectedNeuronRadiusSize, 15, 15);
    }
    else
    {
        gluDisk(quadric, 0.0, neuronRadiusSize, 15, 15);
    }

    glTranslated(-circleX, -circleY, 0);
    gluDeleteQuadric(quadric);
}





// This function renders the neural network
// Wow. I'm terrible at hand coding openGL. Apologies
// to anyone having to suffer through this mess.
void drawBrain()
{
    static int numNeurons;
    static int numLinks;
    static int genomeID;

    static int numHiddenNodes;

    static bool genomeIsGreenRobot;

    static int lengthInputOuputLinkLines = 20;

    vector <neuronDrawingData> *neuronData;
    vector <linkDrawingData>   *linkData;

    switch (mode)
    {
    case DRAW_GENOME1:
        numNeurons         = genome1Brain->getNumberOfNeurons();
        numLinks           = genome1Brain->getNumberOfConnections();
        numHiddenNodes     = genome1Brain->getNumberOfHiddenNodes();
        neuronData         = &genome1NeuronRenderData;
        linkData           = &genome1LinkRenderData;
        genomeID           = genome1Brain->getGenomeID();
        genomeIsGreenRobot = true;
        break;

    case DRAW_GENOME2:
        numNeurons         = genome2Brain->getNumberOfNeurons();
        numLinks           = genome2Brain->getNumberOfConnections();
        numHiddenNodes     = genome2Brain->getNumberOfHiddenNodes();
        neuronData         = &genome2NeuronRenderData;
        linkData           = &genome2LinkRenderData;
        genomeID           = genome2Brain->getGenomeID();
        genomeIsGreenRobot = false;
        break;

    default:
        cout<<"Error in drawBrain, invalid mode"<<endl;
        DONE = true;
        exit(1);
    }


    glPushMatrix();
    {
        // Draw the links
        for (int i = 0; i < numLinks; i++)
        {
            // Check for links to or from selected neurons
            if (neuronSelected)
            {
                if (((neuronData->at(selectedNeuronIndex).posX == linkData->at(i).startX) &&
                     (neuronData->at(selectedNeuronIndex).posY == linkData->at(i).startY)) ||
                     ((neuronData->at(selectedNeuronIndex).posX == linkData->at(i).endX) &&
                     (neuronData->at(selectedNeuronIndex).posY == linkData->at(i).endY)))
                {
                    // The link connects to the selected neuron!
                    glColor3f(1.0, 0.2, 0.2);
                    glLineWidth(2);
                }
                else
                {
                    glColor3f(1.0, 1.0, 1.0);
                    glLineWidth(1);
                }
            }
            else
            {
                glColor3f(1.0, 1.0, 1.0);
                glLineWidth(1);
            }

            // Draw recurrent links as dashed lines between neurons, or if it loops back
            // on itself, then as an empty circle
            if (linkData->at(i).recurrent)
            {
                // If the recurrent link is a loop back onto the same neuron, use an open circle to draw it
                if ((linkData->at(i).startX == linkData->at(i).endX) &&
                        (linkData->at(i).startY == linkData->at(i).endY))
                {
                    drawCircle(linkData->at(i).startX - selectedNeuronRadiusSize, linkData->at(i).startY, true);
                }
                else
                {
                    // It's a recurrent link between two different neurons.
                    glEnable(GL_LINE_STIPPLE);
                    glLineStipple(1, 0xF0F0);
                    glBegin(GL_LINES);
                    {
                        glVertex2i(linkData->at(i).startX, linkData->at(i).startY);
                        glVertex2i(linkData->at(i).endX,   linkData->at(i).endY);
                    }
                    glEnd();
                    glDisable(GL_LINE_STIPPLE);
                }
            }
            else
            {
                glBegin(GL_LINES);
                {
                    glVertex2i(linkData->at(i).startX, linkData->at(i).startY);
                    glVertex2i(linkData->at(i).endX,   linkData->at(i).endY);
                }
                glEnd();
            }
        }


        glLineWidth(1);

        // Draw the neurons
        for (int i = 0; i < numNeurons; i++)
        {
            switch (neuronData->at(i).neuronType)
            {
            case input:
                // Draw input line...
                glColor3f(1.0, 1.0, 1.0);

                glBegin(GL_LINES);
                glVertex2i(neuronData->at(i).posX, neuronData->at(i).posY);
                glVertex2i(neuronData->at(i).posX, neuronData->at(i).posY-lengthInputOuputLinkLines);
                glEnd();

                // Set color for neuron
                glColor3f(0.0, 1.0, 0.0);
                break;

            case hidden:
                glColor3f(0.0, 1.0, 1.0);
                break;

            case output:
                // Draw output line...
                glColor3f(1.0, 1.0, 1.0);

                glBegin(GL_LINES);
                glVertex2i(neuronData->at(i).posX, neuronData->at(i).posY);
                glVertex2i(neuronData->at(i).posX, neuronData->at(i).posY+lengthInputOuputLinkLines);
                glEnd();

                // Set color for neuron
                glColor3f(0.0, 0.0, 1.0);
                break;

            case bias:
                glColor3f(1.0, 1.0, 0.0);
                break;

            default:
                cout<<"Invalid neuron type in drawBrain!"<<endl;
                DONE = true;
                exit(1);
            }

            if (neuronSelected && (selectedNeuronIndex == i))
            {
                drawCircle(neuronData->at(i).posX, neuronData->at(i).posY, true);
            }
            else
            {
                drawCircle(neuronData->at(i).posX, neuronData->at(i).posY, false);
            }
        }
    }
    glPopMatrix();


    // Now draw the text displays
    stringstream genomeIdString;
    genomeIdString << "Showing Neural Network for Genome ID: "<<genomeID;
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2i(10, 680);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)genomeIdString.str().c_str());

    if (genomeIsGreenRobot)
    {
        string colorString = "Green Robot Genome/Neural Network";
        glColor3f(0.0, 1.0, 0.0);
        glRasterPos2i(10, 660);
        glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)colorString.c_str());
    }
    else
    {
        string colorString = "Red Robot Genome/Neural Network";
        glColor3f(1.0, 0.0, 0.0);
        glRasterPos2i(10, 660);
        glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)colorString.c_str());
    }

    glColor3f(1.0, 1.0, 1.0);
    stringstream numNodesString;
    numNodesString << "Number of neurons: "<<numNeurons;
    glRasterPos2i(10, 640);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)numNodesString.str().c_str());

    stringstream numHiddenNodesString;
    numHiddenNodesString << "Number of hidden neurons: "<<numHiddenNodes;
    glRasterPos2i(10, 620);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)numHiddenNodesString.str().c_str());

    stringstream numLinksString;
    numLinksString << "Number of connections: "<<numLinks;
    glRasterPos2i(10, 600);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)numLinksString.str().c_str());

    int numRecurrentLinks = 0;
    for (int i = 0; i < numLinks; i++)
    {
        if (linkData->at(i).recurrent)
        {
            numRecurrentLinks++;
        }
    }
    stringstream numRecurrentLinksString;
    numRecurrentLinksString << "Number of recurrent links: "<<numRecurrentLinks;
    glRasterPos2i(10, 580);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)numRecurrentLinksString.str().c_str());



    // Draw the "legend" for neurons
    string legendLabel;

    // input
    glColor3f(0.0, 1.0, 0.0);
    drawCircle(550, 680, false);
    legendLabel = "= Input Neuron";
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2i(560, 677);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)legendLabel.c_str());


    // hidden
    glColor3f(0.0, 1.0, 1.0);
    drawCircle(550, 660, false);
    legendLabel = "= Hidden Neuron";
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2i(560, 657);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)legendLabel.c_str());

    // output
    glColor3f(0.0, 0.0, 1.0);
    drawCircle(550, 640, false);
    legendLabel = "= Output Neuron";
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2i(560, 637);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)legendLabel.c_str());

    // bias
    glColor3f(1.0, 1.0, 0.0);
    drawCircle(550, 620, false);
    legendLabel = "= Bias Neuron";
    glColor3f(1.0, 1.0, 1.0);
    glRasterPos2i(560, 617);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)legendLabel.c_str());



    // input/outputs label above/below neurons
    string borderLabel;

    glColor3f(1.0, 1.0, 1.0);
    borderLabel = "INPUTS";
    glRasterPos2i(320, 12);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)borderLabel.c_str());

    borderLabel = "OUTPUTS";
    glRasterPos2i(320, 600);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)borderLabel.c_str());
}





// This is the primary drawing function,
void displayFunc()
{
    static struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = nanoSleepAmount;

    static stringstream messageStringStream;

    glClear(GL_COLOR_BUFFER_BIT);

    if (DRAW_ALL_FOOD_TEST)
    {
        drawWorld();
        drawFood();
        glutPostRedisplay();
        glutSwapBuffers();
        nanosleep(&sleepTime, NULL);
        return;
    }

    switch (mode)
    {
    case LOADING:
        glColor3f(1.0, 1.0, 1.0);
        messageStringStream.str("Loading...");
        glRasterPos2i(200, 350);
        glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)messageStringStream.str().c_str());
        break;

    case LOADED_READY:
        glColor3f(1.0, 1.0, 1.0);
        messageStringStream.str("DNA loaded into Genomes, ready for test setup.");
        glRasterPos2i(200, 350);
        glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)messageStringStream.str().c_str());
        drawWorld();
        break;

    case STANDARD_EVAL:
    case TOURNY_EVAL:
    case VCR_REPLAY:
        drawWorld();
        drawFood();
        drawAgents();
        break;

    case DRAW_GENOME1:
    case DRAW_GENOME2:
        drawBrain();
        break;

    case ROUND_ROBIN:
        glColor3f(1.0, 1.0, 1.0);
        messageStringStream.str("Round Robin, no rendering...");
        glRasterPos2i(200, 350);
        glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)messageStringStream.str().c_str());
        break;

    default:
        cout<<"Invalid mode in displayfunc: "<<(int)mode<<endl;
        DONE = true;
        exit(1);
        break;
    }



    glutPostRedisplay();
    glutSwapBuffers();
    nanosleep(&sleepTime, NULL);
}







// This is the glut idle function callback
void idle()
{
    static struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = nanoSleepAmount;

    nanosleep(&sleepTime, NULL);
}

/**********************************************************************
          End OpenGL and Glut functions
*********************************************************************/













void parseArguments(int argc, char *argv[])
{
    switch (argc)
    {
    case 2: // load a single VCR recording to replay
        vcrFileName = argv[1];
        cout<<"Loading VCR file for replay:"<<endl;
        cout<<vcrFileName<<endl;
        mode  = VCR_REPLAY;
        PAUSE = true;
        break;

    case 3: // Load two genomes for testing
        dnaFileName1 = argv[1];
        dnaFileName2 = argv[2];
        cout<<"Loading program with genome dna files:"<<endl;
        cout<<"1- "<<dnaFileName1<<endl;
        cout<<"2- "<<dnaFileName2<<endl;
        break;

    case 4: // load player list file for massive round robin tournament
        roundRobinPlayerFileName = argv[3];
        mode = ROUND_ROBIN;
        cout<<"Loading round robin player list file: "<<roundRobinPlayerFileName<<endl;
        break;

    default:
        cout<<"Usage: genomeTester [dna file name 1] [dna file name 2]"<<endl;
        cout<<"Or for vcr replay: genomeTester [vcr file name]"<<endl;
        cout<<"Or for round robin tournaments with player file: genomeTester ROUND ROBIN [player file name]"<<endl;
        exit(1);
    }
}





// Goes through both neural networks,
// and gets all the data needed for
// drawing the brains.
void setupBrainDrawingData()
{
    SNeuron* neuronPointer;
    neuronDrawingData newNeuronData;
    linkDrawingData   newLinkData;
    const int topRenderingBuffer = 100;


    int numNeurons = genome1Brain->getNumberOfNeurons();

    for (int i = 0; i < numNeurons; i++)
    {
        neuronPointer = genome1Brain->getPointerToNeuron(i);

        newNeuronData.posX               = (neuronPointer->dSplitX * BOARDSIZE) + BOARDBUFFER;
        newNeuronData.posY               = (neuronPointer->dSplitY * (BOARDSIZE-topRenderingBuffer)) + BOARDBUFFER;
        newNeuronData.neuronType         = neuronPointer->NeuronType;
        newNeuronData.activationResponse = neuronPointer->dActivationResponse;
        newNeuronData.neuronID           = neuronPointer->iNeuronID;

        genome1NeuronRenderData.push_back(newNeuronData);

        // Get the links data, based on outgoing links
        int numLinks = neuronPointer->vecLinksOut.size();
        for (int i = 0; i < numLinks; i++)
        {
            newLinkData.startX    = newNeuronData.posX;
            newLinkData.startY    = newNeuronData.posY;
            newLinkData.endX      = (neuronPointer->vecLinksOut.at(i).pOut->dSplitX * BOARDSIZE) + BOARDBUFFER;
            newLinkData.endY      = (neuronPointer->vecLinksOut.at(i).pOut->dSplitY * (BOARDSIZE-topRenderingBuffer)) + BOARDBUFFER;
            newLinkData.recurrent = neuronPointer->vecLinksOut.at(i).bRecurrent;
            newLinkData.weight    = neuronPointer->vecLinksOut.at(i).dWeight;

            genome1LinkRenderData.push_back(newLinkData);
        }
    }

    numNeurons = genome2Brain->getNumberOfNeurons();

    for (int i = 0; i < numNeurons; i++)
    {
        neuronPointer = genome2Brain->getPointerToNeuron(i);

        newNeuronData.posX               = (neuronPointer->dSplitX * BOARDSIZE) + BOARDBUFFER;
        newNeuronData.posY               = (neuronPointer->dSplitY * (BOARDSIZE-topRenderingBuffer)) + BOARDBUFFER;
        newNeuronData.neuronType         = neuronPointer->NeuronType;
        newNeuronData.activationResponse = neuronPointer->dActivationResponse;
        newNeuronData.neuronID           = neuronPointer->iNeuronID;

        genome2NeuronRenderData.push_back(newNeuronData);

        // Get the links data, based on outgoing links
        int numLinks = neuronPointer->vecLinksOut.size();
        for (int i = 0; i < numLinks; i++)
        {
            newLinkData.startX    = newNeuronData.posX;
            newLinkData.startY    = newNeuronData.posY;
            newLinkData.endX      = (neuronPointer->vecLinksOut.at(i).pOut->dSplitX * BOARDSIZE) + BOARDBUFFER;
            newLinkData.endY      = (neuronPointer->vecLinksOut.at(i).pOut->dSplitY * (BOARDSIZE-topRenderingBuffer)) + BOARDBUFFER;
            newLinkData.recurrent = neuronPointer->vecLinksOut.at(i).bRecurrent;
            newLinkData.weight    = neuronPointer->vecLinksOut.at(i).dWeight;

            genome2LinkRenderData.push_back(newLinkData);
        }
    }
}






// Simulation thread
void *runSimulationThread(void*)
{
    static struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = 1000000;

    struct timespec longSleepTime;
    longSleepTime.tv_sec  = 0;
    longSleepTime.tv_nsec = 10000000000;


    Timer delayTimeTimer;

    int lastTournamentStep = -1;


    if (mode == ROUND_ROBIN)
    {
        Timer roundRobinTimer;
        roundRobinTimer.reset();

        roundRobinJob currentJob;
        int gameCounter = 0;

        bool gameWorldCreated = false;

        while (!roundRobinJobQueue.empty())
        {
            gameCounter++;
            cout<<"******************************************"<<endl;
            cout<<"Starting game "<<gameCounter<<" of "<<numRoundRobinGames<<endl;
            cout<<"   Proportion done: "<<(double)gameCounter/(double)numRoundRobinGames<<endl;
            cout<<"   Total time so far: "<<roundRobinTimer.total()<<endl<<endl;


            currentJob = roundRobinJobQueue.front();
            roundRobinJobQueue.pop();

            roundRobinPlayerVec[currentJob.playerOneIndex].genome->DeletePhenotype();
            roundRobinPlayerVec[currentJob.playerTwoIndex].genome->DeletePhenotype();

            roundRobinPlayerVec[currentJob.playerOneIndex].brain =
                    roundRobinPlayerVec[currentJob.playerOneIndex].genome->CreatePhenotype();

            roundRobinPlayerVec[currentJob.playerTwoIndex].brain =
                    roundRobinPlayerVec[currentJob.playerTwoIndex].genome->CreatePhenotype();

            roundRobinPlayerVec[currentJob.playerOneIndex].brain->tidyXSPlitsForNeuralNetwork();
            roundRobinPlayerVec[currentJob.playerTwoIndex].brain->tidyXSPlitsForNeuralNetwork();

            roundRobinPlayerVec[currentJob.playerOneIndex].robotAgent->insertNewBrain(roundRobinPlayerVec[currentJob.playerOneIndex].brain);
            roundRobinPlayerVec[currentJob.playerTwoIndex].robotAgent->insertNewBrain(roundRobinPlayerVec[currentJob.playerTwoIndex].brain);

            cout<<"Match between indexes: "<<currentJob.playerOneIndex<<", "<<currentJob.playerTwoIndex<<endl;
            cout<<"GenomeIDs: "<<roundRobinPlayerVec[currentJob.playerOneIndex].robotAgent->getBrainGenomeID()<<", "<<roundRobinPlayerVec[currentJob.playerTwoIndex].robotAgent->getBrainGenomeID()<<endl;

            if (!gameWorldCreated)
            {
                gameWorld = new GameWorld(hostStartLeft, roundRobinPlayerVec[currentJob.playerOneIndex].robotAgent,
                                          roundRobinPlayerVec[currentJob.playerTwoIndex].robotAgent,
                                          standardFoodLayout, 0, 0);
                gameWorldCreated = true;
            }

            ticks          = 0;
            tournamentStep = 0;

            int firstBotTotalMatchScore  = 0;
            int secondBotTotalMatchScore = 0;

            // Run the matches for a single game
            while (tournamentStep < numTournySteps)
            {
                if (lastTournamentStep != tournamentStep)
                {
                    // setup for new tournament step
                    tournamentData matchData;
                    matchData.greenRobotStartLeft     = tournyMatchData.at(tournamentStep).greenRobotStartLeft;
                    matchData.foodLayout              = tournyMatchData.at(tournamentStep).foodLayout;
                    matchData.firstExtraFoodPosition  = tournyMatchData.at(tournamentStep).firstExtraFoodPosition;
                    matchData.secondExtraFoodPosition = tournyMatchData.at(tournamentStep).secondExtraFoodPosition;

                    ticks = 0;

                    if (matchData.greenRobotStartLeft)
                    {
                        gameWorld->resetForNewRun(hostStartLeft,
                                                  roundRobinPlayerVec[currentJob.playerOneIndex].robotAgent,
                                                  roundRobinPlayerVec[currentJob.playerTwoIndex].robotAgent,
                                                  matchData.foodLayout,
                                                  matchData.firstExtraFoodPosition,
                                                  matchData.secondExtraFoodPosition);
                    }
                    else
                    {
                        gameWorld->resetForNewRun(hostStartRight,
                                                  roundRobinPlayerVec[currentJob.playerOneIndex].robotAgent,
                                                  roundRobinPlayerVec[currentJob.playerTwoIndex].robotAgent,
                                                  matchData.foodLayout,
                                                  matchData.firstExtraFoodPosition,
                                                  matchData.secondExtraFoodPosition);
                    }

                    lastTournamentStep = tournamentStep;
                }

                if (ticks < param_numTicks)
                {
                    if (gameWorld->update())
                    {
                        // game ended with a winner
                        int firstBotScore  = gameWorld->getHostFinalScore();
                        int secondBotScore = gameWorld->getParasiteFinalScore();

                        if ((firstBotScore == 1) && (secondBotScore == 0))
                        {
                            firstBotTotalMatchScore++;
                        }
                        else if ((firstBotScore == 0) && (secondBotScore == 1))
                        {
                            secondBotTotalMatchScore++;
                        }
                        else
                        {
                            cout<<"Error, impossible score in round robin match!"<<endl;
                            exit(1);
                        }

                        ticks = param_numTicks;
                        tournamentStep++;
                    }
                }
                else
                {
                    // tie, game expired
                    tournamentStep++;
                }
                ticks++;
            }

            // Check match score, add it to structure for that robot,
            // move onto next queued job....

            cout<<"Done, P1 score: "<<firstBotTotalMatchScore<<", P2 score: "<<secondBotTotalMatchScore<<endl;
            if (firstBotTotalMatchScore > secondBotTotalMatchScore)
            {
                roundRobinPlayerVec[currentJob.playerOneIndex].wins++;
                roundRobinPlayerVec[currentJob.playerTwoIndex].losses++;
            }
            else if (firstBotTotalMatchScore < secondBotTotalMatchScore)
            {
                roundRobinPlayerVec[currentJob.playerOneIndex].losses++;
                roundRobinPlayerVec[currentJob.playerTwoIndex].wins++;
            }
            else if (firstBotTotalMatchScore == secondBotTotalMatchScore)
            {
                roundRobinPlayerVec[currentJob.playerOneIndex].ties++;
                roundRobinPlayerVec[currentJob.playerTwoIndex].ties++;
            }
            else
            {
                cout<<"Error, basic laws of math has been broken. Pretty impressive screwup."<<endl;
                exit(1);
            }
        }

        // Round robin finished!
        cout<<endl;
        cout<<"Round robin results:"<<endl;
        for (unsigned int i = 0; i < roundRobinPlayerVec.size(); i++)
        {
            cout<<"Player: "<<i+1<<", wins/losses/ties: "<<roundRobinPlayerVec[i].wins<<"/"
               <<roundRobinPlayerVec[i].losses<<"/"
                 <<roundRobinPlayerVec[i].ties<<endl;
            cout<<"      Total games: "<<roundRobinPlayerVec[i].wins + roundRobinPlayerVec[i].losses + roundRobinPlayerVec[i].ties<<endl;
        }

        cout<<endl;
        cout<<"CSV style dump: "<<endl;

        for (unsigned int i = 0; i < roundRobinPlayerVec.size(); i++)
        {
            cout<<i+1<<", "<<roundRobinPlayerVec[i].wins<<", "<<roundRobinPlayerVec[i].losses<<", "<<roundRobinPlayerVec[i].ties<<endl;
        }

        cout<<"Done with Round Robin Tournament."<<endl;

        DONE = true;
        exit(0);
    }
    else
    {
        // Setup the players and gameworld....
        genome1 = new CGenome();

        if (genome1->ReadGenomeFromFile(dnaFileName1))
        {
            cout<<"Successfully read in genomeID "<<genome1->ID()<<" for the first robot"<<endl;
            firstGenomeID = genome1->ID();
        }
        else
        {
            cout<<"Failed to read dna file: "<<dnaFileName1<<endl;
            exit(1);
        }


        genome2 = new CGenome();

        if (genome2->ReadGenomeFromFile(dnaFileName2))
        {
            cout<<"Successfully read in genomeID "<<genome2->ID()<<" for the second robot"<<endl;
            secondGenomeID = genome2->ID();
        }
        else
        {
            cout<<"Failed to read dna file: "<<dnaFileName2<<endl;
            exit(1);
        }

        genome1Brain = genome1->CreatePhenotype();
        genome2Brain = genome2->CreatePhenotype();


        // This is to assist in rendering the networks
        // later
        genome1Brain->tidyXSPlitsForNeuralNetwork();
        genome2Brain->tidyXSPlitsForNeuralNetwork();

        robotAgent1  = new RobotAgent();
        robotAgent2  = new RobotAgent();

        robotAgent1->insertNewBrain(genome1Brain);
        robotAgent2->insertNewBrain(genome2Brain);


        // Setup the drawing data for the brains. Since brains
        // are static, only need to do this once.
        setupBrainDrawingData();


        // Green robot == host in this programs context
        // Go ahead and initialize gameworld. Settings will likely be changed through the menu later
        // with resetForNewRun calls.
        gameWorld = new GameWorld(hostStartLeft, robotAgent1, robotAgent2, standardFoodLayout, 0, 0);

        mode = LOADED_READY;


        while (!DONE)
        {
            // We're loaded, but waiting for options
            // selection from the menu
            while (mode == LOADED_READY)
            {
                ticks = 0;
                nanosleep(&longSleepTime, NULL);
            }

            // Standard eval is the mode to execute
            while (mode == STANDARD_EVAL)
            {
                // Wait till Start is selected from
                // the menu
                while (!START)
                {
                    greenAgentScore = 0;
                    redAgentScore   = 0;
                    nanosleep(&longSleepTime, NULL);
                }

                // See if we're paused...
                if (PAUSE)
                {
                    nanosleep(&longSleepTime, NULL);
                    continue;
                }

                // Check to see if we're in realtime mode, or accelerated
                if (timeMode == realtime)
                {
                    if (delayTimeTimer.total() >= realtimeDelay)
                    {
                        delayTimeTimer.reset();
                    }
                    else
                    {
                        nanosleep(&sleepTime, NULL);
                        continue;
                    }
                }
                else if (timeMode == fastforward) // And if we're in fast forward mode...
                {
                    if (delayTimeTimer.total() >= fastForwardDelay)
                    {
                        delayTimeTimer.reset();
                    }
                    else
                    {
                        nanosleep(&sleepTime, NULL);
                        continue;
                    }
                }

                // Now, actually do the gameworld updates...
                if (ticks < param_numTicks)
                {
                    if (gameWorld->update())
                    {
                        greenAgentScore = gameWorld->getHostFinalScore();
                        redAgentScore   = gameWorld->getParasiteFinalScore();

                        if ((greenAgentScore == 1) && (redAgentScore == 0))
                        {
                            lastWinner = GREEN_WINNER;
                        }
                        else if ((greenAgentScore == 0) && (redAgentScore == 1))
                        {
                            lastWinner = RED_WINNER;
                        }
                        else
                        {
                            cout<<"Error, impossible scores on early end, green robot: "<<greenAgentScore<<", red robot: "<<redAgentScore<<endl;
                            exit(1);
                        }

                        ticks = param_numTicks;

                        START = false;
                        mode  = LOADED_READY;
                    }
                    else // get data for rendering
                    {
                        renderGreenAgent = gameWorld->getHostPosition();
                        renderRedAgent   = gameWorld->getParasitePosition();

                        numFoodPieces = gameWorld->getNumFoodPieces();
                        for (int i = 0; i < numFoodPieces; i++)
                        {
                            renderFoodData[i] = gameWorld->getFoodData(i);
                        }

                        greenAgentNumFoodEaten = gameWorld->getNumFoodEaten(HOST);
                        redAgentNumFoodEaten   = gameWorld->getNumFoodEaten(PARASITE);

                        greenAgentSensorData   = gameWorld->getRobotSensorValues(HOST);
                        redAgentSensorData     = gameWorld->getRobotSensorValues(PARASITE);

                        greenAgentEnergy       = gameWorld->getRobotEnergy(HOST);
                        redAgentEnergy         = gameWorld->getRobotEnergy(PARASITE);
                    }
                }
                else
                {
    //                greenAgentScore = gameWorld->getHostFinalScore();
    //                redAgentScore   = gameWorld->getParasiteFinalScore();
    //                renderGreenAgent = gameWorld->getHostPosition();
    //                renderRedAgent   = gameWorld->getParasitePosition();

                    lastWinner = NO_WINNER;
                    START      = false;
                    mode       = LOADED_READY;
                }

                ticks++;
            }

            // Tournament mode is what we should execute
            while (mode == TOURNY_EVAL)
            {
                // Wait till Start is selected from
                // the menu
                while (!START)
                {
                    ticks                  = 0;
                    tournamentStep         = 0;
                    greenAgentScore        = 0;
                    redAgentScore          = 0;
                    greenAgentNumFoodEaten = 0;
                    redAgentNumFoodEaten   = 0;
                    nanosleep(&longSleepTime, NULL);
                }

                // See if we're paused...
                if (PAUSE)
                {
                    nanosleep(&longSleepTime, NULL);
                    continue;
                }

                // Check to see if we're in realtime mode, or accelerated
                if (timeMode == realtime)
                {
                    if (delayTimeTimer.total() >= realtimeDelay)
                    {
                        delayTimeTimer.reset();
                    }
                    else
                    {
                        nanosleep(&sleepTime, NULL);
                        continue;
                    }
                }
                else if (timeMode == fastforward) // And if we're in fast forward mode...
                {
                    if (delayTimeTimer.total() >= fastForwardDelay)
                    {
                        delayTimeTimer.reset();
                    }
                    else
                    {
                        nanosleep(&sleepTime, NULL);
                        continue;
                    }
                }


                // Setup for new tournament step
                if (lastTournamentStep != tournamentStep)
                {
                    if (tournamentStep == numTournySteps)
                    {
                        cout<<"Finished all tournament match steps"<<endl;
                    }
                    else
                    {
                        tournamentData matchData;
                        matchData.greenRobotStartLeft     = tournyMatchData.at(tournamentStep).greenRobotStartLeft;
                        matchData.foodLayout              = tournyMatchData.at(tournamentStep).foodLayout;
                        matchData.firstExtraFoodPosition  = tournyMatchData.at(tournamentStep).firstExtraFoodPosition;
                        matchData.secondExtraFoodPosition = tournyMatchData.at(tournamentStep).secondExtraFoodPosition;

                        ticks = 0;

                        if (matchData.greenRobotStartLeft)
                        {
                            gameWorld->resetForNewRun(hostStartLeft,
                                                      robotAgent1,
                                                      robotAgent2,
                                                      matchData.foodLayout,
                                                      matchData.firstExtraFoodPosition,
                                                      matchData.secondExtraFoodPosition);
                        }
                        else
                        {
                            gameWorld->resetForNewRun(hostStartRight,
                                                      robotAgent1,
                                                      robotAgent2,
                                                      matchData.foodLayout,
                                                      matchData.firstExtraFoodPosition,
                                                      matchData.secondExtraFoodPosition);
                        }
                    }

                    lastTournamentStep = tournamentStep;
                }

                // Run the tournament matches
                if (tournamentStep < numTournySteps)
                {
                    if (ticks < param_numTicks)
                    {
                        if (gameWorld->update())
                        {
                            int greenAgentMatchScore = gameWorld->getHostFinalScore();;
                            int redAgentMatchScore   = gameWorld->getParasiteFinalScore();

                            if ((greenAgentMatchScore == 1) && (redAgentMatchScore == 0))
                            {
                                lastWinner = GREEN_WINNER;
                            }
                            else if ((greenAgentMatchScore == 0) && (redAgentMatchScore == 1))
                            {
                                lastWinner = RED_WINNER;
                            }
                            else
                            {
                                cout<<"Error, impossible scores on early end, green robot: "<<greenAgentMatchScore<<", red robot: "<<redAgentMatchScore<<endl;
                                exit(1);
                            }

                            greenAgentScore += greenAgentMatchScore;
                            redAgentScore   += redAgentMatchScore;

                            ticks = param_numTicks;
                            tournamentStep++;
                        }
                        else // get rendering data
                        {
                            renderGreenAgent = gameWorld->getHostPosition();
                            renderRedAgent   = gameWorld->getParasitePosition();

                            numFoodPieces = gameWorld->getNumFoodPieces();
                            for (int i = 0; i < numFoodPieces; i++)
                            {
                                renderFoodData[i] = gameWorld->getFoodData(i);
                            }

                            greenAgentNumFoodEaten = gameWorld->getNumFoodEaten(HOST);
                            redAgentNumFoodEaten   = gameWorld->getNumFoodEaten(PARASITE);

                            greenAgentSensorData   = gameWorld->getRobotSensorValues(HOST);
                            redAgentSensorData     = gameWorld->getRobotSensorValues(PARASITE);

                            greenAgentEnergy       = gameWorld->getRobotEnergy(HOST);
                            redAgentEnergy         = gameWorld->getRobotEnergy(PARASITE);
                        }
                    }
                    else
                    {
    //                    greenAgentScore += gameWorld->getHostFinalScore();
    //                    redAgentScore   += gameWorld->getParasiteFinalScore();
                        lastWinner = NO_WINNER;
                        tournamentStep++;
                    }
                    ticks++;
                }
                else
                {
                    // All tournament matches complete
                    // Actually, this check will be hit by the lastTournamentStep != tournamentStep
                    // code above first.
                    START = false;
                    mode = LOADED_READY;
                }
            }
        }
    }
}







// Thread to read in data from
// VCR recording file
void *runVcrRecordingThread(void*)
{
    static struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = 1000000;

    struct timespec longSleepTime;
    longSleepTime.tv_sec  = 0;
    longSleepTime.tv_nsec = 10000000000;


    Timer delayTimeTimer;

    QFile vcrFile(vcrFileName.c_str());
    vcrFile.open(QIODevice::ReadOnly);

    QDataStream vcrDataStream(&vcrFile);
//    vcrDataStream.setVersion(QDataStream::Qt_4_6);
    vcrDataStream.setVersion(QDataStream::Qt_4_4);

    quint16 generation;
    quint16 tick;
    quint32 genChampGenomeID;
    quint32 dominantStratGenomeID;
    quint8  dominantStratNumber;
    quint8  numberFoodPieces;
    quint8  genChampNumFoodEaten;
    quint8  domStratNumFoodEaten;
    quint16 genChampEnergy;
    quint16 domStratEnergy;
    quint8  genChampScore;
    quint8  domStratScore;

    float genChampHeading;
    float genChampPosX;
    float genChampPosY;

    float domStratHeading;
    float domStratPosX;
    float domStratPosY;

    float genChampFoodSensorForward;
    float genChampFoodSensorLeft;
    float genChampFoodSensorRight;
    float genChampFoodSensorBackLeft;
    float genChampFoodSensorBackRight;
    float genChampEnemySensorForward;
    float genChampEnemySensorLeft;
    float genChampEnemySensorRight;
    float genChampEnemySensorBackLeft;
    float genChampEnemySensorBackRight;
    float genChampWallSensor;
    float genChampEnergyDiffSensor;

    float domStratFoodSensorForward;
    float domStratFoodSensorLeft;
    float domStratFoodSensorRight;
    float domStratFoodSensorBackLeft;
    float domStratFoodSensorBackRight;
    float domStratEnemySensorForward;
    float domStratEnemySensorLeft;
    float domStratEnemySensorRight;
    float domStratEnemySensorBackLeft;
    float domStratEnemySensorBackRight;
    float domStratWallSensor;
    float domStratEnergyDiffSensor;

    float foodPosX[MAX_FOOD_PIECES];
    float foodPosY[MAX_FOOD_PIECES];
    bool  foodEaten[MAX_FOOD_PIECES];

    genChampScore = 0;
    domStratScore = 0;

    delayTimeTimer.reset();

    while (!DONE)
    {
        // Check for timing delays, and pauses
        if (PAUSE)
        {
            nanosleep(&longSleepTime, NULL);
            continue;
        }

        if (timeMode == realtime)
        {
            if (delayTimeTimer.total() >= realtimeDelay)
            {
                delayTimeTimer.reset();
            }
            else
            {
                nanosleep(&sleepTime, NULL);
                continue;
            }
        }
        else if (timeMode == fastforward)
        {
            if (delayTimeTimer.total() >= fastForwardDelay)
            {
                delayTimeTimer.reset();
            }
            else
            {
                nanosleep(&sleepTime, NULL);
                continue;
            }
        }


        // read the next tick record from the file
        if (!vcrDataStream.atEnd())
        {
            vcrDataStream >> generation
                          >> tick
                          >> genChampGenomeID
                          >> dominantStratGenomeID
                          >> dominantStratNumber
                          >> numberFoodPieces
                          >> genChampNumFoodEaten
                          >> domStratNumFoodEaten
                          >> genChampEnergy
                          >> domStratEnergy
                          >> genChampScore
                          >> domStratScore
                          >> genChampHeading
                          >> genChampPosX
                          >> genChampPosY
                          >> domStratHeading
                          >> domStratPosX
                          >> domStratPosY
                          >> genChampFoodSensorForward
                          >> genChampFoodSensorLeft
                          >> genChampFoodSensorRight
                          >> genChampFoodSensorBackLeft
                          >> genChampFoodSensorBackRight
                          >> genChampEnemySensorForward
                          >> genChampEnemySensorLeft
                          >> genChampEnemySensorRight
                          >> genChampEnemySensorBackLeft
                          >> genChampEnemySensorBackRight
                          >> genChampWallSensor
                          >> genChampEnergyDiffSensor
                          >> domStratFoodSensorForward
                          >> domStratFoodSensorLeft
                          >> domStratFoodSensorRight
                          >> domStratFoodSensorBackLeft
                          >> domStratFoodSensorBackRight
                          >> domStratEnemySensorForward
                          >> domStratEnemySensorLeft
                          >> domStratEnemySensorRight
                          >> domStratEnemySensorBackLeft
                          >> domStratEnemySensorBackRight
                          >> domStratWallSensor
                          >> domStratEnergyDiffSensor;

            for (int i = 0; i < numberFoodPieces; i++)
            {
                vcrDataStream >> foodPosX[i]
                              >> foodPosY[i]
                              >> foodEaten[i];
            }


            // Plop the data into rendering data
            renderGreenAgent.agentHdg   = (double)genChampHeading;
            renderGreenAgent.agentPos.x = (double)genChampPosX;
            renderGreenAgent.agentPos.y = (double)genChampPosY;

            renderRedAgent.agentHdg   = (double)domStratHeading;
            renderRedAgent.agentPos.x = (double)domStratPosX;
            renderRedAgent.agentPos.y = (double)domStratPosY;

            numFoodPieces = (int)numberFoodPieces;
            for (int i = 0; i < numFoodPieces; i++)
            {
                renderFoodData[i].foodPosition.x = (double)foodPosX[i];
                renderFoodData[i].foodPosition.y = (double)foodPosY[i];
                renderFoodData[i].foodEaten      = foodEaten[i];

            }

            greenAgentNumFoodEaten = (int)genChampNumFoodEaten;
            redAgentNumFoodEaten   = (int)domStratNumFoodEaten;

            greenAgentEnergy = (int)genChampEnergy;
            redAgentEnergy   = (int)domStratEnergy;

            greenAgentScore += (int)genChampScore;
            redAgentScore   += (int)domStratScore;

            greenAgentSensorData.foodSensorForward      = (double)genChampFoodSensorForward;
            greenAgentSensorData.foodSensorLeft         = (double)genChampFoodSensorLeft;
            greenAgentSensorData.foodSensorRight        = (double)genChampFoodSensorRight;
            greenAgentSensorData.foodSensorBackLeft     = (double)genChampFoodSensorBackLeft;
            greenAgentSensorData.foodSensorBackRight    = (double)genChampFoodSensorBackRight;
            greenAgentSensorData.enemySensorForward     = (double)genChampEnemySensorForward;
            greenAgentSensorData.enemySensorLeft        = (double)genChampEnemySensorLeft;
            greenAgentSensorData.enemySensorRight       = (double)genChampEnemySensorRight;
            greenAgentSensorData.enemySensorBackLeft    = (double)genChampEnemySensorBackLeft;
            greenAgentSensorData.enemySensorBackRight   = (double)genChampEnemySensorBackRight;
            greenAgentSensorData.wallSensor             = (double)genChampWallSensor;
            greenAgentSensorData.energyDifferenceSensor = (double)genChampEnergyDiffSensor;

            redAgentSensorData.foodSensorForward        = (double)domStratFoodSensorForward;
            redAgentSensorData.foodSensorLeft           = (double)domStratFoodSensorLeft;
            redAgentSensorData.foodSensorRight          = (double)domStratFoodSensorRight;
            redAgentSensorData.foodSensorBackLeft       = (double)domStratFoodSensorBackLeft;
            redAgentSensorData.foodSensorBackRight      = (double)domStratFoodSensorBackRight;
            redAgentSensorData.enemySensorForward       = (double)domStratEnemySensorForward;
            redAgentSensorData.enemySensorLeft          = (double)domStratEnemySensorLeft;
            redAgentSensorData.enemySensorRight         = (double)domStratEnemySensorRight;
            redAgentSensorData.enemySensorBackLeft      = (double)domStratEnemySensorBackLeft;
            redAgentSensorData.enemySensorBackRight     = (double)domStratEnemySensorBackRight;
            redAgentSensorData.wallSensor               = (double)domStratWallSensor;
            redAgentSensorData.energyDifferenceSensor   = (double)domStratEnergyDiffSensor;

            firstGenomeID  = (int)genChampGenomeID;
            secondGenomeID = (int)dominantStratGenomeID;

            ticks = (int)tick;

            if (tick == 0)
            {
                tournamentStep++;
            }
        }
        else
        {
            cout<<"Reached end of vcr file: "<<vcrFileName.c_str()<<endl;
            vcrFile.close();
            DONE = true;
        }
    }
}







// Setup the data needed for a whole tournament
// round
void setupTournyMatchDataVector()
{
    tournamentData matchData;

    // Setup the matches using the standard food layout
    matchData.greenRobotStartLeft     = true;
    matchData.foodLayout              = standardFoodLayout;
    matchData.firstExtraFoodPosition  = 0;
    matchData.secondExtraFoodPosition = 0;

    tournyMatchData.push_back(matchData);



    matchData.greenRobotStartLeft     = false;
    matchData.foodLayout              = standardFoodLayout;
    matchData.firstExtraFoodPosition  = 0;
    matchData.secondExtraFoodPosition = 0;

    tournyMatchData.push_back(matchData);


    // Add the 12 matches (x2 for both side start) for the
    // single extra food piece
    for (int i = 0; i < MAX_EXTRA_FOOD_PIECES; i++)
    {
        matchData.greenRobotStartLeft     = true;
        matchData.foodLayout              = singleExtraFoodLayout;
        matchData.firstExtraFoodPosition  = i;
        matchData.secondExtraFoodPosition = 0;

        tournyMatchData.push_back(matchData);



        matchData.greenRobotStartLeft     = false;
        matchData.foodLayout              = singleExtraFoodLayout;
        matchData.firstExtraFoodPosition  = i;
        matchData.secondExtraFoodPosition = 0;

        tournyMatchData.push_back(matchData);
    }


    // Add the 66 matches (x2 for both side start) for the two
    // additional food pieces
    for (int i = 0; i < MAX_EXTRA_FOOD_PIECES; i++)
    {
        for (int j = i+1; j < MAX_EXTRA_FOOD_PIECES; j++)
        {
            matchData.greenRobotStartLeft     = true;
            matchData.foodLayout              = doubleExtraFoodLayout;
            matchData.firstExtraFoodPosition  = i;
            matchData.secondExtraFoodPosition = j;

            tournyMatchData.push_back(matchData);

            matchData.greenRobotStartLeft     = false;
            matchData.foodLayout              = doubleExtraFoodLayout;
            matchData.firstExtraFoodPosition  = i;
            matchData.secondExtraFoodPosition = j;

            tournyMatchData.push_back(matchData);
        }
    }

    numTournySteps = tournyMatchData.size();
}






void readRoundRobinFile()
{
    ifstream playerFileStream;
    playerFileStream.open(roundRobinPlayerFileName.c_str(), ios::in);

    if (playerFileStream.fail())
    {
        cout<<"readRoundRobinFile, Error opening player file: "<<roundRobinPlayerFileName<<endl;
        exit(1);
    }

    roundRobinPlayerData newPlayerData;

    playerFileStream >> numRoundRobinPlayers;


    for (int i = 0; i < numRoundRobinPlayers; i++)
    {
        playerFileStream  >> newPlayerData.dnaFileName;

        newPlayerData.genome     = new CGenome();
        newPlayerData.robotAgent = new RobotAgent();
        newPlayerData.wins       = 0;
        newPlayerData.losses     = 0;
        newPlayerData.ties       = 0;

        if (!newPlayerData.genome->ReadGenomeFromFile(newPlayerData.dnaFileName))
        {
            cout<<"Error reading dna file into genome in readRoundRobinFile!"<<endl;
            exit(1);
        }

        roundRobinPlayerVec.push_back(newPlayerData);
    }

    numRoundRobinGames = ((double)numRoundRobinPlayers/2.0) * (numRoundRobinPlayers-1);



    cout<<"Round Robin mode, read in player list of size: "<<roundRobinPlayerVec.size()<<endl;
    cout<<"Number of games required: "<<numRoundRobinGames<<endl;
}





void setupRoundRobinTournamentQueue()
{
    roundRobinJob newRoundRobinJob;


    for (int i = 0; i < numRoundRobinPlayers; i++)
    {
        for (int j = i + 1; j < numRoundRobinPlayers; j++)
        {
            newRoundRobinJob.playerOneIndex = i;
            newRoundRobinJob.playerTwoIndex = j;

            roundRobinJobQueue.push(newRoundRobinJob);
        }
    }

    cout<<"At end of round robin scheduling, num games is: "<<roundRobinJobQueue.size()<<endl;
}






int main(int argc, char *argv[])
{
    cout<<"Genome Tester program starting..."<<endl;

    if (!DRAW_ALL_FOOD_TEST)
    {
        parseArguments(argc, argv);
    }

    if (mode == ROUND_ROBIN)
    {
        cout<<"In round robin tournament mode"<<endl;
        readRoundRobinFile();
        setupRoundRobinTournamentQueue();
    }


    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(BOARDSIZE + (2 * BOARDBUFFER), BOARDSIZE + (2 * BOARDBUFFER));
    glutCreateWindow("DAK Genome Tester Visualization");
    graphicsInitialization();

    // Register my GLUT callbacks for input,
    // display, menus, and calculation
    glutMouseFunc(mouseFunction);
    glutKeyboardFunc(keyboardFunction);
    createGlutMenu();
    glutDisplayFunc(displayFunc);
    glutIdleFunc(idle);


    // pthread stuffs for running the simulated world
    pthread_t simulationThread;
    pthread_t vcrRecordingReaderThread;

    // Setup the vector that stores the data about the
    // match setup for tournaments
    setupTournyMatchDataVector();


    // If we're just replaying a recording, we don't
    // need to simulate anything
    if (mode != VCR_REPLAY)
    {
        // Create simulation thread
        if (!DRAW_ALL_FOOD_TEST)
        {
            pthread_create(&simulationThread, NULL, runSimulationThread, NULL);
        }
    }
    else
    {
        // Create thread to read in recorded data
        // from VCR recording file
        pthread_create(&vcrRecordingReaderThread, NULL, runVcrRecordingThread, NULL);
    }

    // And.... go!
    glutMainLoop();


    // Cleanup from simulation
    if (mode != VCR_REPLAY)
    {
        delete gameWorld;
        delete robotAgent1;
        delete robotAgent2;
        delete genome1Brain;
        delete genome2Brain;
        delete genome1;
        delete genome2;
    }

    cout<<"Exiting Genome Tester program"<<endl;
    return 0;
}
