#include "gameworld.h"




GameWorld::GameWorld(hostStartPosition startPos,
                     RobotAgent *newHostAgent,
                     RobotAgent *newParasiteAgent,
                     foodLayoutType newFoodLayout,
                     int firstExtraFoodPosIndex,
                     int secondExtraFoodPosIndex)
{
    hostStartingPosition = startPos;
    hostAgent            = newHostAgent;
    parasiteAgent        = newParasiteAgent;
    foodLayout           = newFoodLayout;

    hostScore            = 0;
    parasiteScore        = 0;
    forwardMoveAmount    = 0.0;
    turnAngle            = 0.0;
    newHeading           = 0.0;
    hostEnergy           = FULLENERGY;
    parasiteEnergy       = FULLENERGY;
    energyUsed           = 0.0;
    enemyBearing         = 0.0;
    absEnemyBearing      = 0.0;
    enemyDistance        = 0.0;
    foodBearing          = 0.0;
    absFoodBearing       = 0.0;
    foodDistance         = 0.0;
    robotHeading         = 0.0;
    wallDistance         = 0.0;
    numFoodEatenHost     = 0;
    numFoodEatenParasite = 0;


    // Initialize input/output data
    hostBrainInputData.foodSensorForward          = 0.0;
    hostBrainInputData.foodSensorLeft             = 0.0;
    hostBrainInputData.foodSensorRight            = 0.0;
    hostBrainInputData.foodSensorBackLeft         = 0.0;
    hostBrainInputData.foodSensorBackRight        = 0.0;
    hostBrainInputData.enemySensorForward         = 0.0;
    hostBrainInputData.enemySensorLeft            = 0.0;
    hostBrainInputData.enemySensorRight           = 0.0;
    hostBrainInputData.enemySensorBackLeft        = 0.0;
    hostBrainInputData.enemySensorBackRight       = 0.0;
    hostBrainInputData.wallSensor                 = 0.0;
    hostBrainInputData.energyDifferenceSensor     = 0.0;
    hostBrainInputData.biasInput                  = 0.0;

    hostBrainOutputData.leftOutput                = 0.0;
    hostBrainOutputData.rightOutput               = 0.0;
    hostBrainOutputData.forwardOutput             = 0.0;

    parasiteBrainInputData.foodSensorForward      = 0.0;
    parasiteBrainInputData.foodSensorLeft         = 0.0;
    parasiteBrainInputData.foodSensorRight        = 0.0;
    parasiteBrainInputData.foodSensorBackLeft     = 0.0;
    parasiteBrainInputData.foodSensorBackRight    = 0.0;
    parasiteBrainInputData.enemySensorForward     = 0.0;
    parasiteBrainInputData.enemySensorLeft        = 0.0;
    parasiteBrainInputData.enemySensorRight       = 0.0;
    parasiteBrainInputData.enemySensorBackLeft    = 0.0;
    parasiteBrainInputData.enemySensorBackRight   = 0.0;
    parasiteBrainInputData.wallSensor             = 0.0;
    parasiteBrainInputData.energyDifferenceSensor = 0.0;
    parasiteBrainInputData.biasInput              = 0.0;

    parasiteBrainOutputData.leftOutput            = 0.0;
    parasiteBrainOutputData.rightOutput           = 0.0;
    parasiteBrainOutputData.forwardOutput         = 0.0;


    // Set start positions
    switch (hostStartingPosition)
    {
    case hostStartLeft:
        hostPosition.agentPos.x     = LEFT_POS_START_X;
        hostPosition.agentPos.y     = LEFT_POS_START_Y;
        hostPosition.agentPos.z     = 0.0;
        hostPosition.agentHdg       = LEFT_POS_START_HDG;

        parasitePosition.agentPos.x = RIGHT_POS_START_X;
        parasitePosition.agentPos.y = RIGHT_POS_START_Y;
        parasitePosition.agentPos.z = 0.0;
        parasitePosition.agentHdg   = RIGHT_POS_START_HDG;
        break;

    case hostStartRight:
        hostPosition.agentPos.x     = RIGHT_POS_START_X;
        hostPosition.agentPos.y     = RIGHT_POS_START_Y;
        hostPosition.agentPos.z     = 0.0;
        hostPosition.agentHdg       = RIGHT_POS_START_HDG;

        parasitePosition.agentPos.x = LEFT_POS_START_X;
        parasitePosition.agentPos.y = LEFT_POS_START_Y;
        parasitePosition.agentPos.z = 0.0;
        parasitePosition.agentHdg   = LEFT_POS_START_HDG;
        break;

    default:
        cout<<"Invalid host starting pos in GameWorld constructor"<<endl;
        exit(1);
        break;
    }


    // Set the positions of the extra food pieces,
    // which are needed during generation champ
    // tournaments, and during dominance tournaments
    setExtraFoodPositionData();


    // setup the food layout data
    // The first 9 pieces of food are always in the
    // same positions
    foodData[0].foodPosition.x = 300.0;
    foodData[0].foodPosition.y = 540.0;

    foodData[1].foodPosition.x = 180.0;
    foodData[1].foodPosition.y = 420.0;

    foodData[2].foodPosition.x = 420.0;
    foodData[2].foodPosition.y = 420.0;

    foodData[3].foodPosition.x = 60.0;
    foodData[3].foodPosition.y = 300.0;

    foodData[4].foodPosition.x = 540.0;
    foodData[4].foodPosition.y = 300.0;

    foodData[5].foodPosition.x = 180.0;
    foodData[5].foodPosition.y = 180.0;

    foodData[6].foodPosition.x = 420.0;
    foodData[6].foodPosition.y = 180.0;

    foodData[7].foodPosition.x = 240.0;
    foodData[7].foodPosition.y = 60.0;

    foodData[8].foodPosition.x = 360.0;
    foodData[8].foodPosition.y = 60.0;

    // Check to see what else we
    // need to do, based on the type
    // of food layout
    switch (newFoodLayout)
    {
    case standardFoodLayout:
        numFoodPieces = 9;
        break;

    case singleExtraFoodLayout:
        if ((firstExtraFoodPosIndex < 0) ||
                (firstExtraFoodPosIndex > MAX_EXTRA_FOOD_PIECES-1))
        {
            cout<<"Error in GameWorld, firstExtraFoodPosIndex out of bounds: "<<firstExtraFoodPosIndex<<endl;
            exit(1);
        }

        numFoodPieces = 10;

        foodData[9].foodPosition = extraFoodPosData[firstExtraFoodPosIndex];
        break;

    case doubleExtraFoodLayout:
        if ((firstExtraFoodPosIndex < 0) ||
                (firstExtraFoodPosIndex > MAX_EXTRA_FOOD_PIECES-1))
        {
            cout<<"Error in GameWorld, firstExtraFoodPosIndex out of bounds: "<<firstExtraFoodPosIndex<<endl;
            exit(1);
        }

        if ((secondExtraFoodPosIndex < 0) ||
                (secondExtraFoodPosIndex > MAX_EXTRA_FOOD_PIECES-1))
        {
            cout<<"Error in GameWorld, secondExtraFoodPosIndex out of bounds: "<<secondExtraFoodPosIndex<<endl;
            exit(1);
        }

        numFoodPieces = 11;

        foodData[9].foodPosition  = extraFoodPosData[firstExtraFoodPosIndex];
        foodData[10].foodPosition = extraFoodPosData[secondExtraFoodPosIndex];
        break;

    default:
        cout<<"Invalid food layout in GameWorld constructor"<<endl;
        exit(1);
        break;
    }

    // Mark all food pieces uneaten
    for (int i = 0; i < numFoodPieces; i++)
    {
        foodData[i].foodPosition.z = 0.0;
        foodData[i].foodEaten      = false;
    }


    // North vector used for various calculations
    northVector.x = 0.0;
    northVector.y = 1.0;
    northVector.z = 0.0;

    // Set the wall line points for line
    // intersection calculations
    leftWall.x1   = BOARDBUFFER;
    leftWall.y1   = BOARDBUFFER;
    leftWall.x2   = BOARDBUFFER;
    leftWall.y2   = BOARDSIZE + BOARDBUFFER;

    bottomWall.x1 = BOARDBUFFER;
    bottomWall.y1 = BOARDBUFFER;
    bottomWall.x2 = BOARDSIZE + BOARDBUFFER;
    bottomWall.y2 = BOARDBUFFER;

    rightWall.x1  = BOARDSIZE + BOARDBUFFER;
    rightWall.y1  = BOARDBUFFER;
    rightWall.x2  = BOARDSIZE + BOARDBUFFER;
    rightWall.y2  = BOARDSIZE + BOARDBUFFER;

    topWall.x1    = BOARDBUFFER;
    topWall.y1    = BOARDSIZE + BOARDBUFFER;
    topWall.x2    = BOARDSIZE + BOARDBUFFER;
    topWall.y2    = BOARDSIZE + BOARDBUFFER;



    // Make sure the rotation matrix is
    // is ready
    rotationMatrix.Identity();
}





GameWorld::~GameWorld()
{
}




void GameWorld::resetForNewRun(hostStartPosition startPos,
                               RobotAgent *newHostAgent,
                               RobotAgent *newParasiteAgent,
                               foodLayoutType newFoodLayout,
                               int firstExtraFoodPosIndex,
                               int secondExtraFoodPosIndex)
{
    hostStartingPosition = startPos;
    hostAgent            = newHostAgent;
    parasiteAgent        = newParasiteAgent;

    hostScore            = 0;
    parasiteScore        = 0;
    forwardMoveAmount    = 0.0;
    turnAngle            = 0.0;
    newHeading           = 0.0;
    hostEnergy           = FULLENERGY;
    parasiteEnergy       = FULLENERGY;
    energyUsed           = 0.0;
    enemyBearing         = 0.0;
    absEnemyBearing      = 0.0;
    enemyDistance        = 0.0;
    foodBearing          = 0.0;
    absFoodBearing       = 0.0;
    foodDistance         = 0.0;
    robotHeading         = 0.0;
    wallDistance         = 0.0;
    numFoodEatenHost     = 0;
    numFoodEatenParasite = 0;


    // Initialize input/output data
    hostBrainInputData.foodSensorForward          = 0.0;
    hostBrainInputData.foodSensorLeft             = 0.0;
    hostBrainInputData.foodSensorRight            = 0.0;
    hostBrainInputData.foodSensorBackLeft         = 0.0;
    hostBrainInputData.foodSensorBackRight        = 0.0;
    hostBrainInputData.enemySensorForward         = 0.0;
    hostBrainInputData.enemySensorLeft            = 0.0;
    hostBrainInputData.enemySensorRight           = 0.0;
    hostBrainInputData.enemySensorBackLeft        = 0.0;
    hostBrainInputData.enemySensorBackRight       = 0.0;
    hostBrainInputData.wallSensor                 = 0.0;
    hostBrainInputData.energyDifferenceSensor     = 0.0;
    hostBrainInputData.biasInput                  = 0.0;

    hostBrainOutputData.leftOutput                = 0.0;
    hostBrainOutputData.rightOutput               = 0.0;
    hostBrainOutputData.forwardOutput             = 0.0;

    parasiteBrainInputData.foodSensorForward      = 0.0;
    parasiteBrainInputData.foodSensorLeft         = 0.0;
    parasiteBrainInputData.foodSensorRight        = 0.0;
    parasiteBrainInputData.foodSensorBackLeft     = 0.0;
    parasiteBrainInputData.foodSensorBackRight    = 0.0;
    parasiteBrainInputData.enemySensorForward     = 0.0;
    parasiteBrainInputData.enemySensorLeft        = 0.0;
    parasiteBrainInputData.enemySensorRight       = 0.0;
    parasiteBrainInputData.enemySensorBackLeft    = 0.0;
    parasiteBrainInputData.enemySensorBackRight   = 0.0;
    parasiteBrainInputData.wallSensor             = 0.0;
    parasiteBrainInputData.energyDifferenceSensor = 0.0;
    parasiteBrainInputData.biasInput              = 0.0;

    parasiteBrainOutputData.leftOutput            = 0.0;
    parasiteBrainOutputData.rightOutput           = 0.0;
    parasiteBrainOutputData.forwardOutput         = 0.0;


    // Set start positions
    switch (hostStartingPosition)
    {
    case hostStartLeft:
        hostPosition.agentPos.x     = LEFT_POS_START_X;
        hostPosition.agentPos.y     = LEFT_POS_START_Y;
        hostPosition.agentPos.z     = 0.0;
        hostPosition.agentHdg       = LEFT_POS_START_HDG;

        parasitePosition.agentPos.x = RIGHT_POS_START_X;
        parasitePosition.agentPos.y = RIGHT_POS_START_Y;
        parasitePosition.agentPos.z = 0.0;
        parasitePosition.agentHdg   = RIGHT_POS_START_HDG;
        break;

    case hostStartRight:
        hostPosition.agentPos.x     = RIGHT_POS_START_X;
        hostPosition.agentPos.y     = RIGHT_POS_START_Y;
        hostPosition.agentPos.z     = 0.0;
        hostPosition.agentHdg       = RIGHT_POS_START_HDG;

        parasitePosition.agentPos.x = LEFT_POS_START_X;
        parasitePosition.agentPos.y = LEFT_POS_START_Y;
        parasitePosition.agentPos.z = 0.0;
        parasitePosition.agentHdg   = LEFT_POS_START_HDG;
        break;

    default:
        cout<<"Invalid host starting pos in GameWorld constructor"<<endl;
        exit(1);
        break;
    }



    foodLayout = newFoodLayout;

    // setup the food layout data
    // The first 9 pieces of food are always in the
    // same positions
    foodData[0].foodPosition.x = 300.0;
    foodData[0].foodPosition.y = 540.0;

    foodData[1].foodPosition.x = 180.0;
    foodData[1].foodPosition.y = 420.0;

    foodData[2].foodPosition.x = 420.0;
    foodData[2].foodPosition.y = 420.0;

    foodData[3].foodPosition.x = 60.0;
    foodData[3].foodPosition.y = 300.0;

    foodData[4].foodPosition.x = 540.0;
    foodData[4].foodPosition.y = 300.0;

    foodData[5].foodPosition.x = 180.0;
    foodData[5].foodPosition.y = 180.0;

    foodData[6].foodPosition.x = 420.0;
    foodData[6].foodPosition.y = 180.0;

    foodData[7].foodPosition.x = 240.0;
    foodData[7].foodPosition.y = 60.0;

    foodData[8].foodPosition.x = 360.0;
    foodData[8].foodPosition.y = 60.0;


    // Check to see what else we
    // need to do, based on the type
    // of food layout
    switch (newFoodLayout)
    {
    case standardFoodLayout:
        numFoodPieces = 9;
        break;

    case singleExtraFoodLayout:
        if ((firstExtraFoodPosIndex < 0) ||
                (firstExtraFoodPosIndex > MAX_EXTRA_FOOD_PIECES-1))
        {
            cout<<"Error in GameWorld, firstExtraFoodPosIndex out of bounds: "<<firstExtraFoodPosIndex<<endl;
            exit(1);
        }

        numFoodPieces = 10;

        foodData[9].foodPosition = extraFoodPosData[firstExtraFoodPosIndex];
        break;

    case doubleExtraFoodLayout:
        if ((firstExtraFoodPosIndex < 0) ||
                (firstExtraFoodPosIndex > MAX_EXTRA_FOOD_PIECES-1))
        {
            cout<<"Error in GameWorld, firstExtraFoodPosIndex out of bounds: "<<firstExtraFoodPosIndex<<endl;
            exit(1);
        }

        if ((secondExtraFoodPosIndex < 0) ||
                (secondExtraFoodPosIndex > MAX_EXTRA_FOOD_PIECES-1))
        {
            cout<<"Error in GameWorld, secondExtraFoodPosIndex out of bounds: "<<secondExtraFoodPosIndex<<endl;
            exit(1);
        }

        numFoodPieces = 11;

        foodData[9].foodPosition  = extraFoodPosData[firstExtraFoodPosIndex];
        foodData[10].foodPosition = extraFoodPosData[secondExtraFoodPosIndex];
        break;

    default:
        cout<<"Invalid food layout in GameWorld constructor"<<endl;
        exit(1);
        break;
    }


    // We need to set all food pieces
    // to uneaten
    for (int i = 0; i < numFoodPieces; i++)
    {
        foodData[i].foodPosition.z = 0.0;
        foodData[i].foodEaten      = false;
    }



    // Make sure the rotation matrix is
    // is ready
    rotationMatrix.Identity();
}





void GameWorld::calculateAgentMovement(agentType whichAgent)
{
    switch (whichAgent)
    {
    case HOST:
        forwardMoveAmount = 2.66 * hostBrainOutputData.forwardOutput;
        turnAngle         = 0.24 * (hostBrainOutputData.rightOutput -
                                    hostBrainOutputData.leftOutput);
        newHeading        = hostPosition.agentHdg + (turnAngle * RAD2DEG);
        break;

    case PARASITE:
        forwardMoveAmount = 2.66 * parasiteBrainOutputData.forwardOutput;
        turnAngle         = 0.24 * (parasiteBrainOutputData.rightOutput -
                                    parasiteBrainOutputData.leftOutput);
        newHeading        = parasitePosition.agentHdg + (turnAngle * RAD2DEG);
        break;

    default:
        cout<<"Invalid agentType in GameWorld calc agent movement"<<endl;
        exit(1);
        break;
    }


    rotationMatrix.Identity();
    rotationMatrix.Rotate(newHeading, 0, 0, 1);
    robotHeadingVector = rotationMatrix * northVector;
    robotHeadingVector = robotHeadingVector * forwardMoveAmount;
    energyUsed         = fabs(1.5 * turnAngle) + forwardMoveAmount;



    switch (whichAgent)
    {
    case HOST:
        hostPosition.agentPos += robotHeadingVector;
        hostPosition.agentHdg  = newHeading;
        hostEnergy            -= energyUsed;
        break;

    case PARASITE:
        parasitePosition.agentPos += robotHeadingVector;
        parasitePosition.agentHdg  = newHeading;
        parasiteEnergy            -= energyUsed;
        break;

    default:
        cout<<"Invalid agentType in GameWorld calc agent movement"<<endl;
        exit(1);
        break;
    }


    // Make sure we didn't run out of energy,
    // which shouldn't occur. Agents should
    // have enough power to make it through the
    // full length game
    if (hostEnergy <= 0.0)
    {
        cout<<"Host ran out of energy, this should be impossible!"<<endl;
        exit(1);
    }

    if (parasiteEnergy <= 0.0)
    {
        cout<<"Parasite ran out of energy, this should be impossible!"<<endl;
        exit(1);
    }
}








void GameWorld::setEnemyBearingInputs(brainInputData &brainData,
                                      double bearing, double distance)
{
    // The directional sensors are setup as follows, for exclusive
    // sensing (there's some overlap):
    // The front three sensor cover the front 180 degree
    // FOV, with the front facing sensor taking 30 degrees,
    // and the two front-side ones taking taking 75 degrees each
    // The back two quarter ones take 90 degrees left and right
    // Each sensor overlaps the other one by an extra 5 degrees,
    // so there is a 10 degree shared overlap.

    // Figure out sensor activation on one half to simplify things,
    // check sign of bearing later to figure which input to
    // stimulate
    // Remember that since we're looking at absolute value
    // of bearing across one side, and forward sensor spans
    // both sides, we have to half the sensor angular range

    // Zero out old values
    brainData.enemySensorForward   = 0.0;
    brainData.enemySensorLeft      = 0.0;
    brainData.enemySensorRight     = 0.0;
    brainData.enemySensorBackLeft  = 0.0;
    brainData.enemySensorBackRight = 0.0;

    absEnemyBearing = fabs(bearing);

    if (absEnemyBearing < FIRST_SENSOR_BOUNDARY)
    {
        // forward sensor only
        brainData.enemySensorForward = distance;
    }
    else if  ((absEnemyBearing >= FIRST_SENSOR_BOUNDARY) &&
              (absEnemyBearing <= SECOND_SENSOR_BOUNDARY))
    {
        // both forward and front-side sensors overlap area
        brainData.enemySensorForward = distance;
        if (enemyBearing < 0.0)
        {
            brainData.enemySensorLeft  = distance;
        }
        else
        {
            brainData.enemySensorRight = distance;
        }
    }
    else if (absEnemyBearing < THIRD_SENSOR_BOUNDARY)
    {
        // Forward-side sensor only
        if (enemyBearing < 0.0)
        {
            brainData.enemySensorLeft  = distance;
        }
        else
        {
            brainData.enemySensorRight = distance;
        }
    }
    else if ((absEnemyBearing >= THIRD_SENSOR_BOUNDARY) &&
             (absEnemyBearing <= FOURTH_SENSOR_BOUNDARY))
    {
        // Forward-side and back sensor overlap area
        if (enemyBearing < 0.0)
        {
            brainData.enemySensorLeft      = distance;
            brainData.enemySensorBackLeft  = distance;
        }
        else
        {
            brainData.enemySensorRight     = distance;
            brainData.enemySensorBackRight = distance;
        }
    }
    else if (absEnemyBearing < FIFTH_SENSOR_BOUNDARY)
    {
        // back side sensor only
        if (enemyBearing < 0.0)
        {
            brainData.enemySensorBackLeft  = distance;
        }
        else
        {
            brainData.enemySensorBackRight = distance;
        }
    }
    else if (absEnemyBearing >= FIFTH_SENSOR_BOUNDARY)
    {
        // both back sensor overlap area
        brainData.enemySensorBackLeft  = distance;
        brainData.enemySensorBackRight = distance;
    }
}





void GameWorld::checkFoodDistanceForSensor(int    sensorIndex,
                                           double testDistance,
                                           double bearing)
{
    if (!foodSensorInfo[sensorIndex].foodSet)
    {
        // First food found in that section, so it is
        // by definition the closest food in the sensor FOV
        foodSensorInfo[sensorIndex].distance = testDistance;
        foodSensorInfo[sensorIndex].bearing  = bearing;
        foodSensorInfo[sensorIndex].foodSet  = true;
    }
    else if (testDistance < foodSensorInfo[sensorIndex].distance)
    {
        // It's closer to the robot than the currently saved
        // food in this sensor FOV, so replace the data with this one
        foodSensorInfo[sensorIndex].distance = testDistance;
        foodSensorInfo[sensorIndex].bearing  = bearing;
    }
}





void GameWorld::setFoodBearingInputs(agentType whichAgent)
{
    brainInputData    *brainData;
    agentPositionData agentPosition;

    switch (whichAgent)
    {
    case HOST:
        brainData     = &hostBrainInputData;
        agentPosition =  hostPosition;
        break;

    case PARASITE:
        brainData     = &parasiteBrainInputData;
        agentPosition =  parasitePosition;
        break;

    default:
        cout<<"Invalid agentType in GameWorld setFoodBearing"<<endl;
        exit(1);
        break;
    }



    // Zero the old values
    brainData->foodSensorForward   = 0.0;
    brainData->foodSensorLeft      = 0.0;
    brainData->foodSensorRight     = 0.0;
    brainData->foodSensorBackLeft  = 0.0;
    brainData->foodSensorBackRight = 0.0;

    for (int i = 0; i < NUM_SENSORS; i++)
    {
        foodSensorInfo[i].distance = 0.0;
        foodSensorInfo[i].bearing  = 0.0;
        foodSensorInfo[i].foodSet  = false;
    }




    // Go through all of the food pieces,
    // and figure out what is the closest
    // food per food sensors
    for (int i = 0; i < numFoodPieces; i++)
    {
        if (foodData[i].foodEaten)
        {
            continue;
        }
        else
        {
            // If the food hasn't been eaten,
            // do the calculations
            toFoodVector = foodData[i].foodPosition - agentPosition.agentPos;
            foodDistance = toFoodVector.Magnitude();

            rotationMatrix.Identity();
            rotationMatrix.Rotate(agentPosition.agentHdg, 0, 0, 1);
            robotHeadingVector = rotationMatrix * northVector;
            robotHeadingVector.Normalize();

            foodBearing = (atan2f(robotHeadingVector.y, robotHeadingVector.x) -
                           atan2f(toFoodVector.y, toFoodVector.x)) * RAD2DEG;

            if (foodBearing < -180.0)
            {
                foodBearing += 360.0;
            }
            else if (foodBearing > 180.0)
            {
                foodBearing -= 360.0;
            }


            absFoodBearing = fabs(foodBearing);


            if ((foodBearing >= (-1.0 * (FORWARD_SENSOR_SPREAD/2.0))) &&
                    (foodBearing <= (FORWARD_SENSOR_SPREAD/2.0)))
            {
                // It's in the front sensor section
                checkFoodDistanceForSensor(FORWARD_SENSOR, foodDistance, foodBearing);
            }
            else if (absFoodBearing <= (FORWARD_SIDE_SENSOR_SPREAD + (FORWARD_SENSOR_SPREAD/2.0)))
            {
                // It's in one of the front-side boundaries

                if (foodBearing < 0.0)
                {
                    // Left side
                    checkFoodDistanceForSensor(FRONT_LEFT_SENSOR, foodDistance, foodBearing);
                }
                else
                {
                    // Right side
                    checkFoodDistanceForSensor(FRONT_RIGHT_SENSOR, foodDistance, foodBearing);
                }
            }
            else if (absFoodBearing <= 180.0)
            {
                // It's in the back sensor boundaries

                if (foodBearing < 0.0)
                {
                    // Left side
                    checkFoodDistanceForSensor(BACK_LEFT_SENSOR, foodDistance, foodBearing);
                }
                else
                {
                    // Right side
                    checkFoodDistanceForSensor(BACK_RIGHT_SENSOR, foodDistance, foodBearing);
                }
            }
        }
    }


    // Now that we have an array of all of the food distance values needed to
    // setup the sensor inputs
    // Note: Since the sensors handle multiple food items,
    // overlap of sensors is not used
    // If the foodSet value is false, there is
    // no food in that sensor FOV, so leave the
    // brain input value at 0.0
    if (foodSensorInfo[FORWARD_SENSOR].foodSet)
    {
        brainData->foodSensorForward = fabs((foodSensorInfo[FORWARD_SENSOR].distance/MAX_DISTANCE) - 1.0);
    }

    if (foodSensorInfo[FRONT_LEFT_SENSOR].foodSet)
    {
        brainData->foodSensorLeft = fabs((foodSensorInfo[FRONT_LEFT_SENSOR].distance/MAX_DISTANCE) - 1.0);
    }

    if (foodSensorInfo[FRONT_RIGHT_SENSOR].foodSet)
    {
        brainData->foodSensorRight = fabs((foodSensorInfo[FRONT_RIGHT_SENSOR].distance/MAX_DISTANCE) - 1.0);
    }

    if (foodSensorInfo[BACK_LEFT_SENSOR].foodSet)
    {
        brainData->foodSensorBackLeft = fabs((foodSensorInfo[BACK_LEFT_SENSOR].distance/MAX_DISTANCE) - 1.0);
    }

    if (foodSensorInfo[BACK_RIGHT_SENSOR].foodSet)
    {
        brainData->foodSensorBackRight = fabs((foodSensorInfo[BACK_RIGHT_SENSOR].distance/MAX_DISTANCE) - 1.0);
    }
}







void GameWorld::getWallIntersectPoint(Line2D aimLine)
{
    if (intersect2dLines(aimLine, leftWall, intersectPoint))
    {
        return;
    }
    else if (intersect2dLines(aimLine, bottomWall, intersectPoint))
    {
        return;
    }
    else if (intersect2dLines(aimLine, rightWall, intersectPoint))
    {
        return;
    }
    else if (intersect2dLines(aimLine, topWall, intersectPoint))
    {
        return;
    }
    else
    {
        cout<<"Error in GameWorld::getWallIntersectPoint, no intersection!"<<endl;
        exit(1);
    }
}






void GameWorld::setAgentWallDistance(agentType whichAgent)
{
    switch (whichAgent)
    {
    case HOST:
        robotHeading        = hostPosition.agentHdg;
        robotPositionVector = hostPosition.agentPos;
        break;

    case PARASITE:
        robotHeading        = parasitePosition.agentHdg;
        robotPositionVector = parasitePosition.agentPos;
        break;

    default:
        cout<<"Invalid agent type in GameWorld setAgentWallDistance"<<endl;
        exit(1);
        break;
    }

    // Robot positions don't take into account the buffer,
    // but the wall do, so correct here. They're in
    // game board coordinates, not opengl window
    // coordinates like the walls
    robotPositionVector.x += BOARDBUFFER;
    robotPositionVector.y += BOARDBUFFER;
    robotPositionVector.z  = 0.0;

    rotationMatrix.Identity();
    rotationMatrix.Rotate(robotHeading, 0, 0, 1);
    robotHeadingVector = rotationMatrix * northVector;


    // This vector will be long enough to pass
    // through the walls, so we can check for intersection
    robotHeadingVector = robotHeadingVector * MAX_DISTANCE;
    robotToWallVector  = robotPositionVector + robotHeadingVector;

    robotAimLine.x1 = robotPositionVector.x;
    robotAimLine.y1 = robotPositionVector.y;
    robotAimLine.x2 = robotToWallVector.x;
    robotAimLine.y2 = robotToWallVector.y;

    // Set the point2d intersect point
    getWallIntersectPoint(robotAimLine);

    intersectPointVector.x = intersectPoint.x;
    intersectPointVector.y = intersectPoint.y;
    intersectPointVector.z = 0.0;

    robotToWallVector = intersectPointVector - robotPositionVector;
    wallDistance      = robotToWallVector.Magnitude();
    wallDistance      = fabs((wallDistance/MAX_DISTANCE) - 1.0);

    switch (whichAgent)
    {
    case HOST:
        hostBrainInputData.wallSensor = wallDistance;
        break;

    case PARASITE:
        parasiteBrainInputData.wallSensor = wallDistance;
        break;

    default:
        cout<<"Invalid agent type in GameWorld setAgentWallDistance"<<endl;
        exit(1);
        break;
    }
}






// Input max range is -1.0 -> 1.0
void GameWorld::calculateSensorValues()
{
    // No firm explanation about how the constant bias
    // input is handled. Paper references that NEAT itself
    // uses/changes this value, but no description as to
    // what changes are made to mutations to allow this
    // modification to take place. On the website describing
    // the inputs, it states it is always set to 1.0, though
    // the paper doesn't mention it.
    hostBrainInputData.biasInput     = CONSTANT_BIAS_INPUT;
    parasiteBrainInputData.biasInput = CONSTANT_BIAS_INPUT;



    // Energy difference for a robot is calc'd by:
    // (myEnergy - theirEnergy)/ FULLENERGY
    // if sensor is 1.0, you have more energy
    // than you're opponent, 0.0 is equal,
    // -1.0 is less

    // Old code
    //    hostBrainInputData.energyDifferenceSensor     =
    //            (hostEnergy - parasiteEnergy)/FULLENERGY;
    //    parasiteBrainInputData.energyDifferenceSensor =
    //            -1.0 * hostBrainInputData.energyDifferenceSensor;

    // New code
    // New energy difference sensor code, instead of linear, use a
    // logarithmic curve that is more sensitive to smaller differences
    // between the energies of the two robots
    energyDiffCalcValue   = fabs((hostEnergy - parasiteEnergy)/FULLENERGY);
    energyDiffSensorValue = 0.970909 + 0.216424 * log(energyDiffCalcValue + 0.010619);

    if (energyDiffSensorValue < 0.0)
    {
        energyDiffSensorValue = 0.0;
    }
    else if (energyDiffSensorValue > 1.0)
    {
        energyDiffSensorValue = 1.0;
    }

    if (hostEnergy >= parasiteEnergy)
    {
        hostBrainInputData.energyDifferenceSensor     = energyDiffSensorValue;
        parasiteBrainInputData.energyDifferenceSensor = -1.0 * energyDiffSensorValue;
    }
    else
    {
        hostBrainInputData.energyDifferenceSensor     = -1.0 * energyDiffSensorValue;
        parasiteBrainInputData.energyDifferenceSensor = energyDiffSensorValue;
    }


    // Distance Sensors and bearing calculations
    // Distance is scaled so that the closer the enemy is,
    // the higher the input value. 1.0 would be enemy right on
    // top of you, 0.0 would be really far away
    toParasiteVector = parasitePosition.agentPos - hostPosition.agentPos;
    toHostVector     = hostPosition.agentPos     - parasitePosition.agentPos;

    enemyDistance    = toParasiteVector.Magnitude();
    enemyDistance    = fabs((enemyDistance/MAX_DISTANCE) - 1.0);

    toParasiteVector.Normalize();
    toHostVector.Normalize();

    // Calculate for the host
    rotationMatrix.Identity();
    rotationMatrix.Rotate(hostPosition.agentHdg, 0, 0, 1);
    robotHeadingVector = rotationMatrix * northVector;
    robotHeadingVector.Normalize();

    // The bearing is a signed value, from -180 -> 180
    enemyBearing = (atan2f(robotHeadingVector.y, robotHeadingVector.x) -
                    atan2f(toParasiteVector.y, toParasiteVector.x)) * RAD2DEG;

    if (enemyBearing < -180.0)
    {
        enemyBearing += 360.0;
    }
    else if (enemyBearing > 180.0)
    {
        enemyBearing -= 360.0;
    }

    // Set the enemy input sensors for the host
    setEnemyBearingInputs(hostBrainInputData, enemyBearing, enemyDistance);


    // Calculate for the parasite
    rotationMatrix.Identity();
    rotationMatrix.Rotate(parasitePosition.agentHdg, 0, 0, 1);
    robotHeadingVector = rotationMatrix * northVector;
    robotHeadingVector.Normalize();

    // The bearing is a signed value, from -180 -> 180
    enemyBearing = (atan2f(robotHeadingVector.y, robotHeadingVector.x) -
                    atan2f(toHostVector.y, toHostVector.x)) * RAD2DEG;

    if (enemyBearing < -180.0)
    {
        enemyBearing += 360.0;
    }
    else if (enemyBearing > 180.0)
    {
        enemyBearing -= 360.0;
    }

    // Set the enemy input sensors for the parasite
    setEnemyBearingInputs(parasiteBrainInputData, enemyBearing, enemyDistance);




    // Setup the food sensors for each robot
    setFoodBearingInputs(HOST);
    setFoodBearingInputs(PARASITE);


    // Setup the wall distance sensor
    // How far in front of the robot is the wall?
    setAgentWallDistance(HOST);
    setAgentWallDistance(PARASITE);
}





bool GameWorld::checkForRobotCollision()
{
    toParasiteVector = parasitePosition.agentPos - hostPosition.agentPos;

    if (toParasiteVector.Magnitude() <= ROBOT_COLLISION_DISTANCE)
    {
        return true;
    }
    else
    {
        return false;
    }
}







void GameWorld::checkForFoodCollision()
{
    // The host get's the tie breaker on snaggin food,
    // if both agents get there at the same time,
    // the host gets it. That's what you get for
    // being a parasite.

    // Check host for food munching action
    for (int i = 0; i < numFoodPieces; i++)
    {
        if (foodData[i].foodEaten)
        {
            // This food has already been eaten,
            // skip it
            continue;
        }
        else
        {
            toFoodVector = foodData[i].foodPosition - hostPosition.agentPos;
            if (toFoodVector.Magnitude() <= FOOD_COLLISION_DISTANCE)
            {
                foodData[i].foodEaten  = true;
                numFoodEatenHost++;
                hostEnergy            += FOODENERGYBOOST;

                if (hostEnergy > FULLENERGY)
                {
                    hostEnergy = FULLENERGY;
                }
            }
        }
    }


    // Now see if the parasite partook in food munchuns
    for (int i = 0; i < numFoodPieces; i++)
    {
        if (foodData[i].foodEaten)
        {
            // This food has already been eaten,
            // skip it
            continue;
        }
        else
        {
            toFoodVector = foodData[i].foodPosition - parasitePosition.agentPos;
            if (toFoodVector.Magnitude() <= FOOD_COLLISION_DISTANCE)
            {
                foodData[i].foodEaten  = true;
                numFoodEatenParasite++;
                parasiteEnergy        += FOODENERGYBOOST;

                if (parasiteEnergy > FULLENERGY)
                {
                    parasiteEnergy = FULLENERGY;
                }
            }
        }
    }
}








void GameWorld::checkForWallCollision()
{
    // Check X values...
    // host
    if (hostPosition.agentPos.x < MIN_POS_X)
    {
        hostPosition.agentPos.x = MIN_POS_X;
    }
    else if (hostPosition.agentPos.x > MAX_POS_X)
    {
        hostPosition.agentPos.x = MAX_POS_X;
    }

    // parasite
    if (parasitePosition.agentPos.x < MIN_POS_X)
    {
        parasitePosition.agentPos.x = MIN_POS_X;
    }
    else if (parasitePosition.agentPos.x > MAX_POS_X)
    {
        parasitePosition.agentPos.x = MAX_POS_X;
    }


    // Check Y values...
    // host
    if (hostPosition.agentPos.y < MIN_POS_Y)
    {
        hostPosition.agentPos.y = MIN_POS_Y;
    }
    else if (hostPosition.agentPos.y > MAX_POS_Y)
    {
        hostPosition.agentPos.y = MAX_POS_Y;
    }

    // parasite
    if (parasitePosition.agentPos.y < MIN_POS_Y)
    {
        parasitePosition.agentPos.y = MIN_POS_Y;
    }
    else if (parasitePosition.agentPos.y > MAX_POS_Y)
    {
        parasitePosition.agentPos.y = MAX_POS_Y;
    }
}





void GameWorld::setExtraFoodPositionData()
{
    extraFoodPosData[0].x  =  60.0;
    extraFoodPosData[0].y  = 480.0;
    extraFoodPosData[0].z  =   0.0;

    extraFoodPosData[1].x  = 180.0;
    extraFoodPosData[1].y  = 480.0;
    extraFoodPosData[1].z  =   0.0;

    extraFoodPosData[2].x  = 420.0;
    extraFoodPosData[2].y  = 480.0;
    extraFoodPosData[2].z  =   0.0;

    extraFoodPosData[3].x  = 540.0;
    extraFoodPosData[3].y  = 480.0;
    extraFoodPosData[3].z  =   0.0;

    extraFoodPosData[4].x  = 180.0;
    extraFoodPosData[4].y  = 360.0;
    extraFoodPosData[4].z  =   0.0;

    extraFoodPosData[5].x  = 420.0;
    extraFoodPosData[5].y  = 360.0;
    extraFoodPosData[5].z  =   0.0;

    extraFoodPosData[6].x  = 180.0;
    extraFoodPosData[6].y  = 240.0;
    extraFoodPosData[6].z  =   0.0;

    extraFoodPosData[7].x  = 420.0;
    extraFoodPosData[7].y  = 240.0;
    extraFoodPosData[7].z  =   0.0;

    extraFoodPosData[8].x  =  60.0;
    extraFoodPosData[8].y  = 120.0;
    extraFoodPosData[8].z  =   0.0;

    extraFoodPosData[9].x  = 180.0;
    extraFoodPosData[9].y  = 120.0;
    extraFoodPosData[9].z  =   0.0;

    extraFoodPosData[10].x = 420.0;
    extraFoodPosData[10].y = 120.0;
    extraFoodPosData[10].z =   0.0;

    extraFoodPosData[11].x = 540.0;
    extraFoodPosData[11].y = 120.0;
    extraFoodPosData[11].z =   0.0;
}




// Return true if competition over, false otherwise
bool GameWorld::update()
{
    // Do sensor calcs, prepare brain inputs...
    calculateSensorValues();


    // Send inputs and update brain...
    hostAgent->setBrainInputs(hostBrainInputData);
    hostAgent->update();

    parasiteAgent->setBrainInputs(parasiteBrainInputData);
    parasiteAgent->update();


    // Get outputs....
    hostAgent->getBrainOutputs(hostBrainOutputData);
    parasiteAgent->getBrainOutputs(parasiteBrainOutputData);


    // Figure out how much they moved, and how much energy was used...
    calculateAgentMovement(HOST);
    calculateAgentMovement(PARASITE);

    // Check for collisions....
    checkForFoodCollision();
    checkForWallCollision();

    if (checkForRobotCollision())
    {
        // Robots collided, someone won the contest...
        if (hostEnergy > parasiteEnergy)
        {
            hostScore     = 1;
            parasiteScore = 0;
        }
        else
        {
            hostScore     = 0;
            parasiteScore = 1;
        }
        return true;
    }

    return false;
}





unsigned int GameWorld::getNumFoodPieces()
{
    return numFoodPieces;
}





singlePieceFoodData GameWorld::getFoodData(unsigned int foodIndex)
{
    if (foodIndex >= numFoodPieces)
    {
        cout<<"Error, invalid foodIndex in getFoodData: "<<foodIndex<<endl;
        exit(1);
    }
    else
    {
        return foodData[foodIndex];
    }
}




unsigned int GameWorld::getNumFoodEaten(agentType whichAgent)
{
    switch (whichAgent)
    {
    case HOST:
        return numFoodEatenHost;
        break;

    case PARASITE:
        return numFoodEatenParasite;
        break;

    default:
        cout<<"Invalid agentType in GameWorld::getNumFoodEaten"<<endl;
        exit(1);
        break;
    }
}





brainInputData GameWorld::getRobotSensorValues(agentType whichAgent)
{
    switch (whichAgent)
    {
    case HOST:
        return hostBrainInputData;
        break;

    case PARASITE:
        return parasiteBrainInputData;
        break;

    default:
        cout<<"Invalid agentType in GameWorld::getRobotSensorValues"<<endl;
        exit(1);
        break;
    }
}






brainOutputData GameWorld::getRobotOutputValues(agentType whichAgent)
{
    switch (whichAgent)
    {
    case HOST:
        return hostBrainOutputData;
        break;

    case PARASITE:
        return parasiteBrainOutputData;
        break;

    default:
        cout<<"Invalid agentType in GameWorld::getRobotOutputValues"<<endl;
        exit(1);
        break;
    }
}







double GameWorld::getRobotEnergy(agentType whichAgent)
{
    switch (whichAgent)
    {
    case HOST:
        return hostEnergy;
        break;

    case PARASITE:
        return parasiteEnergy;
        break;

    default:
        cout<<"Invalid agentType in GameWorld::getRobotEnergy"<<endl;
        exit(1);
        break;
    }
}






unsigned int GameWorld::getHostFinalScore()
{
    return hostScore;
}


unsigned int GameWorld::getParasiteFinalScore()
{
    return parasiteScore;
}


agentPositionData GameWorld::getHostPosition()
{
    return hostPosition;
}


agentPositionData GameWorld::getParasitePosition()
{
    return parasitePosition;
}

