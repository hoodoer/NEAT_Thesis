#ifndef GAMEWORLD_H
#define GAMEWORLD_H

#include "time.h"
#include "SVector2D.h"
#include "mathVector.h"
#include "mathMatrix.h"
#include "timer.h"
#include "robotAgent.h"
#include "phenotype.h"
#include "parameters.h"
#include "lineIntersect2D.h"


#define RAD2DEG                    57.29578

#define BOARDSIZE                  600
#define BOARDBUFFER                50

#define ROBOT_COLLISION_DISTANCE   20.0
#define FOOD_COLLISION_DISTANCE    18.0
#define WALL_COLLISION_DISTANCE    10.0

#define FULLENERGY                 2500.00
#define FOODENERGYBOOST            500.0

#define CONSTANT_BIAS_INPUT        1.0

#define LEFT_POS_START_X           200
#define LEFT_POS_START_Y           300
#define LEFT_POS_START_HDG         270.0

#define RIGHT_POS_START_X          400
#define RIGHT_POS_START_Y          300
#define RIGHT_POS_START_HDG        90.0

#define MAX_FOOD_PIECES            12
#define MAX_EXTRA_FOOD_PIECES      12


#define SENSOR_OVERLAP             5.0
#define FORWARD_SENSOR_SPREAD      30.0
#define FORWARD_SIDE_SENSOR_SPREAD 75.0
#define BACK_SENSOR_SPREAD         90.0
#define NUM_SENSORS                5

#define FORWARD_SENSOR             0
#define FRONT_LEFT_SENSOR          1
#define FRONT_RIGHT_SENSOR         2
#define BACK_LEFT_SENSOR           3
#define BACK_RIGHT_SENSOR          4


// These should NOT take into account
// the border/buffer area around
// the game box
const int MIN_POS_X = 0 + WALL_COLLISION_DISTANCE;
const int MAX_POS_X = BOARDSIZE - WALL_COLLISION_DISTANCE;
const int MIN_POS_Y = 0 + WALL_COLLISION_DISTANCE;
const int MAX_POS_Y = BOARDSIZE - WALL_COLLISION_DISTANCE;


const double FIRST_SENSOR_BOUNDARY  = (FORWARD_SENSOR_SPREAD/2.0) - SENSOR_OVERLAP;
const double SECOND_SENSOR_BOUNDARY = FIRST_SENSOR_BOUNDARY + (2.0 * SENSOR_OVERLAP);
const double THIRD_SENSOR_BOUNDARY  = ((FORWARD_SENSOR_SPREAD/2.0) +
                                      FORWARD_SIDE_SENSOR_SPREAD - SENSOR_OVERLAP);
const double FOURTH_SENSOR_BOUNDARY = THIRD_SENSOR_BOUNDARY + (2.0 * SENSOR_OVERLAP);
const double FIFTH_SENSOR_BOUNDARY  = (((FORWARD_SENSOR_SPREAD/2.0) +
                                       FORWARD_SIDE_SENSOR_SPREAD + BACK_SENSOR_SPREAD) -
                                      SENSOR_OVERLAP);


#define MAX_DISTANCE 848.53



enum hostStartPosition
{
    hostStartLeft,
    hostStartRight
};


enum foodLayoutType
{
    standardFoodLayout,
    singleExtraFoodLayout,
    doubleExtraFoodLayout
};


enum agentType
{
    HOST,
    PARASITE
};





struct singlePieceFoodData
{
    CVec3 foodPosition;
    bool  foodEaten;
};




struct foodSensorData
{
    double distance;
    double bearing;
    bool   foodSet;
};


struct agentPositionData
{
    CVec3     agentPos;
    double    agentHdg;
};




// Useful for testing, contains all the info
// needed to render a single tick of a match.
// Setup for testing dominance tournament matches
struct domTournyRenderingDataType
{
    int   renderGeneration;
    int   renderTick;
    int   renderGenChampGenomeID;
    int   renderDominantStratGenomeID;
    int   renderDominantStratNumber;
    int   renderNumFoodPieces;
    int   renderGenChampNumFoodEaten;
    int   renderDominantStratNumFoodEaten;
    int   renderGenChampEnergy;
    int   renderDominantStratEnergy;
    int   renderGenChampScore;
    int   renderDominantStratScore;

    agentPositionData   renderGenomeChampPosData;
    agentPositionData   renderDominantStratPosData;
    brainInputData      renderGenChampSensorData;
    brainInputData      renderDominantStratSensorData;
    singlePieceFoodData renderFoodData[MAX_FOOD_PIECES];
};






class GameWorld
{
public:
    GameWorld(hostStartPosition startPos,
              RobotAgent *newHostAgent,
              RobotAgent *newParasiteAgent,
              foodLayoutType newFoodLayout,
              int firstExtraFoodPosIndex,
              int secondExtraFoodPosIndex);

    ~GameWorld();

    // Return true if competition over early due to winner,
    // false otherwise
    bool update();


    // Get ready for another run so you don't have
    // so create new game worlds constantly
    void resetForNewRun(hostStartPosition startPos,
                        RobotAgent *newHostAgent,
                        RobotAgent *newParasiteAgent,
                        foodLayoutType newFoodLayout,
                        int firstExtraFoodPosIndex,
                        int secondExtraFoodPosIndex);

    unsigned int getHostFinalScore();
    unsigned int getParasiteFinalScore();


    // These are for rendering and
    // behavior data collection
    agentPositionData   getHostPosition();
    agentPositionData   getParasitePosition();
    singlePieceFoodData getFoodData(unsigned int foodIndex);
    unsigned int        getNumFoodEaten(agentType whichAgent);
    unsigned int        getNumFoodPieces();
    brainInputData      getRobotSensorValues(agentType whichAgent);
    brainOutputData     getRobotOutputValues(agentType whichAgent);
    double              getRobotEnergy(agentType whichAgent);


private:
    void                calculateAgentMovement(agentType whichAgent);

    void                calculateSensorValues();
    void                setEnemyBearingInputs(brainInputData &brainData,
                                              double bearing, double distance);
    void                setFoodBearingInputs(agentType whichAgent);
    void                checkFoodDistanceForSensor(int sensorIndex,
                                                   double testDistance,
                                                   double bearing);
    void                setAgentWallDistance(agentType whichAgent);
    void                getWallIntersectPoint(Line2D aimLine);

    bool                checkForRobotCollision();
    void                checkForFoodCollision();
    void                checkForWallCollision();

    void                setExtraFoodPositionData();


    Timer               simTimer;

    RobotAgent          *hostAgent;
    RobotAgent          *parasiteAgent;

    agentPositionData   hostPosition;
    agentPositionData   parasitePosition;

    hostStartPosition   hostStartingPosition;
    foodLayoutType      foodLayout;
    singlePieceFoodData foodData[MAX_FOOD_PIECES];
    CVec3               extraFoodPosData[MAX_EXTRA_FOOD_PIECES];

    brainInputData      hostBrainInputData;
    brainOutputData     hostBrainOutputData;
    brainInputData      parasiteBrainInputData;
    brainOutputData     parasiteBrainOutputData;

    unsigned int        numFoodPieces;
    unsigned int        hostScore;
    unsigned int        parasiteScore;
    unsigned int        numFoodEatenHost;
    unsigned int        numFoodEatenParasite;

    double              forwardMoveAmount;
    double              turnAngle;
    double              newHeading;

    double              hostEnergy;
    double              parasiteEnergy;
    double              energyUsed;
    double              energyDiffCalcValue;
    double              energyDiffSensorValue;

    CVec3               northVector;
    CVec3               robotHeadingVector;
    CVec3               toParasiteVector;
    CVec3               toHostVector;
    CVec3               toFoodVector;
    CMatrix             rotationMatrix;

    double              enemyBearing;
    double              absEnemyBearing;
    double              enemyDistance;

    double              foodBearing;
    double              absFoodBearing;
    double              foodDistance;
    foodSensorData      foodSensorInfo[NUM_SENSORS];

    Line2D              leftWall;
    Line2D              bottomWall;
    Line2D              rightWall;
    Line2D              topWall;
    Line2D              robotAimLine;
    Point2D             intersectPoint;
    CVec3               robotToWallVector;
    CVec3               robotPositionVector;
    CVec3               intersectPointVector;
    double              robotHeading;
    double              wallDistance;
};

#endif // GAMEWORLD_H
