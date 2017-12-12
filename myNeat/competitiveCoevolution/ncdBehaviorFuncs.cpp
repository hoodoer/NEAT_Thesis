#ifdef LZMA
#include <cl-util.h>
#include <cl-internal.h>
#include <complearn/cl-command.h>
#endif

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include "ncdBehaviorFuncs.h"

#include "zlibNcd.h"


// This code handles behavior data storage, processing,
// and bridges between the competitive coevolution
// experiment written by Drew Kirkpatrick,
// and NCD code by Rudi Cilibrasi
// For NCD information, please see the website:
// http://www.complearn.org/
//
// It also supports using a fast zlib
// based NCD calculation. See
// zlibNcd.h for the class that
// does this much faster calculation
//
// drew.kirkpatrick@gmail.com



using namespace std;





/**********************************************************************
              Data types and constants
*********************************************************************/
const int numMatchesPerAgent = 2 * (param_numBestAgentsPerTeam + param_numHallOfFamers);


// How many behavioral records we're expecting.
// In it's current form, it's too big for the
// zlib NCD sliding window, so we'll split it
// into 2 buffers (first half, and second half).
// NCD value will be an average of the NCD values
// of both buffers.
const int expectedNumStaticStims = 1250;
const int staticStimBufferSplit  = 625;


// Small memory footpring version
// of brainBehaviorData. Instead
// of doubles, these are char's
// for significantly less resolution.
// The bias input has also been
// discarded, since it's a constant
// in my experiment.
struct simpleBrainBehaviorData
{
    simplifiedBrainInputData  inputData;
    simplifiedBrainOutputData outputData;
};





// The buffer data needed to
// pass to the NCD algorithms.
struct behaviorBuffer
{
    char*  buffer;
    size_t bufferSize;
    char*  bufferLabel;
};



// An individual match for a robot
struct agentMatchRecord
{
    // For normal behavior recording during
    // dynamic matches where we're using both
    // brain inputs and outputs
    vector <agentBehaviorData> reportedData;

    // For  behavior recording during
    // dynamic matches where we're using only
    // brain outputs
    vector <agentOutputBehaviorData> reportedOutputData;

    behaviorBuffer             ncdBufferData;
};



// Agent behavior data, all 24 matches
// that they undergo during a standard
// evaluation
struct agentAllMatchesRecord
{
    unsigned int robotIndex;
    hostTeamType robotTeam;

    // There will be 24 matches
    agentMatchRecord matchArray[numMatchesPerAgent];
};





// Team level behavior data
struct teamAgentBehaviorData
{
    hostTeamType          team;
    agentAllMatchesRecord agentData[param_numAgents];

    teamAgentBehaviorData(hostTeamType assignedTeam):team(assignedTeam)
    {}
};





// Agent behavior data struct, for
// use in static stimulation Novelty
// mode
struct agentStaticStimRecord
{
    unsigned int robotIndex;
    hostTeamType robotTeam;
    int          genomeID;

    // There are two buffers because
    // in this experiment the amount of
    // static stimulus values exceeds the
    // max size for ZLIB based NCD, so
    // it has to be split into to buffers
    char*        firstBuffer;
    char*        secondBuffer;

    size_t       firstBufferSize;
    size_t       secondBufferSize;

    // Vector in which to hold simplified brain
    // output data as it's reported.
    vector <brainOutputData> brainOutputVec;
};



// Team level behavior for static stimulation
// behavior data. This is an alternate method
// of characterizing behavior, which uses
// a set collection of brain input
// stimulus values, instead of sampling
// from real matches. Should give NCD a
// much better shot at measuring actual
// behavioral novelty, since the data
// being serialized and compared is
// ONLY brain outputs, with no input
// data. Each neural network will be
// stimulated with identical inputs, in
// the same order
struct teamAgentStaticStimBehaviorData
{
    hostTeamType team;
    agentStaticStimRecord teamMemberBehaviorData[param_numAgents];

    teamAgentStaticStimBehaviorData(hostTeamType assignedTeam):team(assignedTeam)
    {}
};





// Structure for storing behavior data
// of archived agents. Because we
// need to compare against all
// agents in the novelty archive,
// we can't look at every single
// match. We'll just use the first
// opponenet, on the left side.
// Keep in mind, that each generation,
// the opponenets will be different.
// The agents in the novelty archive
// will have to be rerun on the same
// opponent to keep the behavior
// comparisons meaningful.
// The Cga class will contain
// the genomes of the agents in
// the archive, so their phenotypes
// can be created, and rerun against
// the new opponenets. The buffer
// will be updated with that new
// behavior data.
struct noveltyArchiveRecord
{
    int    genomeID;
    char*  data;
    size_t size;

    // For use when using both brain input and output
    // data for behavior recording
    vector <brainBehaviorData> behaviorDataVec;

    // For use when only using brain outputs
    // for behavior recording
    vector <brainOutputData> behaviorOutputDataVec;
};




// Data type for storing archive
// behavior buffer data
struct noveltyArchiveStaticStimRecord
{
    int genomeID;

    char* firstBuffer;
    char* secondBuffer;

    size_t firstBufferSize;
    size_t secondBufferSize;
};






/**********************************************************************
              Variables and data
*********************************************************************/
// These store the serialized behavior data for
// the teams, in the Novelty version that samples
// from actual matches
teamAgentBehaviorData redTeamBehaviorData(REDTEAM);
teamAgentBehaviorData blueTeamBehaviorData(BLUETEAM);


// These store the serialized behavior data for
// the teams, in the Novelty version that uses
// a static set of brain stimulus variables.
teamAgentStaticStimBehaviorData redTeamStaticStimBehavData(REDTEAM);
teamAgentStaticStimBehaviorData blueTeamStaticStimBehavData(BLUETEAM);



// Record of the genome ID's of the opponenets
// a robot had matches against. For error checking.
int blueTeamEnemyGenomeIDs[numMatchesPerAgent/2];
int redTeamEnemyGenomeIDs[numMatchesPerAgent/2];




// Holds the archive of novel behaviors
// for each time. It's like a "hall-of-fame"
// for novel behavior.
vector <noveltyArchiveRecord> redTeamNoveltyArchive;
vector <noveltyArchiveRecord> blueTeamNoveltyArchive;

// Archive for static stimulus mode
vector <noveltyArchiveStaticStimRecord> redTeamNoveltyArchiveStaticStim;
vector <noveltyArchiveStaticStimRecord> blueTeamNoveltyArchiveStaticStim;




// Used for the condition where neural networks
// are stimulated with the exact same values,
// and their outputs are used as a sample of
// behavior. The values chosen to include
// here have to be limited.
vector <brainInputData> staticStimulusVector;


bool staticStimulusVectorCreated = false;








/**********************************************************************
              Functions
*********************************************************************/

// This function greatly simplifies a double typed
// behavior measure through rounding, and converting
// to a char types integer. Lots of resolution loss
// but much less memory.
// For example, an 8-byte double of 0.9573855 would become
// a char 1-byte integer of 96
char simplifyBehaviorDouble(double doubleValue)
{
    char returnValue;

    if (doubleValue < 0.0)
    {
        returnValue = (char)(100.0 * (doubleValue - 0.005));
    }
    else if (doubleValue > 0.0)
    {
        returnValue = (char)(100.0 * (doubleValue + 0.005));
    }
    else
    {
        returnValue = (char)(100.0 * doubleValue);
    }

    return returnValue;
}




// A deserialization function, for testing only really
vector<simpleBrainBehaviorData> deserializeAgentBehavior(char* buffer, size_t bufferSize)
{
    vector <simpleBrainBehaviorData> tinyBehaviorDataVec;
    simpleBrainBehaviorData          tinyBehaviorData;
    size_t                           sizeTracker;

    sizeTracker = 0;

    // What buffer format are we using?
    if (param_simplifyBehaviorBuffers)
    {
        while (sizeTracker < bufferSize)
        {
            // Inputs...
            memcpy(&tinyBehaviorData.inputData.foodSensorForward, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.foodSensorForward));
            sizeTracker += sizeof(tinyBehaviorData.inputData.foodSensorForward);

            memcpy(&tinyBehaviorData.inputData.foodSensorLeft, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.foodSensorLeft));
            sizeTracker += sizeof(tinyBehaviorData.inputData.foodSensorLeft);

            memcpy(&tinyBehaviorData.inputData.foodSensorRight, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.foodSensorRight));
            sizeTracker += sizeof(tinyBehaviorData.inputData.foodSensorRight);

            memcpy(&tinyBehaviorData.inputData.foodSensorBackLeft, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.foodSensorBackLeft));
            sizeTracker += sizeof(tinyBehaviorData.inputData.foodSensorBackLeft);

            memcpy(&tinyBehaviorData.inputData.foodSensorBackRight, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.foodSensorBackRight));
            sizeTracker += sizeof(tinyBehaviorData.inputData.foodSensorBackRight);

            memcpy(&tinyBehaviorData.inputData.enemySensorForward, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.enemySensorForward));
            sizeTracker += sizeof(tinyBehaviorData.inputData.enemySensorForward);

            memcpy(&tinyBehaviorData.inputData.enemySensorLeft, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.enemySensorLeft));
            sizeTracker += sizeof(tinyBehaviorData.inputData.enemySensorLeft);

            memcpy(&tinyBehaviorData.inputData.enemySensorRight, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.enemySensorRight));
            sizeTracker += sizeof(tinyBehaviorData.inputData.enemySensorRight);

            memcpy(&tinyBehaviorData.inputData.enemySensorBackLeft, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.enemySensorBackLeft));
            sizeTracker += sizeof(tinyBehaviorData.inputData.enemySensorBackLeft);

            memcpy(&tinyBehaviorData.inputData.enemySensorBackRight, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.enemySensorBackRight));
            sizeTracker += sizeof(tinyBehaviorData.inputData.enemySensorBackRight);

            memcpy(&tinyBehaviorData.inputData.wallSensor, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.wallSensor));
            sizeTracker += sizeof(tinyBehaviorData.inputData.wallSensor);

            memcpy(&tinyBehaviorData.inputData.energyDifferenceSensor, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.inputData.energyDifferenceSensor));
            sizeTracker += sizeof(tinyBehaviorData.inputData.energyDifferenceSensor);

            // Outputs....
            memcpy(&tinyBehaviorData.outputData.leftOutput, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.outputData.leftOutput));
            sizeTracker += sizeof(tinyBehaviorData.outputData.leftOutput);

            memcpy(&tinyBehaviorData.outputData.rightOutput, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.outputData.rightOutput));
            sizeTracker += sizeof(tinyBehaviorData.outputData.rightOutput);

            memcpy(&tinyBehaviorData.outputData.forwardOutput, &buffer[sizeTracker],
                   sizeof(tinyBehaviorData.outputData.forwardOutput));
            sizeTracker += sizeof(tinyBehaviorData.outputData.forwardOutput);

            tinyBehaviorDataVec.push_back(tinyBehaviorData);
        }
    }
    else
    {
        cout<<"Non-simplified buffer deserialization not handled!"<<endl;
        exit(1);
    }

    return tinyBehaviorDataVec;
}









// When a match is done, this function will be called by the execution
// threads so that the behavior data can be serialized in a format
// that can be used with NCD. NCD usage in this software is that it
// will calculate a normalized compression distance between two
// binary buffers. This function is what takes "behavior data",
// and puts it into a buffer NCD can work with
void serializeAgentBehavior(hostTeamType goodGuyTeam, int robotIndex,
                            unsigned int enemyIndex, unsigned int enemyGenomeID,
                            evalSide startSide)
{
    teamAgentBehaviorData* teamPointer;
    int                    expectedGenomeId;
    unsigned int           calculatedEnemyIndex;

    calculatedEnemyIndex = enemyIndex;

    switch(goodGuyTeam)
    {
    case REDTEAM:
        teamPointer      = &redTeamBehaviorData;
        expectedGenomeId = redTeamEnemyGenomeIDs[enemyIndex];
        break;

    case BLUETEAM:
        teamPointer      = &blueTeamBehaviorData;
        expectedGenomeId = blueTeamEnemyGenomeIDs[enemyIndex];
        break;

    default:
        cout<<"In serializeAgentBehavior, invalid team type."<<endl;
        exit(1);
    }

    if (teamPointer->team != goodGuyTeam)
    {
        cout<<"Error in serializedAgentBehavior, teams don't match!"<<endl;
        exit(1);
    }

    if (expectedGenomeId != enemyGenomeID)
    {
        cout<<"Error in serializeAgentBehavior, mismatched enemy genome IDs!"<<endl;
        exit(1);
    }


    // There's two array indexes per enemy, one for left, one for right.
    // So enemyIndex 0 will take up index 0 and 1, 0 for  left, and 1 for right.
    // This sets up the index for a left side eval
    calculatedEnemyIndex *= 2;

    // If it's a right side eval, add one more to the index number
    if (startSide == RIGHT)
    {
        calculatedEnemyIndex += 1;
    }



    // Serialize the data....
    // How many records are we serializing?
    int numBehaviorRecords;
    if (UseBrainOutputOnly)
    {
        numBehaviorRecords = (int)teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedOutputData.size();
    }
    else
    {
        numBehaviorRecords = (int)teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.size();
    }

    // Do we simplify the behavior data to take
    // up very little space, or use the default
    // data types and size?
    if (param_simplifyBehaviorBuffers)
    {
        // We're simplifying. Take the double typed behavior data, round it and convert to a small int.
        // Use char type for storing these small range int's. This takes the size of struct data type
        // from 128 bytes to 15. If we're using output data from the brains, it's even smaller.
        simpleBrainBehaviorData   tinyBehaviorData;
        simplifiedBrainOutputData tinyOutputBehaviorData;

        brainBehaviorData         fullBehaviorData;
        brainOutputData           outputBehaviorData;

        size_t                    bufferSize;

        // Figure out how big the buffer will be
        // This depends on whether we're using input and output
        // behavior data, or just output only behavior data
        if (UseBrainOutputOnly)
        {
            bufferSize = sizeof(simplifiedBrainOutputData) * numBehaviorRecords;
        }
        else
        {
            bufferSize = sizeof(simpleBrainBehaviorData) * numBehaviorRecords;
        }


        // Allocate the buffer space
        teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.bufferSize  = bufferSize;
        teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer      = new char[bufferSize]();

        // Do we even care about buffer labels in our use of NCD??? Meh....
        teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.bufferLabel = "";

        size_t currentBufferSize = 0;



        // Simplify, and serialize the data
        for (int i = 0; i < numBehaviorRecords; i++)
        {

            if (UseBrainOutputOnly)
            {
                outputBehaviorData = teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedOutputData.at(i).outputData;

                // Simplify the data first
                tinyOutputBehaviorData.forwardOutput = simplifyBehaviorDouble(outputBehaviorData.forwardOutput);
                tinyOutputBehaviorData.leftOutput    = simplifyBehaviorDouble(outputBehaviorData.leftOutput);
                tinyOutputBehaviorData.rightOutput   = simplifyBehaviorDouble(outputBehaviorData.rightOutput);

                // Now serialize the data
                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyOutputBehaviorData.forwardOutput,
                       sizeof(tinyOutputBehaviorData.forwardOutput));
                currentBufferSize += sizeof(tinyOutputBehaviorData.forwardOutput);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyOutputBehaviorData.leftOutput,
                       sizeof(tinyOutputBehaviorData.leftOutput));
                currentBufferSize += sizeof(tinyOutputBehaviorData.leftOutput);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyOutputBehaviorData.rightOutput,
                       sizeof(tinyOutputBehaviorData.rightOutput));
                currentBufferSize += sizeof(tinyOutputBehaviorData.rightOutput);
            }
            else
            {
                fullBehaviorData = teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot;

                // Simplify the data first
                // inputs first
                tinyBehaviorData.inputData.foodSensorForward      = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorForward);
                tinyBehaviorData.inputData.foodSensorLeft         = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorLeft);
                tinyBehaviorData.inputData.foodSensorRight        = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorRight);
                tinyBehaviorData.inputData.foodSensorBackLeft     = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorBackLeft);
                tinyBehaviorData.inputData.foodSensorBackRight    = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorBackRight);
                tinyBehaviorData.inputData.enemySensorForward     = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorForward);
                tinyBehaviorData.inputData.enemySensorLeft        = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorLeft);
                tinyBehaviorData.inputData.enemySensorRight       = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorRight);
                tinyBehaviorData.inputData.enemySensorBackLeft    = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorBackLeft);
                tinyBehaviorData.inputData.enemySensorBackRight   = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorBackRight);
                tinyBehaviorData.inputData.wallSensor             = simplifyBehaviorDouble(fullBehaviorData.inputData.wallSensor);
                tinyBehaviorData.inputData.energyDifferenceSensor = simplifyBehaviorDouble(fullBehaviorData.inputData.energyDifferenceSensor);

                // now the outputs
                tinyBehaviorData.outputData.leftOutput    = simplifyBehaviorDouble(fullBehaviorData.outputData.leftOutput);
                tinyBehaviorData.outputData.rightOutput   = simplifyBehaviorDouble(fullBehaviorData.outputData.rightOutput);
                tinyBehaviorData.outputData.forwardOutput = simplifyBehaviorDouble(fullBehaviorData.outputData.forwardOutput);

                // Now serialize
                // Copy the neural network inputs into the buffer...
                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorForward,
                       sizeof(tinyBehaviorData.inputData.foodSensorForward));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorForward);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorLeft,
                       sizeof(tinyBehaviorData.inputData.foodSensorLeft));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorLeft);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorRight,
                       sizeof(tinyBehaviorData.inputData.foodSensorRight));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorRight);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorBackLeft,
                       sizeof(tinyBehaviorData.inputData.foodSensorBackLeft));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorBackLeft);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorBackRight,
                       sizeof(tinyBehaviorData.inputData.foodSensorBackRight));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorBackRight);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorForward,
                       sizeof(tinyBehaviorData.inputData.enemySensorForward));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorForward);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorLeft,
                       sizeof(tinyBehaviorData.inputData.enemySensorLeft));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorLeft);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorRight,
                       sizeof(tinyBehaviorData.inputData.enemySensorRight));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorRight);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorBackLeft,
                       sizeof(tinyBehaviorData.inputData.enemySensorBackLeft));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorBackLeft);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorBackRight,
                       sizeof(tinyBehaviorData.inputData.enemySensorBackRight));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorBackRight);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.wallSensor,
                       sizeof(tinyBehaviorData.inputData.wallSensor));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.wallSensor);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.inputData.energyDifferenceSensor,
                       sizeof(tinyBehaviorData.inputData.energyDifferenceSensor));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.energyDifferenceSensor);

                // Copy the neural network outputs into the buffer...
                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.outputData.leftOutput,
                       sizeof(tinyBehaviorData.outputData.leftOutput));
                currentBufferSize += sizeof(tinyBehaviorData.outputData.leftOutput);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.outputData.rightOutput,
                       sizeof(tinyBehaviorData.outputData.rightOutput));
                currentBufferSize += sizeof(tinyBehaviorData.outputData.rightOutput);

                memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                       &tinyBehaviorData.outputData.forwardOutput,
                       sizeof(tinyBehaviorData.outputData.forwardOutput));
                currentBufferSize += sizeof(tinyBehaviorData.outputData.forwardOutput);
            }
        }
    }
    else
    {
        // Figure out how big the buffer will be
        size_t bufferSize = sizeof(brainBehaviorData) * numBehaviorRecords;

        // Allocate the buffer space
        teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.bufferSize  = bufferSize;
        teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer      = new char[bufferSize]();

        // Do we even care about buffer labels in our use of NCD??? Meh....
        teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.bufferLabel = "";

        size_t currentBufferSize = 0;


        // Serialize the data into a char*
        for (int i = 0; i < numBehaviorRecords; i++)
        {
            // Copy the neural network inputs into the buffer...
            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorForward,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorForward));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorForward);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorLeft,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorLeft));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorLeft);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorRight,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorRight));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorRight);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorBackLeft,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorBackLeft));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorBackLeft);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorBackRight,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorBackRight));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.foodSensorBackRight);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorForward,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorForward));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorForward);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorLeft,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorLeft));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorLeft);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorRight,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorRight));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorRight);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorBackLeft,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorBackLeft));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorBackLeft);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorBackRight,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorBackRight));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.enemySensorBackRight);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.wallSensor,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.wallSensor));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.wallSensor);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.energyDifferenceSensor,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.energyDifferenceSensor));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.energyDifferenceSensor);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.biasInput,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.biasInput));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.inputData.biasInput);


            // Copy the neural network outputs into the buffer...
            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.outputData.leftOutput,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.outputData.leftOutput));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.outputData.leftOutput);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.outputData.rightOutput,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.outputData.rightOutput));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.outputData.rightOutput);

            memcpy(&teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].ncdBufferData.buffer[currentBufferSize],
                   &teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.outputData.forwardOutput,
                   sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.outputData.forwardOutput));
            currentBufferSize += sizeof(teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.at(i).behaviorSnapshot.outputData.forwardOutput);
        }
    }


    // Save memory, we shouldn't need this data anymore now that we've serialized it into a char* buffer
    teamPointer->agentData[robotIndex].matchArray[calculatedEnemyIndex].reportedData.clear();
}











// When a archived behavior match is redone with a new opponent,
// this function will be called by the execution
// threads so that the behavior data can be serialized in a format
// that can be used with NCD. NCD usage in this software is that it
// will calculate a normalized compression distance between two
// binary buffers. This function is what takes "behavior data",
// and puts it into a buffer NCD can work with
void serializeArchivedAgentBehavior(hostTeamType team, int robotIndex, int genomeID)
{
    noveltyArchiveRecord* recordPointer;

    switch(team)
    {
    case REDTEAM:
        if ((robotIndex < 0) || (robotIndex > redTeamNoveltyArchive.size()-1))
        {
            cout<<"Error in serializeArchivedAgentBehavior, invalide robot index!"<<endl;
            exit(1);
        }

        recordPointer = &redTeamNoveltyArchive[robotIndex];
        break;

    case BLUETEAM:
        if ((robotIndex < 0) || (robotIndex > blueTeamNoveltyArchive.size()-1))
        {
            cout<<"Error in serializeArchivedAgentBehavior, invalide robot index!"<<endl;
            exit(1);
        }

        recordPointer = &blueTeamNoveltyArchive[robotIndex];
        break;
    }


    // Make sure the genome ID's match for sanity checking
    if (genomeID != recordPointer->genomeID)
    {
        cout<<"Error in serializeArchivedAgentBehavior, mismatched genomeID's!"<<endl;
        exit(1);
    }



    // Serialize the data....
    // How many records are we serializing?
    int numBehaviorRecords;

    if (UseBrainOutputOnly)
    {
        numBehaviorRecords = (int)recordPointer->behaviorOutputDataVec.size();
    }
    else
    {
        numBehaviorRecords = (int)recordPointer->behaviorDataVec.size();
    }


    if (param_simplifyBehaviorBuffers)
    {
        // We're simplifying. Take the double typed behavior data, round it and convert to a small int.
        // Use char type for storing these small range int's. This takes the size of struct data type
        // from 128 bytes to 15.
        simpleBrainBehaviorData   tinyBehaviorData;
        simplifiedBrainOutputData tinyOutputBehaviorData;

        brainBehaviorData       fullBehaviorData;
        brainOutputData         outputBehaviorData;

        // Figure out how big the buffer will be
        size_t bufferSize;

        if (UseBrainOutputOnly)
        {
            bufferSize = sizeof(simplifiedBrainOutputData) * numBehaviorRecords;
        }
        else
        {
            bufferSize = sizeof(simpleBrainBehaviorData) * numBehaviorRecords;
        }

        // Allocate the buffer space
        recordPointer->size = bufferSize;
        recordPointer->data = new char[bufferSize]();

        size_t currentBufferSize = 0;

        // Simplify, and serialize the data
        for (int i = 0; i < numBehaviorRecords; i++)
        {
            if (UseBrainOutputOnly)
            {
                outputBehaviorData = recordPointer->behaviorOutputDataVec.at(i);

                // simplify the data first
                tinyOutputBehaviorData.forwardOutput = simplifyBehaviorDouble(outputBehaviorData.forwardOutput);
                tinyOutputBehaviorData.leftOutput    = simplifyBehaviorDouble(outputBehaviorData.leftOutput);
                tinyOutputBehaviorData.rightOutput   = simplifyBehaviorDouble(outputBehaviorData.rightOutput);

                // Now serialize the data
                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyOutputBehaviorData.forwardOutput,
                       sizeof(tinyOutputBehaviorData.forwardOutput));
                currentBufferSize += sizeof(tinyOutputBehaviorData.forwardOutput);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyOutputBehaviorData.leftOutput,
                       sizeof(tinyOutputBehaviorData.leftOutput));
                currentBufferSize += sizeof(tinyOutputBehaviorData.leftOutput);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyOutputBehaviorData.rightOutput,
                       sizeof(tinyOutputBehaviorData.rightOutput));
                currentBufferSize += sizeof(tinyOutputBehaviorData.rightOutput);
            }
            else
            {
                fullBehaviorData = recordPointer->behaviorDataVec.at(i);

                // Simplify the data first
                // inputs first
                tinyBehaviorData.inputData.foodSensorForward      = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorForward);
                tinyBehaviorData.inputData.foodSensorLeft         = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorLeft);
                tinyBehaviorData.inputData.foodSensorRight        = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorRight);
                tinyBehaviorData.inputData.foodSensorBackLeft     = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorBackLeft);
                tinyBehaviorData.inputData.foodSensorBackRight    = simplifyBehaviorDouble(fullBehaviorData.inputData.foodSensorBackRight);
                tinyBehaviorData.inputData.enemySensorForward     = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorForward);
                tinyBehaviorData.inputData.enemySensorLeft        = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorLeft);
                tinyBehaviorData.inputData.enemySensorRight       = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorRight);
                tinyBehaviorData.inputData.enemySensorBackLeft    = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorBackLeft);
                tinyBehaviorData.inputData.enemySensorBackRight   = simplifyBehaviorDouble(fullBehaviorData.inputData.enemySensorBackRight);
                tinyBehaviorData.inputData.wallSensor             = simplifyBehaviorDouble(fullBehaviorData.inputData.wallSensor);
                tinyBehaviorData.inputData.energyDifferenceSensor = simplifyBehaviorDouble(fullBehaviorData.inputData.energyDifferenceSensor);

                // now the outputs
                tinyBehaviorData.outputData.leftOutput    = simplifyBehaviorDouble(fullBehaviorData.outputData.leftOutput);
                tinyBehaviorData.outputData.rightOutput   = simplifyBehaviorDouble(fullBehaviorData.outputData.rightOutput);
                tinyBehaviorData.outputData.forwardOutput = simplifyBehaviorDouble(fullBehaviorData.outputData.forwardOutput);


                // Now serialize
                // Copy the neural network inputs into the buffer...
                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorForward,
                       sizeof(tinyBehaviorData.inputData.foodSensorForward));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorForward);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorLeft,
                       sizeof(tinyBehaviorData.inputData.foodSensorLeft));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorLeft);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorRight,
                       sizeof(tinyBehaviorData.inputData.foodSensorRight));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorRight);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorBackLeft,
                       sizeof(tinyBehaviorData.inputData.foodSensorBackLeft));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorBackLeft);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.foodSensorBackRight,
                       sizeof(tinyBehaviorData.inputData.foodSensorBackRight));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.foodSensorBackRight);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorForward,
                       sizeof(tinyBehaviorData.inputData.enemySensorForward));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorForward);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorLeft,
                       sizeof(tinyBehaviorData.inputData.enemySensorLeft));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorLeft);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorRight,
                       sizeof(tinyBehaviorData.inputData.enemySensorRight));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorRight);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorBackLeft,
                       sizeof(tinyBehaviorData.inputData.enemySensorBackLeft));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorBackLeft);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.enemySensorBackRight,
                       sizeof(tinyBehaviorData.inputData.enemySensorBackRight));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.enemySensorBackRight);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.wallSensor,
                       sizeof(tinyBehaviorData.inputData.wallSensor));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.wallSensor);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.inputData.energyDifferenceSensor,
                       sizeof(tinyBehaviorData.inputData.energyDifferenceSensor));
                currentBufferSize += sizeof(tinyBehaviorData.inputData.energyDifferenceSensor);

                // Copy the neural network outputs into the buffer...
                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.outputData.leftOutput,
                       sizeof(tinyBehaviorData.outputData.leftOutput));
                currentBufferSize += sizeof(tinyBehaviorData.outputData.leftOutput);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.outputData.rightOutput,
                       sizeof(tinyBehaviorData.outputData.rightOutput));
                currentBufferSize += sizeof(tinyBehaviorData.outputData.rightOutput);

                memcpy(&recordPointer->data[currentBufferSize],
                       &tinyBehaviorData.outputData.forwardOutput,
                       sizeof(tinyBehaviorData.outputData.forwardOutput));
                currentBufferSize += sizeof(tinyBehaviorData.outputData.forwardOutput);
            }
        }
    }
    else
    {
        cout<<"Error, trying to use non-simplified behavior buffers, and this isn't supported in serializeArchivedAgentBehavior yet!"<<endl;
        exit(1);
    }


    // Save memory, we shouldn't need this data anymore now that we've serialized it into a char* buffer
    recordPointer->behaviorDataVec.clear();
}









// This function takes to arbitrary binary buffers, and calculates
// a NCD (Normalized Compression Distance) between them. The result will
// be between 0.0 and 1.0. A NCD value of 0.0 would indicate identical
// buffers, and the higher the NCD value, the more difference between
// the data.
// This function relies on the NCD implementation from complearn.org
#ifdef LZMA
double getLzmaNcdValue(char* buffer1, size_t buffer1Size, char* buffer1Label,
                       char* buffer2, size_t buffer2Size, char* buffer2Label)
{
    CLDatumList *list1;
    CLDatumList *list2;

    list1 = CLNewDatumListFromBuffer(buffer1, buffer1Size, buffer1Label);
    list2 = CLNewDatumListFromBuffer(buffer2, buffer2Size, buffer2Label);

    CLCompLearnResult *result;
    result = CLCalculate(CLPair, CLNCD, list1, list2, NULL);

    double ncdValue = CLGetNumberResult(result);

    CLFreeDatumList(list1);
    CLFreeDatumList(list2);
    CLFreeResult(result);

    return ncdValue;
}
#endif









// This function is called to setup the
// list of enemy genome IDs the robots
// will be battling against. This
// will allow these functions to
// have a genomeID -> index number
// lookup list.
void setGenerationEnemySeries(hostTeamType goodGuyTeam, vector<int> enemyGenomesIdents)
{
    if (enemyGenomesIdents.size() != (numMatchesPerAgent/2))
    {
        cout<<"Error in setGenerationEnemySeries, wrong number of enemies: "<<enemyGenomesIdents.size()<<endl;
        exit(1);
    }

    switch (goodGuyTeam)
    {
    case REDTEAM:
        for (int i = 0; i < numMatchesPerAgent/2; i++)
        {
            redTeamEnemyGenomeIDs[i] = enemyGenomesIdents.at(i);
        }
        break;

    case BLUETEAM:
        for (int i = 0; i < numMatchesPerAgent/2; i++)
        {
            blueTeamEnemyGenomeIDs[i] = enemyGenomesIdents.at(i);
        }
        break;

    default:
        cout<<"In setGenerationEnemySeries, invalid team type."<<endl;
        exit(1);
    }
}








// This function looks up the index number for a particular
// enemy genome ID
int lookupEnemyGenomeIdIndex(hostTeamType goodGuyTeam, int enemyGenomeID)
{
    int returnIndex;

    for (int i = 0; i < numMatchesPerAgent/2; i++)
    {
        if (goodGuyTeam == REDTEAM)
        {
            if (redTeamEnemyGenomeIDs[i] == enemyGenomeID)
            {
                return i;
            }
        }
        else if (goodGuyTeam == BLUETEAM)
        {
            if (blueTeamEnemyGenomeIDs[i] == enemyGenomeID)
            {
                return i;
            }
        }
    }

    // If we get here, something is seriously amiss
    cout<<"Error in lookupEnemyGenomeIdIndex, didn't find enemyGenomeID, shouldn't be possible!"<<endl;

    if (goodGuyTeam == REDTEAM)
    {
        cout<<"Failed to find red teams enemy genome ID: "<<enemyGenomeID<<endl;
        exit(1);
    }
    else if (goodGuyTeam == BLUETEAM)
    {
        cout<<"Failed to find blue teams enemy genome ID: "<<enemyGenomeID<<endl;
        exit(1);
    }
    else
    {
        cout<<"Invalid good guy team: "<<(int)goodGuyTeam<<endl;
        exit(1);
    }
}






// How execution threads push agent behavior data
// here to be serialized into a memory buffer
// Left side evaluation data will be stored in the
// first index for an enemy, and the right side evaluation
// will be stored in the next array position
void reportAgentBehavior(agentBehaviorData behaviorData)
{
    teamAgentBehaviorData *teamPointer;
    unsigned int           calculatedEnemyIndex;
    int                    expectedGenomeId;

    calculatedEnemyIndex = behaviorData.enemyIndex;

    switch (behaviorData.robotTeam)
    {
    case REDTEAM:
        teamPointer      = &redTeamBehaviorData;
        expectedGenomeId = redTeamEnemyGenomeIDs[behaviorData.enemyIndex];
        break;

    case BLUETEAM:
        teamPointer      = &blueTeamBehaviorData;
        expectedGenomeId = blueTeamEnemyGenomeIDs[behaviorData.enemyIndex];
        break;

    default:
        cout<<"In reportAgentBehavior, invalid team type."<<endl;
        exit(1);
    }



    if (expectedGenomeId != behaviorData.enemyGenomeID)
    {
        cout<<"Error in reportAgentBehavior, mismatched enemy genome IDs!"<<endl;
        cout<<"We got genomeID: "<<behaviorData.enemyGenomeID<<", expected: "<<expectedGenomeId<<endl;
        cout<<"Enemy Index: "<<behaviorData.enemyIndex<<endl;
        cout<<"Host index: "<<behaviorData.robotIndex<<endl;
        if (behaviorData.robotTeam == REDTEAM)
        {
            cout<<"Host on red team"<<endl;
        }
        else
        {
            cout<<"Host on blue team"<<endl;
        }

        if (behaviorData.robotStartSide == LEFT)
        {
            cout<<"Host start left"<<endl;
        }
        else
        {
            cout<<"Host start right"<<endl;
        }

        cout<<endl<<"Expected vector dumps:"<<endl;

        cout<<"RED TEAM:"<<endl;
        for (int i = 0; i < numMatchesPerAgent/2; i++)
        {
            cout<<"Index: "<<i<<", genomeID: "<<redTeamEnemyGenomeIDs[i]<<endl;
        }

        cout<<"BLUE TEAM:"<<endl;
        for (int i = 0; i < numMatchesPerAgent/2; i++)
        {
            cout<<"Index: "<<i<<", genomeID: "<<blueTeamEnemyGenomeIDs[i]<<endl;
        }

        exit(1);
    }



    // There's two array indexes per enemy, one for left, one for right.
    // So enemyIndex 0 will take up index 0 and 1, 0 for  left, and 1 for right.
    // This sets up the index for a left side eval
    calculatedEnemyIndex *= 2;

    // If it's a right side eval, add one more to the index number
    if (behaviorData.robotStartSide == RIGHT)
    {
        calculatedEnemyIndex += 1;
    }


    // Now, add the behavior data to the records
    teamPointer->agentData[behaviorData.robotIndex].robotIndex = behaviorData.robotIndex;
    teamPointer->agentData[behaviorData.robotIndex].robotTeam  = behaviorData.robotTeam;

    agentBehaviorData       newBehaviorData;
    agentOutputBehaviorData newOutputBehaviorData;

    // Are we using both brain inputs and outputs? Or just outputs?
    if (UseBrainOutputOnly)
    {
        newOutputBehaviorData.robotIndex       = behaviorData.robotIndex;
        newOutputBehaviorData.robotTeam        = behaviorData.robotTeam;
        newOutputBehaviorData.robotStartSide   = behaviorData.robotStartSide;
        newOutputBehaviorData.enemyIndex       = behaviorData.enemyIndex;
        newOutputBehaviorData.enemyGenomeID    = behaviorData.enemyGenomeID;

        newOutputBehaviorData.outputData.forwardOutput = behaviorData.behaviorSnapshot.outputData.forwardOutput;
        newOutputBehaviorData.outputData.leftOutput    = behaviorData.behaviorSnapshot.outputData.leftOutput;
        newOutputBehaviorData.outputData.rightOutput   = behaviorData.behaviorSnapshot.outputData.rightOutput;

        teamPointer->agentData[behaviorData.robotIndex].matchArray[calculatedEnemyIndex].reportedOutputData.push_back(newOutputBehaviorData);
    }
    else
    {
        newBehaviorData.robotIndex       = behaviorData.robotIndex;
        newBehaviorData.robotTeam        = behaviorData.robotTeam;
        newBehaviorData.robotStartSide   = behaviorData.robotStartSide;
        newBehaviorData.enemyIndex       = behaviorData.enemyIndex;
        newBehaviorData.enemyGenomeID    = behaviorData.enemyGenomeID;
        newBehaviorData.behaviorSnapshot = behaviorData.behaviorSnapshot;

        teamPointer->agentData[behaviorData.robotIndex].matchArray[calculatedEnemyIndex].reportedData.push_back(newBehaviorData);
    }
}






// How execution threads push agent behavior data
// here to be serialized into a memory buffer
// This function is only used in Novelty search
// with static stimulus.
void reportAndSerializeAgentStaticStimBehavior(agentStaticStimBehaviorData behaviorData)
{
    if (!UseStaticStimNovelty)
    {
        cout<<"Error in reportAndSerializeAgentStaticStimBehavior, shouldn't be here unless we're in static stim mode"<<endl;
        exit(1);
    }

    teamAgentStaticStimBehaviorData *teamPointer;

    switch (behaviorData.robotTeam)
    {
    case REDTEAM:
        teamPointer = &redTeamStaticStimBehavData;
        break;

    case BLUETEAM:
        teamPointer = &blueTeamStaticStimBehavData;
        break;

    default:
        cout<<"In reportAndSerializeAgentStaticStimBehavior, invalid team type"<<endl;
        exit(1);
    }

    teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].robotIndex     = behaviorData.robotIndex;
    teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].robotTeam      = behaviorData.robotTeam;
    teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].genomeID       = behaviorData.genomeID;
    teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].brainOutputVec = behaviorData.brainOutputVector;


    // Now serialize the data
    if (teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].brainOutputVec.size() != expectedNumStaticStims)
    {
        cout<<"Error in reportAndSerializeAgentStaticStimBehavior, invalid vector size"<<endl;
        exit(1);
    }

    // Buffer size is the size of 1 of 2 equal sized buffers.
    size_t bufferSize = staticStimBufferSplit * sizeof(simplifiedBrainOutputData);

    // Both buffers are equal sized, with the behavior data split in half
    teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].firstBufferSize  = bufferSize;
    teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].secondBufferSize = bufferSize;

    // Allocate buffer memory
    teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].firstBuffer  = new char[bufferSize]();
    teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].secondBuffer = new char[bufferSize]();

    size_t currentBufferSize = 0;

    brainOutputData           outputBehaviorData;
    simplifiedBrainOutputData tinyOutputBehaviorData;

    // Simplify and serialize the data for the first buffer
    for (int i = 0; i < staticStimBufferSplit; i++)
    {
        outputBehaviorData = teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].brainOutputVec.at(i);

        tinyOutputBehaviorData.forwardOutput = simplifyBehaviorDouble(outputBehaviorData.forwardOutput);
        tinyOutputBehaviorData.leftOutput    = simplifyBehaviorDouble(outputBehaviorData.leftOutput);
        tinyOutputBehaviorData.rightOutput   = simplifyBehaviorDouble(outputBehaviorData.rightOutput);

        // now serialize the data
        memcpy(&teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].firstBuffer[currentBufferSize],
               &tinyOutputBehaviorData.forwardOutput,
               sizeof(tinyOutputBehaviorData.forwardOutput));
        currentBufferSize += sizeof(tinyOutputBehaviorData.forwardOutput);

        memcpy(&teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].firstBuffer[currentBufferSize],
               &tinyOutputBehaviorData.leftOutput,
               sizeof(tinyOutputBehaviorData.leftOutput));
        currentBufferSize += sizeof(tinyOutputBehaviorData.leftOutput);

        memcpy(&teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].firstBuffer[currentBufferSize],
               &tinyOutputBehaviorData.rightOutput,
               sizeof(tinyOutputBehaviorData.rightOutput));
        currentBufferSize += sizeof(tinyOutputBehaviorData.rightOutput);
    }

    // reset buffer counter
    currentBufferSize = 0;

    // Simplify and serialize the data for the second buffer
    for (int i = staticStimBufferSplit; i < expectedNumStaticStims; i++)
    {
        outputBehaviorData = teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].brainOutputVec.at(i);

        tinyOutputBehaviorData.forwardOutput = simplifyBehaviorDouble(outputBehaviorData.forwardOutput);
        tinyOutputBehaviorData.leftOutput    = simplifyBehaviorDouble(outputBehaviorData.leftOutput);
        tinyOutputBehaviorData.rightOutput   = simplifyBehaviorDouble(outputBehaviorData.rightOutput);

        // now serialize the data
        memcpy(&teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].secondBuffer[currentBufferSize],
               &tinyOutputBehaviorData.forwardOutput,
               sizeof(tinyOutputBehaviorData.forwardOutput));
        currentBufferSize += sizeof(tinyOutputBehaviorData.forwardOutput);

        memcpy(&teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].secondBuffer[currentBufferSize],
               &tinyOutputBehaviorData.leftOutput,
               sizeof(tinyOutputBehaviorData.leftOutput));
        currentBufferSize += sizeof(tinyOutputBehaviorData.leftOutput);

        memcpy(&teamPointer->teamMemberBehaviorData[behaviorData.robotIndex].secondBuffer[currentBufferSize],
               &tinyOutputBehaviorData.rightOutput,
               sizeof(tinyOutputBehaviorData.rightOutput));
        currentBufferSize += sizeof(tinyOutputBehaviorData.rightOutput);
    }
}







// How execution threads push agent behavior data here to
// be serialized into a memory buffer. This version is for
// re-running agents that are in the novelty archive, so that
// they're recorded behavior is against the same opponenets as
// that the standard population is against. It will actually
// only be a single opponenet, from one side for speed sake in
// comparisons, but it will the same one the standard population
// had matches against.
void reportArchivedAgentBehavior(archiveAgentBehaviorData behaviorData)
{
    switch (behaviorData.robotTeam)
    {
    case REDTEAM:
        if (behaviorData.genomeID != redTeamNoveltyArchive[behaviorData.robotIndex].genomeID)
        {
            cout<<"Mismatched genome ID's in reportArchivedAgentBehavior!"<<endl;
            exit(1);
        }

        // Are we using both inputs and outputs for behavior recording, or just
        // outputs?
        if (UseBrainOutputOnly)
        {
            redTeamNoveltyArchive[behaviorData.robotIndex].behaviorOutputDataVec.push_back(behaviorData.behaviorSnapshot.outputData);
        }
        else
        {
            redTeamNoveltyArchive[behaviorData.robotIndex].behaviorDataVec.push_back(behaviorData.behaviorSnapshot);
        }
        break;

    case BLUETEAM:
        if (behaviorData.genomeID != blueTeamNoveltyArchive[behaviorData.robotIndex].genomeID)
        {
            cout<<"Mismatched genome ID's in reportArchivedAgentBehavior!"<<endl;
            exit(1);
        }

        // Are we using both inputs and outputs for behavior recording, or just
        // outputs?
        if (UseBrainOutputOnly)
        {
            blueTeamNoveltyArchive[behaviorData.robotIndex].behaviorOutputDataVec.push_back(behaviorData.behaviorSnapshot.outputData);
        }
        else
        {
            blueTeamNoveltyArchive[behaviorData.robotIndex].behaviorDataVec.push_back(behaviorData.behaviorSnapshot);
        }
        break;
    }
}







// This function is called after a generations
// behavior data has been processed and used.
// This will deallocate memory.
void clearBehaviorDataMemory()
{
    for (int i = 0; i < param_numAgents; i++)
    {
        if (UseStaticStimNovelty)
        {
            redTeamStaticStimBehavData.teamMemberBehaviorData[i].firstBufferSize  = 0;
            redTeamStaticStimBehavData.teamMemberBehaviorData[i].secondBufferSize = 0;
            redTeamStaticStimBehavData.teamMemberBehaviorData[i].brainOutputVec.clear();
            delete[] redTeamStaticStimBehavData.teamMemberBehaviorData[i].firstBuffer;
            delete[] redTeamStaticStimBehavData.teamMemberBehaviorData[i].secondBuffer;

            blueTeamStaticStimBehavData.teamMemberBehaviorData[i].firstBufferSize  = 0;
            blueTeamStaticStimBehavData.teamMemberBehaviorData[i].secondBufferSize = 0;
            blueTeamStaticStimBehavData.teamMemberBehaviorData[i].brainOutputVec.clear();
            delete[] blueTeamStaticStimBehavData.teamMemberBehaviorData[i].firstBuffer;
            delete[] blueTeamStaticStimBehavData.teamMemberBehaviorData[i].secondBuffer;
        }
        else
        {
            for (int j = 0; j < numMatchesPerAgent; j++)
            {
                redTeamBehaviorData.agentData[i].matchArray[j].reportedData.clear();
                redTeamBehaviorData.agentData[i].matchArray[j].reportedOutputData.clear();
                redTeamBehaviorData.agentData[i].matchArray[j].ncdBufferData.bufferSize = 0;
                delete[] redTeamBehaviorData.agentData[i].matchArray[j].ncdBufferData.buffer;

                blueTeamBehaviorData.agentData[i].matchArray[j].reportedData.clear();
                blueTeamBehaviorData.agentData[i].matchArray[j].reportedOutputData.clear();
                blueTeamBehaviorData.agentData[i].matchArray[j].ncdBufferData.bufferSize = 0;
                delete[] blueTeamBehaviorData.agentData[i].matchArray[j].ncdBufferData.buffer;
            }
        }
    }

    // If we're using static stimulus, no need to clear
    // the archive data, since it'll still be valid
    // generation after generations.
    if (!UseStaticStimNovelty)
    {
        // Clear all the behavior data for the novelty archive, as it
        // will be redone for the next generation with new matches.
        // Keep the genome ID though, for sanity checking.
        for (unsigned int i = 0; i < redTeamNoveltyArchive.size(); i++)
        {
            redTeamNoveltyArchive[i].behaviorDataVec.clear();
            redTeamNoveltyArchive[i].behaviorOutputDataVec.clear();
            redTeamNoveltyArchive[i].size = 0;
            delete[] redTeamNoveltyArchive[i].data;
        }

        for (unsigned int i = 0; i < blueTeamNoveltyArchive.size(); i++)
        {
            blueTeamNoveltyArchive[i].behaviorDataVec.clear();
            blueTeamNoveltyArchive[i].behaviorOutputDataVec.clear();
            blueTeamNoveltyArchive[i].size = 0;
            delete[] blueTeamNoveltyArchive[i].data;
        }
    }
}






// Returns an NCD score between two robots on the same team.
// It compares each of the 24 matches the robots had
// against the match the other had. It averages all
// of these NCD values and returns it.
double getInterteamNcdValue(hostTeamType team, int robotIndex1, int robotIndex2)
{
    teamAgentBehaviorData* teamPointer;
    double ncdValue = 0.0;

    switch(team)
    {
    case REDTEAM:
        teamPointer = &redTeamBehaviorData;
        break;

    case BLUETEAM:
        teamPointer = &blueTeamBehaviorData;
        break;

    default:
        cout<<"In serializeAgentBehavior, invalid team type."<<endl;
        exit(1);
    }


    NcdZlibCalc   ncdCalculator;
    NcdDataObject robot1Data;
    NcdDataObject robot2Data;


    for (int i = 0; i < numMatchesPerAgent; i++)
    {
        robot1Data.data = teamPointer->agentData[robotIndex1].matchArray[i].ncdBufferData.buffer;
        robot1Data.size = teamPointer->agentData[robotIndex1].matchArray[i].ncdBufferData.bufferSize;

        robot2Data.data = teamPointer->agentData[robotIndex2].matchArray[i].ncdBufferData.buffer;
        robot2Data.size = teamPointer->agentData[robotIndex2].matchArray[i].ncdBufferData.bufferSize;

        ncdValue += ncdCalculator.GetNcdValue(robot1Data, robot2Data);
    }

    ncdValue /= (double)numMatchesPerAgent;

    return ncdValue;
}







// Returns an NCD score of a robot. Many shortcut/compromises
// are made here to limit the number of NCD calculations
double calculateNcdScoreForRobot(hostTeamType robotTeam, int robotIndex)
{
    double       ncdValue              = 0.0;
    int          ncdValueCounter       = 0;
    int          numArchivedBehaviors;
    int          calculatedIndex;
    vector <int> comparisonIndexVector;

    teamAgentBehaviorData*           teamPointer;
    teamAgentStaticStimBehaviorData* teamStaticStimPointer;

    NcdZlibCalc            ncdCalculator;
    NcdDataObject          robotData;
    NcdDataObject          comparisonData;

    vector <NcdDataObject> teammateDataVector;
    vector <NcdDataObject> archiveDataVector;

    // For static stimulus comparisons
    NcdDataObject  robotData1;
    NcdDataObject  robotData2;

    NcdDataObject  comparisonData1;
    NcdDataObject  comparisonData2;

    vector <NcdDataObject> teammateData1Vector;
    vector <NcdDataObject> teammateData2Vector;

    vector <NcdDataObject> archiveData1Vector;
    vector <NcdDataObject> archiveData2Vector;



    // Simple error checking....
    if ((robotIndex < 0) || (robotIndex >= param_numAgents))
    {
        cout<<"In calculateNcdScoreForRobot, invalid robotIndex: "<<robotIndex<<endl;
        exit(1);
    }


    // Setup a simple vector with the index's of all
    // the teammate robot index's that the robot
    // will be compared against. If the teammate
    // index is less than 0, or more than the number
    // of agents on the team, the index's will roll
    // around to the other side
    // If param_numNeighborNcdComparisons is 10, the
    // robot will be compared to the 5 robots before it
    // int the index, and 5 robots after it in the index.
    if (!StaticStimExpandedComparisons)
    {
        for (int i = 1; i <= param_numNeighborNcdComparisons/2; i++)
        {
            calculatedIndex = robotIndex - i;

            if (calculatedIndex < 0)
            {
                calculatedIndex += param_numAgents;
            }

            comparisonIndexVector.push_back(calculatedIndex);

            calculatedIndex = robotIndex +i;

            if (calculatedIndex >= param_numAgents)
            {
                calculatedIndex -= param_numAgents;
            }

            comparisonIndexVector.push_back(calculatedIndex);
        }
    }


    switch(robotTeam)
    {
    case REDTEAM:
        if (UseStaticStimNovelty)
        {
            teamStaticStimPointer = &redTeamStaticStimBehavData;
        }
        else
        {
            teamPointer = &redTeamBehaviorData;
        }

        numArchivedBehaviors = redTeamNoveltyArchive.size();
        break;

    case BLUETEAM:
        if (UseStaticStimNovelty)
        {
            teamStaticStimPointer = &blueTeamStaticStimBehavData;
        }
        else
        {
            teamPointer = &blueTeamBehaviorData;
        }

        numArchivedBehaviors = blueTeamNoveltyArchive.size();
        break;

    default:
        cout<<"In serializeAgentBehavior, invalid team type."<<endl;
        exit(1);
    }



    // Compare behavior to teammates....

    // Compare all the individual matches for the robot against
    // those matches from the teammates.
    // This only compares the left side matches, they're the even
    // numbered indexes. Left and right matches against the same
    // opponent shouldn't differ too much. Need the performance
    // boost.
    if (UseStaticStimNovelty)
    {
        // There's 2 comparisons per agent (the static stim behavior data is split
        // into two buffers because of zlib max sizes for ncd comparisons)
        teammateData1Vector.clear();
        teammateData2Vector.clear();

        robotData1.data = teamStaticStimPointer->teamMemberBehaviorData[robotIndex].firstBuffer;
        robotData1.size = teamStaticStimPointer->teamMemberBehaviorData[robotIndex].firstBufferSize;

        robotData2.data = teamStaticStimPointer->teamMemberBehaviorData[robotIndex].secondBuffer;
        robotData2.size = teamStaticStimPointer->teamMemberBehaviorData[robotIndex].secondBufferSize;

        if (StaticStimExpandedComparisons)
        {
            for (int i = 0; i < param_numAgents; i++)
            {
                // Make sure you don't compare the robot's behavior to itself, but all other
                // members of the team.
                if (i != robotIndex)
                {
                    comparisonData1.data = teamStaticStimPointer->teamMemberBehaviorData[i].firstBuffer;
                    comparisonData1.size = teamStaticStimPointer->teamMemberBehaviorData[i].firstBufferSize;
                    teammateData1Vector.push_back(comparisonData1);

                    comparisonData2.data = teamStaticStimPointer->teamMemberBehaviorData[i].secondBuffer;
                    comparisonData2.size = teamStaticStimPointer->teamMemberBehaviorData[i].secondBufferSize;
                    teammateData2Vector.push_back(comparisonData2);
                }
            }
        }
        else
        {
            for (int i = 0; i < comparisonIndexVector.size(); i++)
            {
                comparisonData1.data = teamStaticStimPointer->teamMemberBehaviorData[comparisonIndexVector[i]].firstBuffer;
                comparisonData1.size = teamStaticStimPointer->teamMemberBehaviorData[comparisonIndexVector[i]].firstBufferSize;
                teammateData1Vector.push_back(comparisonData1);

                comparisonData2.data = teamStaticStimPointer->teamMemberBehaviorData[comparisonIndexVector[i]].secondBuffer;
                comparisonData2.size = teamStaticStimPointer->teamMemberBehaviorData[comparisonIndexVector[i]].secondBufferSize;
                teammateData2Vector.push_back(comparisonData2);
            }
        }

        // Calculate NCD value
        ncdValue += ncdCalculator.GetNonAveragedNcdValue(robotData1, teammateData1Vector);
        ncdValueCounter += teammateData1Vector.size();

        ncdValue += ncdCalculator.GetNonAveragedNcdValue(robotData2, teammateData2Vector);
        ncdValueCounter += teammateData2Vector.size();
    }
    else
    {
        for (int i = 0; i < numMatchesPerAgent; i+=2)
        {
            teammateDataVector.clear();

            robotData.data = teamPointer->agentData[robotIndex].matchArray[i].ncdBufferData.buffer;
            robotData.size = teamPointer->agentData[robotIndex].matchArray[i].ncdBufferData.bufferSize;

            for (int teammateCounter = 0; teammateCounter < comparisonIndexVector.size(); teammateCounter++)
            {
                comparisonData.data = teamPointer->agentData[comparisonIndexVector[teammateCounter]].matchArray[i].ncdBufferData.buffer;
                comparisonData.size = teamPointer->agentData[comparisonIndexVector[teammateCounter]].matchArray[i].ncdBufferData.bufferSize;
                teammateDataVector.push_back(comparisonData);
            }

            ncdValue        += ncdCalculator.GetNonAveragedNcdValue(robotData, teammateDataVector);
            ncdValueCounter += teammateDataVector.size();
        }
    }


    // Compare behavior to archive....
    // We'll do much smaller comparisons against the behavior archive. Only
    // compare against a single match, not all 24, or all the left side (match index 0).
    // We want to compare against all the archive, even if on such a limited
    // basis.
    if (UseStaticStimNovelty)
    {
        if (numArchivedBehaviors > 0)
        {
            archiveData1Vector.clear();
            archiveData2Vector.clear();

            robotData1.data = teamStaticStimPointer->teamMemberBehaviorData[robotIndex].firstBuffer;
            robotData1.size = teamStaticStimPointer->teamMemberBehaviorData[robotIndex].firstBufferSize;

            robotData2.data = teamStaticStimPointer->teamMemberBehaviorData[robotIndex].secondBuffer;
            robotData2.size = teamStaticStimPointer->teamMemberBehaviorData[robotIndex].secondBufferSize;

            for (int i = 0; i < numArchivedBehaviors; i++)
            {
                switch (robotTeam)
                {
                case REDTEAM:
                    comparisonData1.data = redTeamNoveltyArchiveStaticStim.at(i).firstBuffer;
                    comparisonData1.size = redTeamNoveltyArchiveStaticStim.at(i).firstBufferSize;
                    archiveData1Vector.push_back(comparisonData1);

                    comparisonData2.data = redTeamNoveltyArchiveStaticStim.at(i).secondBuffer;
                    comparisonData2.size = redTeamNoveltyArchiveStaticStim.at(i).secondBufferSize;
                    archiveData2Vector.push_back(comparisonData2);
                    break;

                case BLUETEAM:
                    comparisonData1.data = blueTeamNoveltyArchiveStaticStim.at(i).firstBuffer;
                    comparisonData1.size = blueTeamNoveltyArchiveStaticStim.at(i).firstBufferSize;
                    archiveData1Vector.push_back(comparisonData1);

                    comparisonData2.data = blueTeamNoveltyArchiveStaticStim.at(i).secondBuffer;
                    comparisonData2.size = blueTeamNoveltyArchiveStaticStim.at(i).secondBufferSize;
                    archiveData2Vector.push_back(comparisonData2);
                    break;
                }
            }

            ncdValue        += ncdCalculator.GetNonAveragedNcdValue(robotData1, archiveData1Vector);
            ncdValueCounter += archiveData1Vector.size();

            ncdValue        += ncdCalculator.GetNonAveragedNcdValue(robotData2, archiveData2Vector);
            ncdValueCounter += archiveData2Vector.size();
        }
    }
    else
    {
        // Dynamic match behavior version
        if (numArchivedBehaviors > 0)
        {
            archiveDataVector.clear();

            robotData.data = teamPointer->agentData[robotIndex].matchArray[0].ncdBufferData.buffer;
            robotData.size = teamPointer->agentData[robotIndex].matchArray[0].ncdBufferData.bufferSize;

            for (int i = 0; i < numArchivedBehaviors; i++)
            {
                switch(robotTeam)
                {
                case REDTEAM:
                    comparisonData.data = redTeamNoveltyArchive[i].data;
                    comparisonData.size = redTeamNoveltyArchive[i].size;
                    archiveDataVector.push_back(comparisonData);
                    break;

                case BLUETEAM:
                    comparisonData.data = blueTeamNoveltyArchive[i].data;
                    comparisonData.size = blueTeamNoveltyArchive[i].size;
                    archiveDataVector.push_back(comparisonData);
                    break;
                }
            }

            ncdValue        += ncdCalculator.GetNonAveragedNcdValue(robotData, archiveDataVector);
            ncdValueCounter += archiveDataVector.size();
        }
    }


    // Average the ncd value...
    ncdValue /= (double)ncdValueCounter;

    //... and return it to the experiment code
    return ncdValue;
}






// Returns the size of the teams archive. Useful
// for planning multi-threaded evaluations
unsigned int getArchiveSize(hostTeamType team)
{
    switch (team)
    {
    case REDTEAM:
        if (UseStaticStimNovelty)
        {
            return redTeamNoveltyArchiveStaticStim.size();
        }
        else
        {
            return redTeamNoveltyArchive.size();
        }
        break;

    case BLUETEAM:
        if (UseStaticStimNovelty)
        {
            return blueTeamNoveltyArchiveStaticStim.size();
        }
        else
        {
            return blueTeamNoveltyArchive.size();
        }
        break;
    }
}





// Stores a robot into the behavior archive
void addRobotToArchive(hostTeamType team, int genomeID)
{
    if (UseStaticStimNovelty)
    {
        noveltyArchiveStaticStimRecord   newStaticStimRecord;
        teamAgentStaticStimBehaviorData* teamStaticStimPointer;
        bool foundGenome = false;

        switch (team)
        {
        case REDTEAM:
            teamStaticStimPointer = &redTeamStaticStimBehavData;
            break;

        case BLUETEAM:
            teamStaticStimPointer = &blueTeamStaticStimBehavData;
            break;
        }

        for (unsigned int i = 0; i < param_numAgents; i++)
        {
            if (teamStaticStimPointer->teamMemberBehaviorData[i].genomeID == genomeID)
            {
                // Found the right robot, create the new static stimulus archive record
                foundGenome = true;

                newStaticStimRecord.genomeID = genomeID;

                newStaticStimRecord.firstBufferSize  = teamStaticStimPointer->teamMemberBehaviorData[i].firstBufferSize;
                newStaticStimRecord.secondBufferSize = teamStaticStimPointer->teamMemberBehaviorData[i].secondBufferSize;

                newStaticStimRecord.firstBuffer  = new char[newStaticStimRecord.firstBufferSize]();
                newStaticStimRecord.secondBuffer = new char[newStaticStimRecord.secondBufferSize]();

                // Copy the buffer contents
                memcpy(newStaticStimRecord.firstBuffer, teamStaticStimPointer->teamMemberBehaviorData[i].firstBuffer,
                       teamStaticStimPointer->teamMemberBehaviorData[i].firstBufferSize);

                memcpy(newStaticStimRecord.secondBuffer, teamStaticStimPointer->teamMemberBehaviorData[i].secondBuffer,
                       teamStaticStimPointer->teamMemberBehaviorData[i].secondBufferSize);


                //                // Test BS code
                //                NcdDataObject originalBufferData;
                //                NcdDataObject copiedBufferData;

                //                originalBufferData.size = newStaticStimRecord.firstBufferSize;
                //                copiedBufferData.size   = newStaticStimRecord.firstBufferSize;

                //                originalBufferData.data = teamStaticStimPointer->teamMemberBehaviorData[i].firstBuffer;
                //                copiedBufferData.data   = newStaticStimRecord.firstBuffer;

                //                NcdZlibCalc ncdCalculator;
                //                double ncdValue;

                //                ncdValue = ncdCalculator.GetNcdValue(originalBufferData, copiedBufferData);

                //                cout<<"~~~ Test Archive buffer copy, ncd value of first buffer copy: "<<ncdValue<<endl;
                //                cout<<"~ copiedBufferSize: "<<copiedBufferData.size<<endl;

                //                originalBufferData.size = newStaticStimRecord.secondBufferSize;
                //                copiedBufferData.size   = newStaticStimRecord.secondBufferSize;

                //                originalBufferData.data = teamStaticStimPointer->teamMemberBehaviorData[i].secondBuffer;
                //                copiedBufferData.data   = newStaticStimRecord.secondBuffer;

                //                ncdValue = ncdCalculator.GetNcdValue(originalBufferData, copiedBufferData);

                //                cout<<"~~~ Test Archive buffer copy, ncd value of second buffer copy: "<<ncdValue<<endl;
                //                cout<<"~ copiedBufferSize: "<<copiedBufferData.size<<endl;
                //                // end test BS code




                switch (team)
                {
                case REDTEAM:
                    redTeamNoveltyArchiveStaticStim.push_back(newStaticStimRecord);
                    break;

                case BLUETEAM:
                    blueTeamNoveltyArchiveStaticStim.push_back(newStaticStimRecord);
                    break;
                }

                // Done with the search, get outta here
                break;
            }
        }

        if (!foundGenome)
        {
            cout<<"Error in addRobotToArchive in static stim mode, didn't find the genomeID!"<<endl;
            exit(1);
        }
    }
    else
    {
        noveltyArchiveRecord newRecord;

        newRecord.genomeID = genomeID;
        newRecord.size     = 0;

        switch (team)
        {
        case REDTEAM:
            redTeamNoveltyArchive.push_back(newRecord);
            break;

        case BLUETEAM:
            blueTeamNoveltyArchive.push_back(newRecord);
            break;
        }
    }
}





// Set the constant stimulus values
// to use in novelty search when using
// static stimulation, instead of dynamic
// match behavioral data
void setupStaticStimulusVector()
{
    brainInputData stimulusData;
    const int numSamplesPerPrimeSensor = 5;
    const int numSensorsPasses         = 5;
    const int numEnergySamples         = 2;

    // These are the static values we'll use to
    // stimulate the distance sensors for food
    // and the enemy
    double distSensorValues[5]    = {0.2, 0.4, 0.6, 0.8, 1.0};
    double energySensorValues[2]  = {-0.2, 0.2};
    double foodSensorValueToUse   = 0.0;
    double enemySensorValueToUse  = 0.0;

    staticStimulusVector.clear();

    for (int h = 0; h < numSamplesPerPrimeSensor; h++)
    {
        foodSensorValueToUse = distSensorValues[h];

        for (int i = 0; i < numSensorsPasses; i++)
        {
            stimulusData.foodSensorForward   = 0.0;
            stimulusData.foodSensorLeft      = 0.0;
            stimulusData.foodSensorRight     = 0.0;
            stimulusData.foodSensorBackLeft  = 0.0;
            stimulusData.foodSensorBackRight = 0.0;

            switch (i)
            {
            case 0:
                stimulusData.foodSensorForward = foodSensorValueToUse;
                break;
            case 1:
                stimulusData.foodSensorLeft = foodSensorValueToUse;
                break;
            case 2:
                stimulusData.foodSensorRight = foodSensorValueToUse;
                break;
            case 3:
                stimulusData.foodSensorBackLeft = foodSensorValueToUse;
                break;
            case 4:
                stimulusData.foodSensorBackRight = foodSensorValueToUse;
                break;
            default:
                cout<<"In setupStaticStimulusVector, invalid i: "<<i<<endl;
                exit(1);
            }

            for (int j = 0; j < numSamplesPerPrimeSensor; j++)
            {
                enemySensorValueToUse = distSensorValues[j];

                for (int k = 0; k < numSensorsPasses; k++)
                {
                    stimulusData.enemySensorForward   = 0.0;
                    stimulusData.enemySensorLeft      = 0.0;
                    stimulusData.enemySensorRight     = 0.0;
                    stimulusData.enemySensorBackLeft  = 0.0;
                    stimulusData.enemySensorBackRight = 0.0;

                    switch (k)
                    {
                    case 0:
                        stimulusData.enemySensorForward = enemySensorValueToUse;
                        break;
                    case 1:
                        stimulusData.enemySensorLeft = enemySensorValueToUse;
                        break;
                    case 2:
                        stimulusData.enemySensorRight = enemySensorValueToUse;
                        break;
                    case 3:
                        stimulusData.enemySensorBackLeft = enemySensorValueToUse;
                        break;
                    case 4:
                        stimulusData.enemySensorBackRight = enemySensorValueToUse;
                        break;
                    default:
                        cout<<"In setupStaticStimulusVector, invalid k: "<<k<<endl;
                        exit(1);
                    }

                    for (int l = 0; l < numEnergySamples; l++)
                    {
                        stimulusData.wallSensor             = 0.0;
                        stimulusData.biasInput              = 1.0;
                        stimulusData.energyDifferenceSensor = energySensorValues[l];

                        // Push onto vector
                        staticStimulusVector.push_back(stimulusData);
                    }
                }
            }
        }
    }

    if (expectedNumStaticStims != staticStimulusVector.size())
    {
        cout<<"Error in setupStaticStimulusVector, size of static stimulus vector not what expected."<<endl;
        exit(1);
    }
}








// Returns a vector of neural network input
// values to use in static stimulation mode of
// novelty search
vector <brainInputData> getStaticStimulusVector()
{
    if (!staticStimulusVectorCreated)
    {
        setupStaticStimulusVector();
        staticStimulusVectorCreated = true;
    }

    return staticStimulusVector;
}




// Test BS function
void testFunc()
{
    cout<<"Red team first buffer size dump:"<<endl;
    for (int i = 0; i < param_numAgents; i++)
    {
        cout<<"i: "<<i<<", bufferSize: "<<redTeamStaticStimBehavData.teamMemberBehaviorData[i].firstBufferSize<<endl;
    }

    return;
    cout<<"TEST!!!! Checking ncd values between same agent matches..."<<endl;
    NcdZlibCalc   ncdCalculator;
    NcdDataObject robotFirstData;
    NcdDataObject robotSecondData;

    vector <NcdDataObject> robotDataVec;

    robotFirstData.data = redTeamBehaviorData.agentData[0].matchArray[0].ncdBufferData.buffer;
    robotFirstData.size = redTeamBehaviorData.agentData[0].matchArray[0].ncdBufferData.bufferSize;

    robotSecondData.data = redTeamBehaviorData.agentData[0].matchArray[0].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[0].matchArray[0].ncdBufferData.bufferSize;

    double singleMatchNcdScore = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Comparing single match of same agent, same opponent, ncd is: "<<singleMatchNcdScore<<endl;

    robotSecondData.data = redTeamBehaviorData.agentData[0].matchArray[1].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[0].matchArray[1].ncdBufferData.bufferSize;

    double leftRightNcdScore = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Comparing left/right match of same agent, same opponent, ncd is: "<<leftRightNcdScore<<endl;


    robotSecondData.data = redTeamBehaviorData.agentData[1].matchArray[0].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[1].matchArray[0].ncdBufferData.bufferSize;
    double leftSingleMatch = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Robots 0 & 1, first match ncd: "<<leftSingleMatch<<endl;

    robotSecondData.data = redTeamBehaviorData.agentData[2].matchArray[0].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[2].matchArray[0].ncdBufferData.bufferSize;
    leftSingleMatch = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Robots 0 & 2, first match ncd: "<<leftSingleMatch<<endl;


    robotSecondData.data = redTeamBehaviorData.agentData[3].matchArray[0].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[3].matchArray[0].ncdBufferData.bufferSize;
    leftSingleMatch = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Robots 0 & 3, first match ncd: "<<leftSingleMatch<<endl;

    robotSecondData.data = redTeamBehaviorData.agentData[4].matchArray[0].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[4].matchArray[0].ncdBufferData.bufferSize;
    leftSingleMatch = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Robots 0 & 4, first match ncd: "<<leftSingleMatch<<endl;


    //    double wholeRobotComparison = getInterteamNcdValue(REDTEAM, 0, 0);
    //    cout<<"Whole robot comparison for robot 0 and itself: "<<wholeRobotComparison<<endl;

    //    wholeRobotComparison = getInterteamNcdValue(REDTEAM, 0, 1);
    //    cout<<"Whole robot comparison for robot 0 and 1: "<<wholeRobotComparison<<endl;

    //    wholeRobotComparison = getInterteamNcdValue(REDTEAM, 1, 1);
    //    cout<<"Whole robot comparison for robot 1 and itself: "<<wholeRobotComparison<<endl;


    robotFirstData.data = redTeamBehaviorData.agentData[0].matchArray[0].ncdBufferData.buffer;
    robotFirstData.size = redTeamBehaviorData.agentData[0].matchArray[0].ncdBufferData.bufferSize;

    robotSecondData.data = redTeamBehaviorData.agentData[0].matchArray[2].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[0].matchArray[2].ncdBufferData.bufferSize;
    double leftNcdScore = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Robot 0, left matches from two difference opponents ncd is: "<<leftNcdScore<<endl;

    robotSecondData.data = redTeamBehaviorData.agentData[0].matchArray[4].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[0].matchArray[4].ncdBufferData.bufferSize;
    leftNcdScore = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Robot 0, left matches from two difference opponents ncd is: "<<leftNcdScore<<endl;

    robotSecondData.data = redTeamBehaviorData.agentData[0].matchArray[6].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[0].matchArray[6].ncdBufferData.bufferSize;
    leftNcdScore = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Robot 0, left matches from two difference opponents ncd is: "<<leftNcdScore<<endl;

    robotSecondData.data = redTeamBehaviorData.agentData[0].matchArray[8].ncdBufferData.buffer;
    robotSecondData.size = redTeamBehaviorData.agentData[0].matchArray[8].ncdBufferData.bufferSize;
    leftNcdScore = ncdCalculator.GetNcdValue(robotFirstData, robotSecondData);
    cout<<"Robot 0, left matches from two difference opponents ncd is: "<<leftNcdScore<<endl;
}

