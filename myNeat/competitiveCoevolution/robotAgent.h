#ifndef ROBOTAGENT_H
#define ROBOTAGENT_H

#include <vector>
#include <math.h>

#include "phenotype.h"
#include "utils.h"
#include "SVector2D.h"
#include "parameters.h"
#include "timer.h"
#include "mathVector.h"
#include "mathMatrix.h"

using namespace std;


struct brainInputData
{
    double foodSensorForward;
    double foodSensorLeft;
    double foodSensorRight;
    double foodSensorBackLeft;
    double foodSensorBackRight;
    double enemySensorForward;
    double enemySensorLeft;
    double enemySensorRight;
    double enemySensorBackLeft;
    double enemySensorBackRight;
    double wallSensor;
    double energyDifferenceSensor;
    double biasInput;
};



struct brainOutputData
{
    double leftOutput;
    double rightOutput;
    double forwardOutput;
};




// Small memory footprint
// versions of brain I/O,
// useful for NCD behavior
// characterization
struct simplifiedBrainInputData
{
    char foodSensorForward;
    char foodSensorLeft;
    char foodSensorRight;
    char foodSensorBackLeft;
    char foodSensorBackRight;
    char enemySensorForward;
    char enemySensorLeft;
    char enemySensorRight;
    char enemySensorBackLeft;
    char enemySensorBackRight;
    char wallSensor;
    char energyDifferenceSensor;
};



struct simplifiedBrainOutputData
{
    char leftOutput;
    char rightOutput;
    char forwardOutput;
};



class RobotAgent
{
private:
    CNeuralNet* agentBrain;

    // Neural Net inputs
    brainInputData agentInputs;

    // Neural Net outputq
    brainOutputData agentOutputs;


public:
    RobotAgent();

    // Update the Neural net with info from environment
    bool update();

    // Load up a new brain
    void insertNewBrain(CNeuralNet* newBrain);

    // Set brain inputs from match evaluator
    void setBrainInputs(brainInputData newInputs);

    // Get brain outputs after agent update
    void getBrainOutputs(brainOutputData &newOutputs);

    // Get the number of neurons in the brain
    int getNumberOfBrainNeurons();

    // Get the number of connectsions in the brain
    int getNumberOfBrainConnections();

    // Get the number of hidden nodes in the brain
    int getNumberOfBrainHiddenNodes();

    // Get the brain genome ID number
    int getBrainGenomeID();

    // Get the brain clone ID number
    int getBrainCloneID();
};





#endif
