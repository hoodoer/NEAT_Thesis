#include "robotAgent.h"



RobotAgent::RobotAgent()
{
    agentBrain = NULL;
}



/***********************************************************
                 Public functions
************************************************************/

bool RobotAgent::update()
{
    vector<double> inputs;

    inputs.push_back(agentInputs.foodSensorForward);
    inputs.push_back(agentInputs.foodSensorLeft);
    inputs.push_back(agentInputs.foodSensorRight);
    inputs.push_back(agentInputs.foodSensorBackLeft);
    inputs.push_back(agentInputs.foodSensorBackRight);
    inputs.push_back(agentInputs.enemySensorForward);
    inputs.push_back(agentInputs.enemySensorLeft);
    inputs.push_back(agentInputs.enemySensorRight);
    inputs.push_back(agentInputs.enemySensorBackLeft);
    inputs.push_back(agentInputs.enemySensorBackRight);
    inputs.push_back(agentInputs.wallSensor);
    inputs.push_back(agentInputs.energyDifferenceSensor);
    inputs.push_back(agentInputs.biasInput);

    vector<double>outputs = agentBrain->Update(inputs, CNeuralNet::active);

    if (outputs.size() != param_numOutputs)
    {
        cout<<"Error, output vector size from brain is wrong!"<<endl;
        return false;
    }

    agentOutputs.leftOutput    = outputs[0];
    agentOutputs.rightOutput   = outputs[1];
    agentOutputs.forwardOutput = outputs[2];
}




void RobotAgent::insertNewBrain(CNeuralNet* newBrain)
{
    agentBrain = newBrain;
}





void RobotAgent::setBrainInputs(brainInputData newInputs)
{
    agentInputs.foodSensorForward      = newInputs.foodSensorForward;
    agentInputs.foodSensorLeft         = newInputs.foodSensorLeft;
    agentInputs.foodSensorRight        = newInputs.foodSensorRight;
    agentInputs.foodSensorBackLeft     = newInputs.foodSensorBackLeft;
    agentInputs.foodSensorBackRight    = newInputs.foodSensorBackRight;
    agentInputs.enemySensorForward     = newInputs.enemySensorForward;
    agentInputs.enemySensorLeft        = newInputs.enemySensorLeft;
    agentInputs.enemySensorRight       = newInputs.enemySensorRight;
    agentInputs.enemySensorBackLeft    = newInputs.enemySensorBackLeft;
    agentInputs.enemySensorBackRight   = newInputs.enemySensorBackRight;
    agentInputs.wallSensor             = newInputs.wallSensor;
    agentInputs.energyDifferenceSensor = newInputs.energyDifferenceSensor;
    agentInputs.biasInput              = newInputs.biasInput;
}



void RobotAgent::getBrainOutputs(brainOutputData &newOutputs)
{
    newOutputs.leftOutput    = agentOutputs.leftOutput;
    newOutputs.rightOutput   = agentOutputs.rightOutput;
    newOutputs.forwardOutput = agentOutputs.forwardOutput;
}


int RobotAgent::getNumberOfBrainNeurons()
{
    if (agentBrain == NULL)
    {
        cout<<"Error in RobotAgent::getNumberOfBrainNeurons, agentBrain pointer is NULL!"<<endl;
        return -1;
    }
    else
    {
        return agentBrain->getNumberOfNeurons();
    }
}


int RobotAgent::getNumberOfBrainConnections()
{
    if (agentBrain == NULL)
    {
        cout<<"Error in RobotAgent::getNumberOfBrainConnections, agentBrain pointer is NULL!"<<endl;
        return -1;
    }
    else
    {
        return agentBrain->getNumberOfConnections();
    }
}



int RobotAgent::getNumberOfBrainHiddenNodes()
{
    if (agentBrain == NULL)
    {
        cout<<"Error in RobotAgent::getNumberOfBrainHiddenNodes, agentBrain pointer is NULL!"<<endl;
        return -1;
    }
    else
    {
        return agentBrain->getNumberOfHiddenNodes();
    }
}


int RobotAgent::getBrainGenomeID()
{
    if (agentBrain == NULL)
    {
        cout<<"Error in RobotAgent::getBrainGenomeID, agentBrain pointer is NULL!"<<endl;
        return -5;
    }
    else
    {
        return agentBrain->getGenomeID();
    }
}



int RobotAgent::getBrainCloneID()
{
    if (agentBrain == NULL)
    {
        cout<<"Error in RobotAgent::getBrainCloneID, agentBrain pointer is NULL!"<<endl;
        return -5;
    }
    else
    {
        return agentBrain->getCloneID();
    }
}



