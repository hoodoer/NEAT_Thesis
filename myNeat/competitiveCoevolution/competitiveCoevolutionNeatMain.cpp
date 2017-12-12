// This program is for development and testing of my own implementation 
// of competitive coevolution NEAT experiments in Linux.
// This file contains the actual experiment itself,
// and the code is pretty much a disaster. Sorry 'bout that.
// Drew Kirkpatrick, drew.kirkpatrick@gmail.com

#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>
#include <signal.h>
#include <fstream>

#include <pthread.h>
#include "time.h"
#include <signal.h>

#include <QDataStream>
#include <QFile>
#include <QString>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>


#include "networkserver.h"


using namespace std;

#include "SVector2D.h"
#include "timer.h"
#include "mathVector.h"
#include "mathMatrix.h"

#include "utils.h"
#include "Cga.h"
#include "robotAgent.h"
#include "gameworld.h"
#include "parameters.h"

#include "competitiveCoevolutionNeat.h"
#include "sharedMemory.h"
#include "ncdBehaviorFuncs.h"






/**********************************************************************
               General global types and variables
*********************************************************************/

bool DONE                      = false;
bool THREAD_READY              = false;
bool EXECUTION_THREADS_ONLINE  = false;

unsigned int generationCounter = 1;



// For testing only. If you're using this
// for testing, make sure you're only using
// a single execution thread, so everything is
// kept sane. This also will generation a lot
// of very large .vcr files
// See the recordRenderingDataForDomTourny
// function
bool recordDomTournyRenderingData = false;






// Structure that contains
// generation data saved
// to a .csv file when data
// recording is toggled on
struct generationRecordData
{
    int          generation;
    bool         noveltySearchBool; // new
    bool         minimumCriteriaBool; // new
    bool         staticStimulusBool; // new
    bool         staticStimExpandedComparisonBool; // new
    bool         brainOutputOnlyBool; // new
    bool         randomSearchBool; // new
    bool         playoffRequiredBool; // new
    int          numBlueTeamInPlayoff; // new
    int          numRedTeamInPlayoff; // new
    int          numBlueTeamPassedMinCriteria; // new
    int          numRedTeamPassedMinCriteria; // new
    int          blueTeamChampScore;
    int          blueTeamChampGenomeID;
    int          blueTeamChampNumNeurons;
    int          blueTeamChampNumConnections;
    int          blueTeamChampNumHiddenNodes;
    int          blueTeamMeanNumNeurons;
    int          blueTeamMeanNumConnections;
    int          blueTeamMeanNumHiddenNodes;
    int          redTeamChampScore;
    int          redTeamChampGenomeID;
    int          redTeamChampNumNeurons;
    int          redTeamChampNumConnections;
    int          redTeamChampNumHiddenNodes;
    int          redTeamMeanNumNeurons;
    int          redTeamMeanNumConnections;
    int          redTeamMeanNumHiddenNodes;
    int          blueTeamNumSpecies;
    int          redTeamNumSpecies;
    int          blueChampGenTournyScore;
    int          redChampGenTournyScore;
    bool         wasGenTournyTied;
    hostTeamType genChampTeam;
    int          numBlueTeamGenChamps; // new
    int          numRedTeamGenChamps; // new
    int          numBlueTeamDomStrats; // new
    int          numRedTeamDomStrats; // new
    bool         wasDomStratTournySkipped; // new
    bool         wasThereNewDominantStrat;
    hostTeamType newDominantStratFromTeam;
    int          whichDominantStratWon;
    int          winningDominantStratScore;
    int          losingGenChampChallengerScore;
    int          numDominantStrategies;
    int          numGensSinceNewDominantStrat; // new
    int          bestDominantStratGenomeID;
    int          bestDominantStratNumNeurons;
    int          bestDominantStratNumConnections;
    int          bestDominantStratNumHiddenNodes;
    float        standardEvalTime;
    float        playoffEvalTime; // new
    float        genChampEvalTime;
    float        dominanceTournyEvalTime;
    float        generationTime;
    float        totalTime;
};





// Which execution thread should drive the
// rendering, and which (right or left)
// evaluation should provide the data?
int RENDER_THREAD_NUM         = 0;
hostStartPosition RENDER_SIDE = hostStartLeft;


// Mode data
//renderType   renderMode = normal;
renderType   renderMode = renderOff;

//timeType     timeMode   = realtime;
timeType     timeMode   = accelerated;

bool         PAUSE      = false;
hostTeamType hostTeam   = BLUETEAM;




// Data for rendering
agentPositionData   renderHostAgent;
agentPositionData   renderParasiteAgent;
singlePieceFoodData renderFoodData[MAX_FOOD_PIECES];
brainInputData      hostSensorData;
brainInputData      parasiteSensorData;
unsigned int        numFoodPieces        = 0;
unsigned int        hostEnergy           = 0;
unsigned int        parasiteEnergy       = 0;
unsigned int        hostNumFoodEaten     = 0;
unsigned int        parasiteNumFoodEaten = 0;




const int nanoSleepAmount       = 1000;       // millionth of a second
const int longSleepAmount       = 100000000;  // tenth of a second
const int reallyLongSleepAmount = 1000000000; // 1 second







/**********************************************************************
             QT Stuff to support the network server features
             Note: Network client GUI is abandonware right
             now, this server code isn't actually being
             used in the current implementation.
*********************************************************************/
QCoreApplication* qtConsoleAppPtr;
NetworkServer    *netServer;











/**********************************************************************
               AI/Genetic specific types and variables
*********************************************************************/
// Vector of agents for the teams
// Vector of vectors so that
// each execution thread has it's own
// copy to work with
vector < vector<RobotAgent*> > redTeamRobots;
vector < vector<RobotAgent*> > blueTeamRobots;


// Best of all time.
// Contains all generation champions.
// Opponents will be chosen at random
// From these robots.
// Vector of vectors, so that each
// execution thread has it's own copy of a
// neural network
vector < vector<RobotAgent*> > redTeamHallOfFameRobots;
vector < vector<RobotAgent*> > blueTeamHallOfFameRobots;

// Champion from each of the best 4 species
// Each execution thread will have it's own
// to work with, thus the vector of vectors
vector < vector<RobotAgent*> > redTeamBestRobots;
vector < vector<RobotAgent*> > blueTeamBestRobots;


// Robots that are in the novelty archive
vector < vector<RobotAgent*> > redTeamNoveltyArchiveRobots;
vector < vector<RobotAgent*> > blueTeamNoveltyArchiveRobots;












/**********************************************************************
             Data collection and Novelty Search
                variables and functions
*********************************************************************/
string   dataFileName;
bool     recordData       = false;

// Novelty search related switches/modes
bool UseNoveltySearch     = false;

// Novelty search with Minimal Criteria
bool UseMinimalCriteria   = false;

// Base Novelty of agents on the same brain input data,
// not dynamic matches
// If StaticStimExpandedComparisons is true,
// the static stimulus mode will compare
// behavior against all teammates
bool UseStaticStimNovelty          = false;
bool StaticStimExpandedComparisons = false;

// Use normal Novelty search behavior of dynamic matches,
// but only compare brain outputs
bool UseBrainOutputOnly   = false;

// User random evolution, strictly for comparison purposes
// This will be a very poor method of neuroevolution. All
// genomes will be assigned the exact same fitness score
bool UseRandomEvolution = false;


ofstream dataFileStream;
const string dataFileExtension = ".csv";




// This function opens the data file for recording, and
// prints out the header labels
void openDataRecordFile()
{
    string fullDataFileName = dataFileName + dataFileExtension;
    dataFileStream.open(fullDataFileName.c_str(), ios::out);

    if (dataFileStream.fail())
    {
        cout<<"Error opening data file: "<<fullDataFileName<<" for recording!"<<endl;
        evaluationMode = stopEvals;
        DONE = true;
        exit(1);
    }

    dataFileStream << setiosflags(ios::fixed);
    dataFileStream << "Generation"                  << ", "
                   << "NoveltySearchUsed"           << ", "
                   << "MinimumCriteriaSearchUsed"   << ", "
                   << "StaticStimulusUsed"          << ", "
                   << "StatSimExpandedCompsUsed"    << ", "
                   << "BrainOutputsOnlyUsed"        << ", "
                   << "RandomSearchUsed"            << ", "
                   << "PlayoffRequired"             << ", "
                   << "NumBlueTeamInPlayoff"        << ", "
                   << "NumRedTeamInPlayoff"         << ", "
                   << "NumBlueTeamPassMinCriteria"  << ", "
                   << "NumRedTeamPassMinCriteria"   << ", "
                   << "BlueTeamChampScore"          << ", "
                   << "BlueTeamChampGenomeID"       << ", "
                   << "BlueTeamChampNumNeurons"     << ", "
                   << "BlueTeamChampNumConnections" << ", "
                   << "BlueTeamChampNumHiddenNodes" << ", "
                   << "BlueTeamMeanNumNeurons"      << ", "
                   << "BlueTeamMeanNumConnections"  << ", "
                   << "BlueTeamMeanNumHiddenNodes"  << ", "
                   << "RedTeamChampScore"           << ", "
                   << "RedTeamChampGenomeID"        << ", "
                   << "RedTeamChampNumNeurons"      << ", "
                   << "RedTeamChampNumConnections"  << ", "
                   << "RedTeamChampNumHiddenNodes"  << ", "
                   << "RedTeamMeanNumNeurons"       << ", "
                   << "RedTeamMeanNumConnections"   << ", "
                   << "RedTeamMeanNumHiddenNodes"   << ", "
                   << "BlueTeamNumSpeciesPostEpoch" << ", "
                   << "RedTeamNumSpeciesPostEpoch"  << ", "
                   << "BlueChampGenTournyScore"     << ", "
                   << "RedChampGenTournyScore"      << ", "
                   << "WasGenerationTournyTied"     << ", "
                   << "GenChampTeam"                << ", "
                   << "NumBlueTeamGenChamps"        << ", "
                   << "NumRedTeamGenChamps"         << ", "
                   << "NumBlueTeamDominantStrats"   << ", "
                   << "NumRedTeamDominantStrats"    << ", "
                   << "WasDomStratTournySkipped"    << ", "
                   << "NewDominantStrategy"         << ", "
                   << "NewDominantStrategyFromTeam" << ", "
                   << "WhichDominantStrategyWon"    << ", "
                   << "DominantStrategyScore"       << ", "
                   << "GenChampChallengerScore"     << ", "
                   << "NumDominantStrategies"       << ", "
                   << "NumGensSinceNewDomStrat"     << ", "
                   << "DominantStratGenomeID"       << ", "
                   << "DominantStratNumNeurons"     << ", "
                   << "DominantStratNumConnections" << ", "
                   << "DominantStratNumHiddenNodes" << ", "
                   << "StandardEvalCompTime"        << ", "
                   << "PlayoffEvalCompTime"         << ", "
                   << "GenerationChampCompTime"     << ", "
                   << "DominanceTournyCompTime"     << ", "
                   << "EntireGenerationCompTime"    << ", "
                   << "TotalCompTimeSoFar"          << ", "
                   << endl;
}





// This function simply closes the data file
void closeDataRecordFile()
{
    dataFileStream.close();
}



// This function is called to record
// data about a generation.
void recordGenerationData(generationRecordData data)
{
    dataFileStream << data.generation                        << ", ";

    if (data.noveltySearchBool)
    {
        dataFileStream << "NOVELTY_SEARCH"                   << ", ";
    }
    else
    {
        dataFileStream << "OBJECTIVE_SEARCH"                 << ", ";
    }

    if (data.minimumCriteriaBool)
    {
        dataFileStream << "MINIMUM_CRITERIA"                 << ", ";
    }
    else
    {
        dataFileStream << "NO"                               << ", ";
    }

    if (data.staticStimulusBool)
    {
        dataFileStream << "STATIC_STIMULUS_NOVELTY"          << ", ";
    }
    else
    {
        dataFileStream << "NO"                               << ", ";
    }

    if (data.staticStimExpandedComparisonBool)
    {
        dataFileStream << "STATSTIM_EXPANDED_COMPARISONS"    << ", ";
    }
    else
    {
        dataFileStream << "NO"                               << ", ";
    }

    if (data.brainOutputOnlyBool)
    {
        dataFileStream << "BRAIN_OUTPUT_ONLY"                << ", ";
    }
    else
    {
        dataFileStream << "NO"                               << ", ";
    }

    if (data.randomSearchBool)
    {
        dataFileStream << "RANDOM_SEARCH"                    << ", ";
    }
    else
    {
        dataFileStream << "NO"                               << ", ";
    }

    if (data.playoffRequiredBool)
    {
        dataFileStream << "PLAYOFF_REQUIRED"                 << ", ";
    }
    else
    {
        dataFileStream << "NO"                               << ", ";
    }

    if (data.playoffRequiredBool)
    {
        dataFileStream << data.numBlueTeamInPlayoff          << ", ";
        dataFileStream << data.numRedTeamInPlayoff           << ", ";
    }
    else
    {
        dataFileStream << "0"                                << ", ";
        dataFileStream << "0"                                << ", ";
    }

    if (data.minimumCriteriaBool)
    {
        dataFileStream << data.numBlueTeamPassedMinCriteria  << ", ";
        dataFileStream << data.numRedTeamPassedMinCriteria   << ", ";
    }
    else
    {
        dataFileStream << "0"                                << ", ";
        dataFileStream << "0"                                << ", ";
    }

    dataFileStream << data.blueTeamChampScore                << ", ";
    dataFileStream << data.blueTeamChampGenomeID             << ", ";
    dataFileStream << data.blueTeamChampNumNeurons           << ", ";
    dataFileStream << data.blueTeamChampNumConnections       << ", ";
    dataFileStream << data.blueTeamChampNumHiddenNodes       << ", ";

    dataFileStream << data.blueTeamMeanNumNeurons            << ", ";
    dataFileStream << data.blueTeamMeanNumConnections        << ", ";
    dataFileStream << data.blueTeamMeanNumHiddenNodes        << ", ";

    dataFileStream << data.redTeamChampScore                 << ", ";
    dataFileStream << data.redTeamChampGenomeID              << ", ";
    dataFileStream << data.redTeamChampNumNeurons            << ", ";
    dataFileStream << data.redTeamChampNumConnections        << ", ";
    dataFileStream << data.redTeamChampNumHiddenNodes        << ", ";

    dataFileStream << data.redTeamMeanNumNeurons             << ", ";
    dataFileStream << data.redTeamMeanNumConnections         << ", ";
    dataFileStream << data.redTeamMeanNumHiddenNodes         << ", ";

    dataFileStream << data.blueTeamNumSpecies                << ", ";
    dataFileStream << data.redTeamNumSpecies                 << ", ";

    dataFileStream << data.blueChampGenTournyScore           << ", ";
    dataFileStream << data.redChampGenTournyScore            << ", ";

    if (data.wasGenTournyTied)
    {
        dataFileStream << "TIED"                             << ", ";
    }
    else
    {
        dataFileStream << "NotTied"                          << ", ";
    }

    switch (data.genChampTeam)
    {
    case REDTEAM:
        dataFileStream << "RED"                              << ", ";
        break;

    case BLUETEAM:
        dataFileStream << "BLUE"                             << ", ";
        break;
    }

    dataFileStream << data.numBlueTeamGenChamps              << ", ";
    dataFileStream << data.numRedTeamGenChamps               << ", ";

    dataFileStream << data.numBlueTeamDomStrats              << ", ";
    dataFileStream << data.numRedTeamDomStrats               << ", ";

    if (data.wasDomStratTournySkipped)
    {
        dataFileStream << "DOMSTRAT_TOURNY_SKIPPED"          << ", ";
    }
    else
    {
        dataFileStream << "DOMSTRAT_TOURNY_RUN"              << ", ";
    }

    if (data.wasThereNewDominantStrat)
    {
        dataFileStream << "YES"                              << ", ";
    }
    else
    {
        dataFileStream << "NO"                               << ", ";
    }

    if (data.wasThereNewDominantStrat)
    {
        switch (data.newDominantStratFromTeam)
        {
        case REDTEAM:
            dataFileStream << "RED"                          << ", ";
            break;

        case BLUETEAM:
            dataFileStream << "BLUE"                         << ", ";
            break;
        }
    }
    else
    {
        dataFileStream << "NoNewDominantStrat"               << ", ";
    }

    if (data.wasThereNewDominantStrat)
    {
        dataFileStream << "NotApplicable"                    << ", ";
        dataFileStream << "NotApplicable"                    << ", ";
        dataFileStream << "NotApplicable"                    << ", ";
    }
    else
    {
        dataFileStream << data.whichDominantStratWon         << ", ";
        dataFileStream << data.winningDominantStratScore     << ", ";
        dataFileStream << data.losingGenChampChallengerScore << ", ";
    }

    dataFileStream << data.numDominantStrategies             << ", ";
    dataFileStream << data.numGensSinceNewDominantStrat      << ", ";

    dataFileStream << data.bestDominantStratGenomeID         << ", ";
    dataFileStream << data.bestDominantStratNumNeurons       << ", ";
    dataFileStream << data.bestDominantStratNumConnections   << ", ";
    dataFileStream << data.bestDominantStratNumHiddenNodes   << ", ";

    dataFileStream << data.standardEvalTime                  << ", ";
    dataFileStream << data.playoffEvalTime                   << ", ";
    dataFileStream << data.genChampEvalTime                  << ", ";
    dataFileStream << data.dominanceTournyEvalTime           << ", ";
    dataFileStream << data.generationTime                    << ", ";
    dataFileStream << data.totalTime                         << ", ";

    dataFileStream << endl;
}





// This function is used for testing purposes, it records the data
// needed for rendering a dominance tournament later in a visualization
// program. It's called each tick of the match being recorded.
// If you're using this for recording the rendering data for a
// dominance tournament, make sure only 1 execution thread is being used.
// Use QT to write out binary files, as the files are too big as standard
// .csv files
void recordRenderingDataForDomTourny(domTournyRenderingDataType renderDataToSave)
{
    if (!recordDomTournyRenderingData)
    {
        cout<<"Error in recordRenderingDataForDomTourny, func called when not supposed to be recording rendering data"<<endl;
        DONE = true;
        exit(1);
    }

    // Counters so that we can tell if we need
    // to start a new redering data file
    static int lastGeneration     = -1;
    static int lastDomStratNumber = -1;


    static bool         renderDataStreamOpen = false;


//    static QString qRenderDataFilePrefix    = "./dataFiles/renderDataDomTourney_";
    static QString qRenderDataFilePrefix    = "/atlas/tmp/drewThesisData/renderDataDomTourney_";
    static QString qRenderDataFileExtension = ".vcr";
    static QString qRenderDataFileName;

    static QFile qRenderDataFile;
    static QDataStream qRenderDataFileStream;



    // Do we need to start a new file?
    if ((renderDataToSave.renderGeneration != lastGeneration) ||
        (renderDataToSave.renderDominantStratNumber != lastDomStratNumber))
    {
        // If either of these values have changed, we need to start a new
        // data file.

        if (renderDataStreamOpen)
        {
           qRenderDataFile.close();
           renderDataStreamOpen = false;
        }

        qRenderDataFileName = qRenderDataFilePrefix;
        qRenderDataFileName.append(QString(QObject::tr("gen%1_domStrat%2"))
                                   .arg(renderDataToSave.renderGeneration)
                                   .arg(renderDataToSave.renderDominantStratNumber));
        qRenderDataFileName.append(qRenderDataFileExtension);

        qRenderDataFile.setFileName(qRenderDataFileName);
        qRenderDataFile.open(QIODevice::WriteOnly);

        qRenderDataFileStream.setDevice(&qRenderDataFile);
//        qRenderDataFileStream.setVersion(QDataStream::Qt_4_6);
        qRenderDataFileStream.setVersion(QDataStream::Qt_4_4);

        renderDataStreamOpen = true;

        lastGeneration     = renderDataToSave.renderGeneration;
        lastDomStratNumber = renderDataToSave.renderDominantStratNumber;
    }



    qRenderDataFileStream << (quint16)renderDataToSave.renderGeneration
                          << (quint16)renderDataToSave.renderTick
                          << (quint32)renderDataToSave.renderGenChampGenomeID
                          << (quint32)renderDataToSave.renderDominantStratGenomeID
                          << (quint8)renderDataToSave.renderDominantStratNumber
                          << (quint8)renderDataToSave.renderNumFoodPieces
                          << (quint8)renderDataToSave.renderGenChampNumFoodEaten
                          << (quint8)renderDataToSave.renderDominantStratNumFoodEaten
                          << (quint16)renderDataToSave.renderGenChampEnergy
                          << (quint16)renderDataToSave.renderDominantStratEnergy
                          << (quint8)renderDataToSave.renderGenChampScore
                          << (quint8)renderDataToSave.renderDominantStratScore
                          << (float)renderDataToSave.renderGenomeChampPosData.agentHdg
                          << (float)renderDataToSave.renderGenomeChampPosData.agentPos.x
                          << (float)renderDataToSave.renderGenomeChampPosData.agentPos.y
                          << (float)renderDataToSave.renderDominantStratPosData.agentHdg
                          << (float)renderDataToSave.renderDominantStratPosData.agentPos.x
                          << (float)renderDataToSave.renderDominantStratPosData.agentPos.y
                          << (float)renderDataToSave.renderGenChampSensorData.foodSensorForward
                          << (float)renderDataToSave.renderGenChampSensorData.foodSensorLeft
                          << (float)renderDataToSave.renderGenChampSensorData.foodSensorRight
                          << (float)renderDataToSave.renderGenChampSensorData.foodSensorBackLeft
                          << (float)renderDataToSave.renderGenChampSensorData.foodSensorBackRight
                          << (float)renderDataToSave.renderGenChampSensorData.enemySensorForward
                          << (float)renderDataToSave.renderGenChampSensorData.enemySensorLeft
                          << (float)renderDataToSave.renderGenChampSensorData.enemySensorRight
                          << (float)renderDataToSave.renderGenChampSensorData.enemySensorBackLeft
                          << (float)renderDataToSave.renderGenChampSensorData.enemySensorBackRight
                          << (float)renderDataToSave.renderGenChampSensorData.wallSensor
                          << (float)renderDataToSave.renderGenChampSensorData.energyDifferenceSensor
                          << (float)renderDataToSave.renderDominantStratSensorData.foodSensorForward
                          << (float)renderDataToSave.renderDominantStratSensorData.foodSensorLeft
                          << (float)renderDataToSave.renderDominantStratSensorData.foodSensorRight
                          << (float)renderDataToSave.renderDominantStratSensorData.foodSensorBackLeft
                          << (float)renderDataToSave.renderDominantStratSensorData.foodSensorBackRight
                          << (float)renderDataToSave.renderDominantStratSensorData.enemySensorForward
                          << (float)renderDataToSave.renderDominantStratSensorData.enemySensorLeft
                          << (float)renderDataToSave.renderDominantStratSensorData.enemySensorRight
                          << (float)renderDataToSave.renderDominantStratSensorData.enemySensorBackLeft
                          << (float)renderDataToSave.renderDominantStratSensorData.enemySensorBackRight
                          << (float)renderDataToSave.renderDominantStratSensorData.wallSensor
                          << (float)renderDataToSave.renderDominantStratSensorData.energyDifferenceSensor;


    // now write out all the food data that are relevant currently
    for (int i = 0; i < renderDataToSave.renderNumFoodPieces; i++)
    {
        qRenderDataFileStream << (float)renderDataToSave.renderFoodData[i].foodPosition.x
                              << (float)renderDataToSave.renderFoodData[i].foodPosition.y
                              << renderDataToSave.renderFoodData[i].foodEaten;
    }
}










/**********************************************************************
             AI and Evaluation functions
*********************************************************************/
// This function pushes all of the planned standard evaluations into a
// queue to be pulled by the various execution threads
// to execute
void setupStandardEvaluationJobs()
{
    // Now setup the regular evaluation queue stuff...
    evaluationMode = standardEvaluation;

    evaluationJobData evalJob;

    for (int i = 0; i < param_numAgents; i++)
    {
        // Add a job for the blue team member
        evalJob.hostRobotIndex = i;
        evalJob.hostTeam       = BLUETEAM;

        jobManagerAddJobToQueue(evalJob);

        // And the red team member, same index number
        evalJob.hostTeam = REDTEAM;
        jobManagerAddJobToQueue(evalJob);
    }
}






// This function pushes all the needed playoff matches into a queue
// to be pulled by the various execution threads to run. The planned
// matches follow the same tournament style of the dominance tournament,
// Except we'll only use the single extra food item adjustment, so playoff
// tournaments don't take too long.
// Returns the number of jobs that were scheduled, so
// the job manager knows how many results to look for
int setupPlayoffTournamentJobs(const vector<playoffMemberReferenceData> redTeamPlayers,
                                const vector<playoffMemberReferenceData> blueTeamPlayers)
{
    evaluationMode = playoffTournament;
    playoffJobData playoffJob;
    int jobCounter = 0;

    // Loop through all the competitors so that each plays
    // all competitors from the other team
    for (unsigned int redTeamCounter = 0; redTeamCounter < redTeamPlayers.size(); redTeamCounter++)
    {
        for (unsigned int blueTeamCounter = 0; blueTeamCounter < blueTeamPlayers.size(); blueTeamCounter++)
        {
            // Add one job with the standard food layout
            playoffJob.blueTeamContestantRobotIndex = blueTeamPlayers.at(blueTeamCounter).robotIndex;
            playoffJob.blueTeamContestantGenomeId   = blueTeamPlayers.at(blueTeamCounter).robotGenomeId;
            playoffJob.blueTeamContestantDataIndex  = blueTeamCounter;

            playoffJob.redTeamContestantRobotIndex  = redTeamPlayers.at(redTeamCounter).robotIndex;
            playoffJob.redTeamContestantGenomeId    = redTeamPlayers.at(redTeamCounter).robotGenomeId;
            playoffJob.redTeamContestantDataIndex   = redTeamCounter;

            playoffJob.foodLayout                   = standardFoodLayout;
            playoffJob.firstExtraFoodPosIndex       = 0;
            playoffJob.secondExtraFoodPosIndex      = 0;

            jobCounter++;
            jobManagerAddPlayoffJobToQueue(playoffJob);

            // Add the 12 jobs with the single extra food item
            for (int i = 0; i < MAX_EXTRA_FOOD_PIECES; i++)
            {
                playoffJob.blueTeamContestantRobotIndex = blueTeamPlayers.at(blueTeamCounter).robotIndex;
                playoffJob.blueTeamContestantGenomeId   = blueTeamPlayers.at(blueTeamCounter).robotGenomeId;
                playoffJob.blueTeamContestantDataIndex  = blueTeamCounter;

                playoffJob.redTeamContestantRobotIndex  = redTeamPlayers.at(redTeamCounter).robotIndex;
                playoffJob.redTeamContestantGenomeId    = redTeamPlayers.at(redTeamCounter).robotGenomeId;
                playoffJob.redTeamContestantDataIndex   = redTeamCounter;

                playoffJob.foodLayout                   = singleExtraFoodLayout;
                playoffJob.firstExtraFoodPosIndex       = i;
                playoffJob.secondExtraFoodPosIndex      = 0;

                jobCounter++;
                jobManagerAddPlayoffJobToQueue(playoffJob);
            }
        }
    }

    return jobCounter;
}







// This function pushes all the generation champ matches
// into a queue to be pulled by the various execution
// threads to execute. The planned matches follow the
// same tournament style of the dominance tournament,
// but involve pitting the champs from both teams
// against each other.
void setupGenerationChampJobs(int blueChampIndex, int redChampIndex)
{
    evaluationMode = generationChampTournament;
    generationChampJobData genChampJob;

    // Add one job with the standard food layout
    genChampJob.blueTeamChampIndex      = blueChampIndex;
    genChampJob.redTeamChampIndex       = redChampIndex;
    genChampJob.foodLayout              = standardFoodLayout;
    genChampJob.firstExtraFoodPosIndex  = 0;
    genChampJob.secondExtraFoodPosIndex = 0;

    jobManagerAddGenerationChampJobToQueue(genChampJob);

    // Add the 12 jobs with a single additional food item
    for (int i = 0; i < MAX_EXTRA_FOOD_PIECES; i++)
    {
        genChampJob.blueTeamChampIndex      = blueChampIndex;
        genChampJob.redTeamChampIndex       = redChampIndex;
        genChampJob.foodLayout              = singleExtraFoodLayout;
        genChampJob.firstExtraFoodPosIndex  = i;
        genChampJob.secondExtraFoodPosIndex = 0;

        jobManagerAddGenerationChampJobToQueue(genChampJob);
    }

    // Add the 66 jobs with the two additional food items
    for (int i = 0; i < MAX_EXTRA_FOOD_PIECES; i++)
    {
        for (int j = i+1; j < MAX_EXTRA_FOOD_PIECES; j++)
        {
            genChampJob.blueTeamChampIndex      = blueChampIndex;
            genChampJob.redTeamChampIndex       = redChampIndex;
            genChampJob.foodLayout              = doubleExtraFoodLayout;
            genChampJob.firstExtraFoodPosIndex  = i;
            genChampJob.secondExtraFoodPosIndex = j;

            jobManagerAddGenerationChampJobToQueue(genChampJob);
        }
    }
    cout<<"Done setting up generation champ jobs..."<<endl;
}






// This function pushes dominance tournament matches
// into a queue to be pulled by the various execution
// threads to execute. The planned matches follow the
// same tournament style of the generation champ tournament
void setupDominanceTournamentJobs(int dominantStrategyIndex,
                                  hostTeamType genChampTeam,
                                  int genChampIndex)
{
    evaluationMode = dominanceTournament;
    dominanceTournamentJobData dominanceTournyJob;

    // Add one job with the standard food layout
    dominanceTournyJob.dominanceStrategyIndex  = dominantStrategyIndex;
    dominanceTournyJob.genChampTeam            = genChampTeam;
    dominanceTournyJob.genChampIndex           = genChampIndex;
    dominanceTournyJob.foodLayout              = standardFoodLayout;
    dominanceTournyJob.firstExtraFoodPosIndex  = 0;
    dominanceTournyJob.secondExtraFoodPosIndex = 0;

    jobManagerAddDominantTournyJobToQueue(dominanceTournyJob);

    // Add the 12 jobs with a single single additional food item
    for (int i = 0; i < MAX_EXTRA_FOOD_PIECES; i++)
    {
        dominanceTournyJob.dominanceStrategyIndex  = dominantStrategyIndex;
        dominanceTournyJob.genChampTeam            = genChampTeam;
        dominanceTournyJob.genChampIndex           = genChampIndex;
        dominanceTournyJob.foodLayout              = singleExtraFoodLayout;
        dominanceTournyJob.firstExtraFoodPosIndex  = i;
        dominanceTournyJob.secondExtraFoodPosIndex = 0;

        jobManagerAddDominantTournyJobToQueue(dominanceTournyJob);
    }

    // Add the 66 jobs with the two additional food items
    for (int i = 0; i < MAX_EXTRA_FOOD_PIECES; i++)
    {
        for (int j = i+1; j < MAX_EXTRA_FOOD_PIECES; j++)
        {
            dominanceTournyJob.dominanceStrategyIndex  = dominantStrategyIndex;
            dominanceTournyJob.genChampTeam            = genChampTeam;
            dominanceTournyJob.genChampIndex           = genChampIndex;
            dominanceTournyJob.foodLayout              = doubleExtraFoodLayout;
            dominanceTournyJob.firstExtraFoodPosIndex  = i;
            dominanceTournyJob.secondExtraFoodPosIndex = j;

            jobManagerAddDominantTournyJobToQueue(dominanceTournyJob);
        }
    }
}







// This function is called by the
// job manager thread to schedule
// NCD calculations by the execution
// threads
void setupNcdNumberCrunchingJobs(vector <bool> redTeamPassers,
                                 vector <bool> blueTeamPassers)
{
    // Setup the NCD number crunching
    // job queue
    evaluationMode = ncdCalculations;


    ncdJobData evalJob;

    for (int i = 0; i < param_numAgents; i++)
    {
        evalJob.team       = REDTEAM;
        evalJob.robotIndex = i;

        // If we're using minimal criteria novelty search,
        // make sure that we only evaluation those
        // who met the minimal criteria
        if (UseMinimalCriteria)
        {
            if (redTeamPassers.at(i) == true)
            {
                jobManagerAddNcdCalcJobToQueue(evalJob);
            }
        }
        else
        {
            jobManagerAddNcdCalcJobToQueue(evalJob);
        }

        evalJob.team       = BLUETEAM;
        evalJob.robotIndex = i;

        // If we're using minimal criteria novelty search,
        // make sure that we only evaluation those
        // who met the minimal criteria
        if (UseMinimalCriteria)
        {
            if (blueTeamPassers.at(i) == true)
            {
                jobManagerAddNcdCalcJobToQueue(evalJob);
            }
        }
        else
        {
            jobManagerAddNcdCalcJobToQueue(evalJob);
        }
    }
}







// This functions is called by the job manager thread to schedule
// evaluations of agents in the novelty archive, to make sure that
// the behavior that they have stored is against the same opponent
// that the standard population will have competed against.
// The enemy that the archive behavior will be put against is
// the first enemy robot would compete against in the
// standard tournament, which is the first agent in the vector
// of "team best robots". Only the left side will be evaluated.
void setupNoveltyArchiveRefreshJobs(int redTeamArchiveSize, int blueTeamArchiveSize)
{
    // Need new execution thread mode, and queues/structures for
    // schedulign and (retrieving?) results of novelty archive matches.
    // Nedd serialization code for buffers.
    // Need means of adding novelty archive members in both
    // NCD code, and in Cga (custom archive vectors).

    evaluationMode = archiveBehaviorMatches;

    archiveBehaviorRefreshJobData evalJob;

    for (int i = 0; i < redTeamArchiveSize; i++)
    {
        evalJob.team  = REDTEAM;
        evalJob.index = i;

        jobManagerAddArchBehaviorJobToQueue(evalJob);
    }

    for (int i = 0; i < blueTeamArchiveSize; i++)
    {
        evalJob.team  = BLUETEAM;
        evalJob.index = i;

        jobManagerAddArchBehaviorJobToQueue(evalJob);
    }
}







// This thread handles the job distribution to the 
// execution threads, and handles calling the relevant 
// genetic algorithm stuff at the end of a generation. 
void *runJobManagerThread(void*)
{
    static struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = 1000000;

    struct timespec longSleepTime;
    longSleepTime.tv_sec  = 0;
    longSleepTime.tv_nsec = longSleepAmount;

    const double compatibilityThresholdAdjustment = 0.3;

    // Max fitness bonus added to robots that make it to
    // the playoffs. See how this is divided up in the playoff
    // management code further down.
    const double playoffBonusMax                  = 25.0;

    // Second fitness bonus awarded to champion on a team
    // who wins the playoff.
    const double playoffWinnerExtraBonus          = 10.0;

    bool playoffRequired = false;

    unsigned int redChampTournyScore   = 0;
    unsigned int blueChampTournyScore  = 0;

    unsigned int genChampTournyScore   = 0;
    unsigned int dominantStratScore    = 0;
    int          dominantStratGenomeID = 0;

    unsigned int blueHighScore  = 0;
    unsigned int redHighScore   = 0;

    unsigned int redTeamChampIndex             = 0;
    unsigned int redTeamPlayoffWinnerGenomeID  = 0;
    unsigned int blueTeamChampIndex            = 0;
    unsigned int blueTeamPlayoffWinnerGenomeID = 0;

    unsigned int numRedTeamGenChamps  = 0;
    unsigned int numBlueTeamGenChamps = 0;
    unsigned int numRedTeamDomStrats  = 0;
    unsigned int numBlueTeamDomStrats = 0;

    bool dominantStratTournySkipped = false;


    unsigned int blueChampFitnessScore        = 0;
    unsigned int blueChampNumNeurons          = 0;
    int          blueChampGenomeID            = 0;
    int          blueChampNumBrainConnections = 0;
    int          blueChampNumBrainHiddenNodes = 0;

    float        blueTeamMeanNumNeurons       = 0.0;
    float        blueTeamMeanNumConnections   = 0.0;
    float        blueTeamMeanHiddenNodes      = 0.0;

    unsigned int redChampFitnessScore         = 0;
    unsigned int redChampNumNeurons           = 0;
    int          redChampGenomeID             = 0;
    int          redChampNumBrainConnections  = 0;
    int          redChampNumBrainHiddenNodes  = 0;

    float        redTeamMeanNumNeurons        = 0.0;
    float        redTeamMeanNumConnections    = 0.0;
    float        redTeamMeanHiddenNodes       = 0.0;

    int          redTeamNumPassedMinCriteria  = 0;
    int          blueTeamNumPassedMinCriteria = 0;

    unsigned int lastDominantStratNumNeurons          = 0;
    int          lastDominantStratNumBrainConnections = 0;
    int          lastDominantStratNumBrainHiddenNodes = 0;


    bool  genTournyTied           = false;
    bool  newDominantStrat        = false;

    int   gensSinceNewDomStrat    = 0;
    int   lastGenWithNewDomStrat  = 1;

    int   whichDominantStratWon   = 0;
    int   winningDomStratScore    = 0;
    int   losingChallengerScore   = 0;

    float standardEvalTime        = 0.0;
    float playoffEvalTime         = 0.0;
    float generationChampEvalTime = 0.0;
    float dominanceTournyEvalTime = 0.0;
    float generationTime          = 0.0;
    float totalRunTime            = 0.0;

    bool generationEvalsComplete   = false;

    Timer generationTimer;
    Timer standardEvalTimer;
    Timer noveltyEvalTimer;
    Timer playoffTimer;
    Timer genChampTimer;
    Timer dominanceTournamentTimer;
    Timer totalTimeAllGenerations;



    // Seed the psuedo random function
    srand((unsigned)time(NULL));


    // If we're recording data, go ahead and setup the file
    if (recordData)
    {
        openDataRecordFile();
    }



    // Create the robots, with copies for each
    // execution thread
    redTeamRobots.resize(param_numExecutionThreads);
    redTeamBestRobots.resize(param_numExecutionThreads);
    redTeamHallOfFameRobots.resize(param_numExecutionThreads);

    blueTeamRobots.resize(param_numExecutionThreads);
    blueTeamBestRobots.resize(param_numExecutionThreads);
    blueTeamHallOfFameRobots.resize(param_numExecutionThreads);


    for (int i = 0; i < param_numExecutionThreads; i++)
    {
        int j;

        for (j = 0; j < param_numAgents; j++)
        {
            redTeamRobots[i].push_back(new RobotAgent());
            blueTeamRobots[i].push_back(new RobotAgent());
        }

        for (j = 0; j < param_numBestAgentsPerTeam; j++)
        {
            redTeamBestRobots[i].push_back(new RobotAgent());
            blueTeamBestRobots[i].push_back(new RobotAgent());
        }

        for (j = 0; j < param_numHallOfFamers; j++)
        {
            redTeamHallOfFameRobots[i].push_back(new RobotAgent());
            blueTeamHallOfFameRobots[i].push_back(new RobotAgent());
        }
    }

    // Create the Cga typed genetics lab, one for
    // each team.
    Cga redTeamGeneticsLab (param_numAgents, param_numInputs, param_numOutputs);
    Cga blueTeamGeneticsLab(param_numAgents, param_numInputs, param_numOutputs);


    // Test Debug Mode stuff
    //    redTeamGeneticsLab.setDebugMode(true);
    //    blueTeamGeneticsLab.setDebugMode(true);


    /***********************************************
    //  NEAT and parameter validation!
    //  Uncomment this line to run XOR test and exit.
    //  Used for validating the NEAT implementation,
    //  and the parameters set. Only need to do it
    //  with one genetics lab, since both use
    //  the same NEAT parameters.
    ************************************************/
    //    redTeamGeneticsLab.RunXorTestAndExit();




    // If we're using novelty search, turn off automatic handling
    // of hall of fame inside the Cga class. If it had it's way,
    // the most novel genome per generation would be saved.
    // We'll track performance in the experiment, and manually
    // add the best performing genomes to the hall of fame
    if (UseNoveltySearch)
    {
        redTeamGeneticsLab.SetManualHallOfFameControl(true);
        blueTeamGeneticsLab.SetManualHallOfFameControl(true);
    }




    vector<CNeuralNet*> redTeamBrainVector;
    vector<CNeuralNet*> blueTeamBrainVector;

    // Create the initial neural networks
    for (int i = 0; i < param_numExecutionThreads; i++)
    {
        redTeamBrainVector  = redTeamGeneticsLab.CreatePhenotypesDuplicate();
        blueTeamBrainVector = blueTeamGeneticsLab.CreatePhenotypesDuplicate();

        for (int j = 0; j < param_numAgents; j++)
        {
            redTeamRobots[i][j]->insertNewBrain(redTeamBrainVector[j]);
            blueTeamRobots[i][j]->insertNewBrain(blueTeamBrainVector[j]);
        }

        // Since it's first generation, best agents and hall of famers
        // are BS. It'll fix itself after a couple of generations
        for (int j = 0; j < param_numBestAgentsPerTeam; j++)
        {
            redTeamBestRobots[i][j]->insertNewBrain(redTeamBrainVector[j]);
            blueTeamBestRobots[i][j]->insertNewBrain(blueTeamBrainVector[j]);
        }

        for (int j = 0; j < param_numHallOfFamers; j++)
        {
            redTeamHallOfFameRobots[i][j]->insertNewBrain(redTeamBrainVector[j + param_numBestAgentsPerTeam]);
            blueTeamHallOfFameRobots[i][j]->insertNewBrain(blueTeamBrainVector[j + param_numBestAgentsPerTeam]);
        }
    }

    // Wait until all of the execution threads are online
    while (!EXECUTION_THREADS_ONLINE)
    {
        cout<<"Job manager waiting for all threads to come online..."<<endl;
        nanosleep(&longSleepTime, NULL);
    }

    cout<<"Job manager thread online and commencing first generation"<<endl;
    totalTimeAllGenerations.reset();


    while ((generationCounter <= param_maxNumGenerations) && (!DONE))
    {
        // Reset stuff for a generation run
        jobManagerClearAllQueues();
        generationTimer.reset();
        standardEvalTimer.reset();
        generationEvalsComplete = false;

        cout<<"**********************************************************************"<<endl;
        cout<<"Starting generation "<<generationCounter<<endl;

        // Randomly select the hall-of-fame robots to use
        // for this generation
        jobManagerPickHallOfFameRobotsForEval();


        // If we're doing Novelty Search,
        // make sure that the behavior code
        // knows which genome ID's robots
        // are competing against
        // We don't need this if we're doing
        // static stimulation novelty search
        if (UseNoveltySearch && !UseStaticStimNovelty)
        {
            vector <int> enemyGenomeIDVec;
            int genomeID;

            // Setup Red teams reference data first
            // (They compete against blue robots)
            for (int i = 0; i < param_numBestAgentsPerTeam; i++)
            {
                genomeID = blueTeamBestRobots[0].at(i)->getBrainGenomeID();
                enemyGenomeIDVec.push_back(genomeID);
            }

            for (int i = 0; i < param_numHallOfFamers; i++)
            {
                genomeID = blueTeamHallOfFameRobots[0].at(exeThreadGetHallOfFamerIndex(i))->getBrainGenomeID();
                enemyGenomeIDVec.push_back(genomeID);
            }

            setGenerationEnemySeries(REDTEAM, enemyGenomeIDVec);


            // Now setup the blue teams reference data
            enemyGenomeIDVec.clear();

            for (int i = 0; i < param_numBestAgentsPerTeam; i++)
            {
                genomeID = redTeamBestRobots[0].at(i)->getBrainGenomeID();
                enemyGenomeIDVec.push_back(genomeID);
            }

            for (int i = 0; i < param_numHallOfFamers; i++)
            {
                genomeID = redTeamHallOfFameRobots[0].at(exeThreadGetHallOfFamerIndex(i))->getBrainGenomeID();
                enemyGenomeIDVec.push_back(genomeID);
            }

            setGenerationEnemySeries(BLUETEAM, enemyGenomeIDVec);

            enemyGenomeIDVec.clear();
        }


        /****************************************
        // Setup job queue
        ***************************************/
        setupStandardEvaluationJobs();



        while (!generationEvalsComplete)
        {
            /****************************************
            // Look for job results for processing
            // And perform fitness testing, and
            // Evolutionary Epoch. Since there are
            // two teams, we're looking for twice
            // the number of results as there are agents,
            // as numAgents
            // represents the number of agents on
            // a single team.
            ***************************************/
            if (jobManagerGetResultsQueueSize() == (param_numAgents * 2))
            {
                // We have the results for entire generation
                // population. Process the data,
                // do genetic epoch, and start another
                // generation
                blueHighScore  = 0;
                redHighScore   = 0;

                // This vector will contain the fitness
                // scores of the agents. We'll take
                // the host score from it's eval, and
                // use that as a fitness score. Since
                // fitness scores need to be greater
                // than zero, and zero is the lowest
                // score, we'll add 1, and square
                // the results to give us the fitness
                // score
                vector <double> blueTeamFitnessScores(param_numAgents);
                vector <double> redTeamFitnessScores(param_numAgents);
                double calculatedFitnessScore;

                agentEvaluationData evalResults;

                int numResultsToProcess = jobManagerGetResultsQueueSize();

                for (int i = 0; i < numResultsToProcess; i++)
                {
                    if (!jobManagerGetEvalResults(evalResults))
                    {
                        cout<<"Eval results queue empty when it shouldn't be!"<<endl;
                        exit(1);
                    }

                    // Add one, then square the score to get the fitness value to
                    // feed into the genetic algorithms. You don't want any
                    // fitness scores of 0 in NEAT
                    calculatedFitnessScore = pow(((double)(evalResults.score + 1)), 2);


                    if (evalResults.hostTeam == REDTEAM)
                    {
                        redTeamFitnessScores.erase(redTeamFitnessScores.begin() + evalResults.hostRobotIndex);
                        redTeamFitnessScores.insert(redTeamFitnessScores.begin() + evalResults.hostRobotIndex,
                                                    calculatedFitnessScore);

                        if (evalResults.score > redHighScore)
                        {
                            redHighScore      = evalResults.score;
                            redTeamChampIndex = evalResults.hostRobotIndex;
                        }
                    }
                    else
                    {
                        blueTeamFitnessScores.erase(blueTeamFitnessScores.begin() + evalResults.hostRobotIndex);
                        blueTeamFitnessScores.insert(blueTeamFitnessScores.begin() + evalResults.hostRobotIndex,
                                                     calculatedFitnessScore);

                        if (evalResults.score > blueHighScore)
                        {
                            blueHighScore      = evalResults.score;
                            blueTeamChampIndex = evalResults.hostRobotIndex;
                        }
                    }
                }


                /****************************************
                // Do some basic data collection
                ***************************************/
                blueTeamMeanNumNeurons     = 0.0;
                blueTeamMeanNumConnections = 0.0;
                blueTeamMeanHiddenNodes    = 0.0;

                for (int i = 0; i < param_numAgents; i++)
                {
                    blueTeamMeanNumNeurons     += blueTeamRobots[0][i]->getNumberOfBrainNeurons();
                    blueTeamMeanNumConnections += blueTeamRobots[0][i]->getNumberOfBrainConnections();
                    blueTeamMeanHiddenNodes    += blueTeamRobots[0][i]->getNumberOfBrainHiddenNodes();
                }

                blueTeamMeanNumNeurons     /= (float)param_numAgents;
                blueTeamMeanNumConnections /= (float)param_numAgents;
                blueTeamMeanHiddenNodes    /= (float)param_numAgents;


                redTeamMeanNumNeurons       = 0.0;
                redTeamMeanNumConnections   = 0.0;
                redTeamMeanHiddenNodes      = 0.0;

                for (int i = 0; i < param_numAgents; i++)
                {
                    redTeamMeanNumNeurons     += redTeamRobots[0][i]->getNumberOfBrainNeurons();
                    redTeamMeanNumConnections += redTeamRobots[0][i]->getNumberOfBrainConnections();
                    redTeamMeanHiddenNodes    += redTeamRobots[0][i]->getNumberOfBrainHiddenNodes();
                }

                redTeamMeanNumNeurons     /= (float)param_numAgents;
                redTeamMeanNumConnections /= (float)param_numAgents;
                redTeamMeanHiddenNodes    /= (float)param_numAgents;





                /****************************************************
                //            Playoffs!
                // Gotta check to see if multiple team members are
                // tied with the highest score, and do a playoff
                // to make sure we're actually using
                // the "best" from each team. Based on playoff
                // results, we'll add a bit to their fitness score
                // so that they won't be tied anymore, and to
                // reflect their performance.
                *****************************************************/
                unsigned int redTeamNumTiedChamps  = 0;
                unsigned int blueTeamNumTiedChamps = 0;

                int numPlayoffResultsToLookFor     = 0;

                // Make sure the high scores go through the same
                // equation that would be passed into genetic algorithm...
                redHighScore  = pow(((double)(redHighScore + 1)), 2);
                blueHighScore = pow(((double)(blueHighScore + 1)), 2);

                vector <playoffMemberReferenceData> redTeamPlayoffMembers;
                vector <playoffMemberReferenceData> blueTeamPlayoffMembers;

                playoffMemberReferenceData playoffMemberData;

                // Look for ties, setup vectors for tracking
                // and scoring playoffs
                for (int i = 0; i < param_numAgents; i++)
                {
                    if (redTeamFitnessScores.at(i) == redHighScore)
                    {
                        redTeamNumTiedChamps++;
                        playoffMemberData.robotIndex           = i;
                        playoffMemberData.robotGenomeId        = redTeamRobots[0][i]->getBrainGenomeID();
                        playoffMemberData.memberReferenceIndex = redTeamPlayoffMembers.size();
                        playoffMemberData.robotTeam            = REDTEAM;
                        playoffMemberData.numWins              = 0;
                        playoffMemberData.numLosses            = 0;
                        playoffMemberData.numTies              = 0;
                        playoffMemberData.globalScore          = 0;

                        redTeamPlayoffMembers.push_back(playoffMemberData);
                    }

                    if (blueTeamFitnessScores.at(i) == blueHighScore)
                    {
                        blueTeamNumTiedChamps++;
                        playoffMemberData.robotIndex           = i;
                        playoffMemberData.robotGenomeId        = blueTeamRobots[0][i]->getBrainGenomeID();
                        playoffMemberData.memberReferenceIndex = blueTeamPlayoffMembers.size();
                        playoffMemberData.robotTeam            = BLUETEAM;
                        playoffMemberData.numWins              = 0;
                        playoffMemberData.numLosses            = 0;
                        playoffMemberData.numTies              = 0;
                        playoffMemberData.globalScore          = 0;

                        blueTeamPlayoffMembers.push_back(playoffMemberData);
                    }
                }

                playoffRequired = false;

                // Do we need a playoff tournament?
                if ((redTeamNumTiedChamps > 1) || (blueTeamNumTiedChamps > 1))
                {
                    playoffRequired = true;
                    cout<<"------------------------------------------------------------"<<endl;
                    cout<<"Starting required playoff tournament between "<<redTeamPlayoffMembers.size()<<" red robots, and "<<blueTeamPlayoffMembers.size()<<" blue robots"<<endl;
                    playoffTimer.reset();

                    // Setup the enemy records in the playoff member vectors
                    playoffMemberEnemyData enemyData;

                    // Setup the red team data structures
                    for (unsigned int i = 0; i < redTeamPlayoffMembers.size(); i++)
                    {
                        for (unsigned int j = 0; j < blueTeamPlayoffMembers.size(); j++)
                        {
                            enemyData.enemyGenomeId = blueTeamPlayoffMembers[j].robotGenomeId;
                            enemyData.redScore      = 0;
                            enemyData.blueScore     = 0;

                            redTeamPlayoffMembers[i].enemyDataVector.push_back(enemyData);
                        }
                    }

                    // Setup the blue team data structures
                    for (unsigned int i = 0; i < blueTeamPlayoffMembers.size(); i++)
                    {
                        for (unsigned int j = 0; j < redTeamPlayoffMembers.size(); j++)
                        {
                            enemyData.enemyGenomeId = redTeamPlayoffMembers[j].robotGenomeId;
                            enemyData.redScore      = 0;
                            enemyData.blueScore     = 0;

                            blueTeamPlayoffMembers[i].enemyDataVector.push_back(enemyData);
                        }
                    }


                    // Schedule the playoff tournament jobs
                    numPlayoffResultsToLookFor = setupPlayoffTournamentJobs(redTeamPlayoffMembers, blueTeamPlayoffMembers);


                    // Wait for the playoff results...
                    while (jobManagerGetPlayoffResultsQueueSize() < numPlayoffResultsToLookFor)
                    {
                        nanosleep(&sleepTime, NULL);
                    }


                    // Organize the playoff tournament results
                    playoffEvalData playoffResultsData;

                    while (jobManagerGetPlayoffResultsQueueSize() != 0)
                    {
                        if (jobManagerGetPlayoffResults(playoffResultsData))
                        {
                            // Shorten up references for easier readability
                            int redIndex   = playoffResultsData.redTeamContestantDataIndex;
                            int redGenId   = playoffResultsData.redTeamContestantGenomeId;
                            int redPoints  = playoffResultsData.redTeamContestantScore;

                            int blueIndex  = playoffResultsData.blueTeamContestantDataIndex;
                            int blueGenId  = playoffResultsData.blueTeamContestantGenomeId;
                            int bluePoints = playoffResultsData.blueTeamContestantScore;

                            // Do some sanity checking on genomeID's....
                            if (redGenId != redTeamPlayoffMembers[redIndex].robotGenomeId)
                            {
                                cout<<"Error in organizing playoff results, red team genome ID doesn't match!"<<endl;
                                DONE = true;
                                exit(1);
                            }

                            if (blueGenId != blueTeamPlayoffMembers[blueIndex].robotGenomeId)
                            {
                                cout<<"Error in organizing playoff results, blue team genome ID doesn't match!"<<endl;
                                DONE = true;
                                exit(1);
                            }

                            if (redGenId != blueTeamPlayoffMembers[blueIndex].enemyDataVector[redIndex].enemyGenomeId)
                            {
                                cout<<"Error in organizing playoff results, blue team enemy genome ID doesn't match!"<<endl;
                                DONE = true;
                                exit(1);
                            }

                            if (blueGenId != redTeamPlayoffMembers[redIndex].enemyDataVector[blueIndex].enemyGenomeId)
                            {
                                cout<<"Error in organizing playoff results, red team enemy genome ID doesn't match!"<<endl;
                                DONE = true;
                                exit(1);
                            }

                            redTeamPlayoffMembers[redIndex].enemyDataVector[blueIndex].redScore   += redPoints;
                            redTeamPlayoffMembers[redIndex].enemyDataVector[blueIndex].blueScore  += bluePoints;

                            blueTeamPlayoffMembers[blueIndex].enemyDataVector[redIndex].redScore  += redPoints;
                            blueTeamPlayoffMembers[blueIndex].enemyDataVector[redIndex].blueScore += bluePoints;
                        }
                    }


                    int redTeamMaxNumWins       = 0;
                    int numRedTeamTiesWithWins  = 0;
                    int blueTeamMaxNumWins      = 0;
                    int numBlueTeamTiesWithWins = 0;

                    // Figure out the rolled up scoring for red team
                    for (unsigned int redCounter = 0; redCounter < redTeamPlayoffMembers.size(); redCounter++)
                    {
                        for (unsigned int enemyCounter = 0;
                             enemyCounter < redTeamPlayoffMembers[redCounter].enemyDataVector.size();
                             enemyCounter++)
                        {
                            int redPoints  = redTeamPlayoffMembers[redCounter].enemyDataVector[enemyCounter].redScore;
                            int bluePoints = redTeamPlayoffMembers[redCounter].enemyDataVector[enemyCounter].blueScore;

                            if (redPoints > bluePoints)
                            {
                                redTeamPlayoffMembers[redCounter].numWins++;
                            }
                            else if (redPoints < bluePoints)
                            {
                                redTeamPlayoffMembers[redCounter].numLosses++;
                            }
                            else if (redPoints == bluePoints)
                            {
                                redTeamPlayoffMembers[redCounter].numTies++;
                            }

                            redTeamPlayoffMembers[redCounter].globalScore += redPoints - bluePoints;
                        }

                        if (redTeamPlayoffMembers[redCounter].numWins > redTeamMaxNumWins)
                        {
                            redTeamMaxNumWins      = redTeamPlayoffMembers[redCounter].numWins;
                            numRedTeamTiesWithWins = 0;
                        }

                        if (redTeamPlayoffMembers[redCounter].numWins == redTeamMaxNumWins)
                        {
                            numRedTeamTiesWithWins++;
                        }
                    }

                    // Figure out global scoring for blue team
                    for (unsigned int blueCounter = 0; blueCounter < blueTeamPlayoffMembers.size(); blueCounter++)
                    {
                        for (unsigned int enemyCounter = 0;
                             enemyCounter < blueTeamPlayoffMembers[blueCounter].enemyDataVector.size();
                             enemyCounter++)
                        {
                            int redPoints  = blueTeamPlayoffMembers[blueCounter].enemyDataVector[enemyCounter].redScore;
                            int bluePoints = blueTeamPlayoffMembers[blueCounter].enemyDataVector[enemyCounter].blueScore;

                            if (redPoints < bluePoints)
                            {
                                blueTeamPlayoffMembers[blueCounter].numWins++;
                            }
                            else if (redPoints > bluePoints)
                            {
                                blueTeamPlayoffMembers[blueCounter].numLosses++;
                            }
                            else if (redPoints == bluePoints)
                            {
                                blueTeamPlayoffMembers[blueCounter].numTies++;
                            }

                            blueTeamPlayoffMembers[blueCounter].globalScore += bluePoints - redPoints;
                        }

                        if (blueTeamPlayoffMembers[blueCounter].numWins > blueTeamMaxNumWins)
                        {
                            blueTeamMaxNumWins      = blueTeamPlayoffMembers[blueCounter].numWins;
                            numBlueTeamTiesWithWins = 0;
                        }

                        if (blueTeamPlayoffMembers[blueCounter].numWins == blueTeamMaxNumWins)
                        {
                            numBlueTeamTiesWithWins++;
                        }
                    }


                    // Now go through all of the records, and get a vector of all the unique
                    // number of wins per team. For each number of wins, we'll assign a
                    // fitness "bonus", with more wins receiving a bigger bonus. We're
                    // simply refining the fitness scores for the top performers per
                    // team, prior to evolutionary Epoch.
                    vector <int> redTeamNumWinsVector;
                    vector <int> blueTeamNumWinsVector;

                    for (unsigned int i = 0; i < redTeamPlayoffMembers.size(); i++)
                    {
                        bool numWinsFound = false;
                        int  robotNumWins = redTeamPlayoffMembers[i].numWins;

                        if (redTeamNumWinsVector.empty())
                        {
                            redTeamNumWinsVector.push_back(robotNumWins);
                        }
                        else
                        {
                            for (unsigned int j = 0; j < redTeamNumWinsVector.size(); j++)
                            {
                                if (redTeamNumWinsVector[j] == robotNumWins)
                                {
                                    numWinsFound = true;
                                }
                            }

                            if (!numWinsFound)
                            {
                                // This number of wins isn't contained in the vector yet
                                redTeamNumWinsVector.push_back(robotNumWins);
                            }
                        }
                    }

                    for (unsigned int i = 0; i < blueTeamPlayoffMembers.size(); i++)
                    {
                        bool numWinsFound = false;
                        int  robotNumWins = blueTeamPlayoffMembers[i].numWins;

                        if (blueTeamNumWinsVector.empty())
                        {
                            blueTeamNumWinsVector.push_back(robotNumWins);
                        }
                        else
                        {
                            for (unsigned int j = 0; j < blueTeamNumWinsVector.size(); j++)
                            {
                                if (blueTeamNumWinsVector[j] == robotNumWins)
                                {
                                    numWinsFound = true;
                                }
                            }

                            if (!numWinsFound)
                            {
                                // This number of wins isn't contained in the vector yet
                                blueTeamNumWinsVector.push_back(robotNumWins);
                            }
                        }
                    }

                    // Sort the unique num wins vectors
                    sort(redTeamNumWinsVector.begin(), redTeamNumWinsVector.end());
                    sort(blueTeamNumWinsVector.begin(), blueTeamNumWinsVector.end());


                    // Calculate standard playoff bonuses
                    // Doesn't include winner additional bonus, which may
                    // require tiebreaker stats
                    vector <double> redTeamPlayoffBonuses;
                    vector <double> blueTeamPlayoffBonuses;

                    for (unsigned int i = 0; i < redTeamNumWinsVector.size(); i++)
                    {
                        redTeamPlayoffBonuses.push_back(((playoffBonusMax/(double)redTeamNumWinsVector.size()) * (i+1)));
                    }

                    for (unsigned int i = 0; i < blueTeamNumWinsVector.size(); i++)
                    {
                        blueTeamPlayoffBonuses.push_back(((playoffBonusMax/(double)blueTeamNumWinsVector.size()) * (i+1)));
                    }

                    // Add in the standard playoff bonus for everyone
                    for (unsigned int i = 0; i < redTeamPlayoffMembers.size(); i++)
                    {
                        double adjustedFitness = redTeamFitnessScores[redTeamPlayoffMembers[i].robotIndex];

                        for (unsigned int j = 0; j < redTeamNumWinsVector.size(); j++)
                        {
                            if (redTeamPlayoffMembers[i].numWins == redTeamNumWinsVector[j])
                            {
                                adjustedFitness += redTeamPlayoffBonuses[j];

                                if (redTeamRobots[0][redTeamPlayoffMembers[i].robotIndex]->getBrainGenomeID() !=
                                    redTeamPlayoffMembers[i].robotGenomeId)
                                {
                                    cout<<"Error, in red team playoff bonus application, genome ID's don't match!"<<endl;
                                    DONE = true;
                                    exit(1);
                                }

                                redTeamFitnessScores[redTeamPlayoffMembers[i].robotIndex] = adjustedFitness;
                            }
                        }
                    }

                    for (unsigned int i = 0; i < blueTeamPlayoffMembers.size(); i++)
                    {
                        double adjustedFitness = blueTeamFitnessScores[blueTeamPlayoffMembers[i].robotIndex];

                        for (unsigned int j = 0; j < blueTeamNumWinsVector.size(); j++)
                        {
                            if (blueTeamPlayoffMembers[i].numWins == blueTeamNumWinsVector[j])
                            {
                                adjustedFitness += blueTeamPlayoffBonuses[j];

                                if (blueTeamRobots[0][blueTeamPlayoffMembers[i].robotIndex]->getBrainGenomeID() !=
                                    blueTeamPlayoffMembers[i].robotGenomeId)
                                {
                                    cout<<"Error, in blue team playoff bonus application, genome ID's don't match!"<<endl;
                                    DONE = true;
                                    exit(1);
                                }

                                blueTeamFitnessScores[blueTeamPlayoffMembers[i].robotIndex] = adjustedFitness;
                            }
                        }
                    }



                    // Tie breaker for playoff members that have the same number of most wins.
                    // Whoever wins this tie breaker gets the additional playoffWinnerExtraBonus.
                    // Since wins are the same, next check for least loses. If still tied
                    // after checking least losses, take the robot with the highest
                    // global score. If it's STILL tied after that, just pick one at random

                    redTeamPlayoffWinnerGenomeID  = 0;
                    blueTeamPlayoffWinnerGenomeID = 0;

                    // Does red team need a tie breaker?
                    if (numRedTeamTiesWithWins > 1)
                    {
                        // Need tie breaker....
                        vector <int> redTeamTiedIndexes;
                        vector <int> redTeamLosses;

                        for (unsigned int i = 0; i < redTeamPlayoffMembers.size(); i++)
                        {
                            if (redTeamPlayoffMembers[i].numWins == redTeamMaxNumWins)
                            {
                                redTeamTiedIndexes.push_back(i);
                            }
                        }

                        for (unsigned int i = 0; i < redTeamTiedIndexes.size(); i++)
                        {
                            redTeamLosses.push_back(redTeamPlayoffMembers[redTeamTiedIndexes[i]].numLosses);
                        }

                        sort(redTeamLosses.begin(), redTeamLosses.end());

                        int leastLosses        = redTeamLosses[0];


                        // Redo tied index vector so it matches those with max wins, and
                        // least losses, we might have narrowed it down more now...
                        redTeamTiedIndexes.clear();

                        for (unsigned int i = 0; i < redTeamPlayoffMembers.size(); i++)
                        {
                            if ((redTeamPlayoffMembers[i].numWins == redTeamMaxNumWins) &&
                                (redTeamPlayoffMembers[i].numLosses == leastLosses))
                            {
                                redTeamTiedIndexes.push_back(i);
                            }
                        }


                        if (redTeamTiedIndexes.size() > 1)
                        {
                            // Need second stage tie breaker....
                            vector <int> redTeamGlobalScores;

                            for (unsigned int i = 0; i < redTeamTiedIndexes.size(); i++)
                            {
                                redTeamGlobalScores.push_back(redTeamPlayoffMembers[redTeamTiedIndexes[i]].globalScore);
                            }

                            sort(redTeamGlobalScores.begin(), redTeamGlobalScores.end());

                            int highestGlobalScore        = redTeamGlobalScores[redTeamGlobalScores.size() - 1];

                            // Redo tied index vector so it matches those with max wins,
                            // least losses, and highest global score. Maybe narrowed down more now...
                            redTeamTiedIndexes.clear();

                            for (unsigned int i = 0; i < redTeamPlayoffMembers.size(); i++)
                            {
                                if ((redTeamPlayoffMembers[i].numWins == redTeamMaxNumWins) &&
                                    (redTeamPlayoffMembers[i].numLosses == leastLosses) &&
                                    (redTeamPlayoffMembers[i].globalScore == highestGlobalScore))
                                {
                                    redTeamTiedIndexes.push_back(i);
                                }
                            }

                            if (redTeamTiedIndexes.size() > 1)
                            {
                                // Well, that's it. We'll have to pick one randomly now.
                                int winnerIndex        = RandInt(0, redTeamTiedIndexes.size() - 1);
                                int winnerPlayoffIndex = redTeamTiedIndexes[winnerIndex];

                                double adjustedFitness  = redTeamFitnessScores[redTeamPlayoffMembers[winnerPlayoffIndex].robotIndex];
                                adjustedFitness        += playoffWinnerExtraBonus;

                                if (redTeamRobots[0][redTeamPlayoffMembers[winnerPlayoffIndex].robotIndex]->getBrainGenomeID() !=
                                    redTeamPlayoffMembers[winnerPlayoffIndex].robotGenomeId)
                                {
                                    cout<<"Error in red team playoff global score tie breaker extra bonus, genome ID's don't match!"<<endl;
                                    DONE = true;
                                    exit(1);
                                }

                                redTeamPlayoffWinnerGenomeID = redTeamPlayoffMembers[winnerPlayoffIndex].robotGenomeId;
                                redTeamChampIndex            = redTeamPlayoffMembers[winnerPlayoffIndex].robotIndex;
                                redTeamFitnessScores[redTeamPlayoffMembers[winnerPlayoffIndex].robotIndex] = adjustedFitness;
                            }
                            else
                            {
                                // Found a winner with global score tie breaker!
                                int winnerIndex = redTeamTiedIndexes[0];

                                double adjustedFitness  = redTeamFitnessScores[redTeamPlayoffMembers[winnerIndex].robotIndex];
                                adjustedFitness        += playoffWinnerExtraBonus;

                                if (redTeamRobots[0][redTeamPlayoffMembers[winnerIndex].robotIndex]->getBrainGenomeID() !=
                                    redTeamPlayoffMembers[winnerIndex].robotGenomeId)
                                {
                                    cout<<"Error in red team playoff global score tie breaker extra bonus, genome ID's don't match!"<<endl;
                                    DONE = true;
                                    exit(1);
                                }

                                redTeamPlayoffWinnerGenomeID = redTeamPlayoffMembers[winnerIndex].robotGenomeId;
                                redTeamChampIndex            = redTeamPlayoffMembers[winnerIndex].robotIndex;
                                redTeamFitnessScores[redTeamPlayoffMembers[winnerIndex].robotIndex] = adjustedFitness;
                            }
                        }
                        else
                        {
                            // Found a winner with num losses tie breaker!
                            int winnerIndex = redTeamTiedIndexes[0];

                            double adjustedFitness  = redTeamFitnessScores[redTeamPlayoffMembers[winnerIndex].robotIndex];
                            adjustedFitness        += playoffWinnerExtraBonus;

                            if (redTeamRobots[0][redTeamPlayoffMembers[winnerIndex].robotIndex]->getBrainGenomeID() !=
                                redTeamPlayoffMembers[winnerIndex].robotGenomeId)
                            {
                                cout<<"Error in red team playoff losses tie breaker extra bonus, genome ID's don't match!"<<endl;
                                DONE = true;
                                exit(1);
                            }

                            redTeamPlayoffWinnerGenomeID = redTeamPlayoffMembers[winnerIndex].robotGenomeId;
                            redTeamChampIndex            = redTeamPlayoffMembers[winnerIndex].robotIndex;
                            redTeamFitnessScores[redTeamPlayoffMembers[winnerIndex].robotIndex] = adjustedFitness;
                        }
                    }
                    else
                    {
                        // Only 1 Red robot with the highest score in the
                        // playoff tournament, no tie breaker needed.
                        // Give the robot the additional playoff
                        // winner bonus
                        for (unsigned int i = 0; i < redTeamPlayoffMembers.size(); i++)
                        {
                            // find the winner
                            if (redTeamPlayoffMembers[i].numWins == redTeamMaxNumWins)
                            {
                                double adjustedFitness  = redTeamFitnessScores[redTeamPlayoffMembers[i].robotIndex];
                                adjustedFitness        += playoffWinnerExtraBonus;

                                if (redTeamRobots[0][redTeamPlayoffMembers[i].robotIndex]->getBrainGenomeID() !=
                                    redTeamPlayoffMembers[i].robotGenomeId)
                                {
                                    cout<<"Error in red team playoff winner extra bonus, genome ID's don't match!"<<endl;
                                    DONE = true;
                                    exit(1);
                                }

                                redTeamPlayoffWinnerGenomeID = redTeamPlayoffMembers[i].robotGenomeId;
                                redTeamChampIndex            = redTeamPlayoffMembers[i].robotIndex;
                                redTeamFitnessScores[redTeamPlayoffMembers[i].robotIndex] = adjustedFitness;
                            }
                        }
                    }


                    // Does blue team need a tie breaker?
                    if (numBlueTeamTiesWithWins > 1)
                    {
                        // Need tie breaker....
                        vector <int> blueTeamTiedIndexes;
                        vector <int> blueTeamLosses;

                        for (unsigned int i = 0; i < blueTeamPlayoffMembers.size(); i++)
                        {
                            if (blueTeamPlayoffMembers[i].numWins == blueTeamMaxNumWins)
                            {
                                blueTeamTiedIndexes.push_back(i);
                            }
                        }

                        for (unsigned int i = 0; i < blueTeamTiedIndexes.size(); i++)
                        {
                            blueTeamLosses.push_back(blueTeamPlayoffMembers[blueTeamTiedIndexes[i]].numLosses);
                        }

                        sort(blueTeamLosses.begin(), blueTeamLosses.end());

                        int leastLosses        = blueTeamLosses[0];


                        // Redo tied index vector so it matches those with max wins, and
                        // least losses, we might have narrowed it down more now...
                        blueTeamTiedIndexes.clear();

                        for (unsigned int i = 0; i < blueTeamPlayoffMembers.size(); i++)
                        {
                            if ((blueTeamPlayoffMembers[i].numWins == blueTeamMaxNumWins) &&
                                (blueTeamPlayoffMembers[i].numLosses == leastLosses))
                            {
                                blueTeamTiedIndexes.push_back(i);
                            }
                        }


                        if (blueTeamTiedIndexes.size() > 1)
                        {
                            // Need second stage tie breaker....
                            vector <int> blueTeamGlobalScores;

                            for (unsigned int i = 0; i < blueTeamTiedIndexes.size(); i++)
                            {
                                blueTeamGlobalScores.push_back(blueTeamPlayoffMembers[blueTeamTiedIndexes[i]].globalScore);
                            }

                            sort(blueTeamGlobalScores.begin(), blueTeamGlobalScores.end());

                            int highestGlobalScore        = blueTeamGlobalScores[blueTeamGlobalScores.size() - 1];


                            // Redo tied index vector so it matches those with max wins,
                            // least losses, and highest global score. Maybe narrowed down more now...
                            blueTeamTiedIndexes.clear();

                            for (unsigned int i = 0; i < blueTeamPlayoffMembers.size(); i++)
                            {
                                if ((blueTeamPlayoffMembers[i].numWins == blueTeamMaxNumWins) &&
                                    (blueTeamPlayoffMembers[i].numLosses == leastLosses) &&
                                    (blueTeamPlayoffMembers[i].globalScore == highestGlobalScore))
                                {
                                    blueTeamTiedIndexes.push_back(i);
                                }
                            }


                            if (blueTeamTiedIndexes.size() > 1)
                            {
                                // Well, that's it. We'll have to pick one randomly now.
                                int winnerIndex        = RandInt(0, blueTeamTiedIndexes.size() - 1);
                                int winnerPlayoffIndex = blueTeamTiedIndexes[winnerIndex];

                                double adjustedFitness  = blueTeamFitnessScores[blueTeamPlayoffMembers[winnerPlayoffIndex].robotIndex];
                                adjustedFitness        += playoffWinnerExtraBonus;

                                if (blueTeamRobots[0][blueTeamPlayoffMembers[winnerPlayoffIndex].robotIndex]->getBrainGenomeID() !=
                                    blueTeamPlayoffMembers[winnerPlayoffIndex].robotGenomeId)
                                {
                                    cout<<"Error in blue team playoff global score tie breaker extra bonus, genome ID's don't match!"<<endl;
                                    DONE = true;
                                    exit(1);
                                }

                                blueTeamPlayoffWinnerGenomeID = blueTeamPlayoffMembers[winnerPlayoffIndex].robotGenomeId;
                                blueTeamChampIndex            = blueTeamPlayoffMembers[winnerPlayoffIndex].robotIndex;
                                blueTeamFitnessScores[blueTeamPlayoffMembers[winnerPlayoffIndex].robotIndex] = adjustedFitness;
                            }
                            else
                            {
                                // Found a winner with global score tie breaker!
                                int winnerIndex = blueTeamTiedIndexes[0];

                                double adjustedFitness  = blueTeamFitnessScores[blueTeamPlayoffMembers[winnerIndex].robotIndex];
                                adjustedFitness        += playoffWinnerExtraBonus;

                                if (blueTeamRobots[0][blueTeamPlayoffMembers[winnerIndex].robotIndex]->getBrainGenomeID() !=
                                    blueTeamPlayoffMembers[winnerIndex].robotGenomeId)
                                {
                                    cout<<"Error in blue team playoff global score tie breaker extra bonus, genome ID's don't match!"<<endl;
                                    DONE = true;
                                    exit(1);
                                }

                                blueTeamPlayoffWinnerGenomeID = blueTeamPlayoffMembers[winnerIndex].robotGenomeId;
                                blueTeamChampIndex            = blueTeamPlayoffMembers[winnerIndex].robotIndex;
                                blueTeamFitnessScores[blueTeamPlayoffMembers[winnerIndex].robotIndex] = adjustedFitness;
                            }
                        }
                        else
                        {
                            // Found a winner with num losses tie breaker!
                            int winnerIndex = blueTeamTiedIndexes[0];

                            double adjustedFitness  = blueTeamFitnessScores[blueTeamPlayoffMembers[winnerIndex].robotIndex];
                            adjustedFitness        += playoffWinnerExtraBonus;

                            if (blueTeamRobots[0][blueTeamPlayoffMembers[winnerIndex].robotIndex]->getBrainGenomeID() !=
                                blueTeamPlayoffMembers[winnerIndex].robotGenomeId)
                            {
                                cout<<"Error in blue team playoff losses tie breaker extra bonus, genome ID's don't match!"<<endl;
                                DONE = true;
                                exit(1);
                            }

                            blueTeamPlayoffWinnerGenomeID = blueTeamPlayoffMembers[winnerIndex].robotGenomeId;
                            blueTeamChampIndex            = blueTeamPlayoffMembers[winnerIndex].robotIndex;
                            blueTeamFitnessScores[blueTeamPlayoffMembers[winnerIndex].robotIndex] = adjustedFitness;
                        }
                    }
                    else
                    {
                        // Only 1 blue robot with the highest score in the
                        // playoff tournament, no tie breaker needed.
                        // Give the robot the additional playoff
                        // winner bonus
                        for (unsigned int i = 0; i < blueTeamPlayoffMembers.size(); i++)
                        {
                            // find the winner
                            if (blueTeamPlayoffMembers[i].numWins == blueTeamMaxNumWins)
                            {
                                double adjustedFitness  = blueTeamFitnessScores[blueTeamPlayoffMembers[i].robotIndex];
                                adjustedFitness        += playoffWinnerExtraBonus;

                                if (blueTeamRobots[0][blueTeamPlayoffMembers[i].robotIndex]->getBrainGenomeID() !=
                                    blueTeamPlayoffMembers[i].robotGenomeId)
                                {
                                    cout<<"Error in blue team playoff winner extra bonus, genome ID's don't match!"<<endl;
                                    DONE = true;
                                    exit(1);
                                }

                                blueTeamPlayoffWinnerGenomeID = blueTeamPlayoffMembers[i].robotGenomeId;
                                blueTeamChampIndex            = blueTeamPlayoffMembers[i].robotIndex;
                                blueTeamFitnessScores[blueTeamPlayoffMembers[i].robotIndex] = adjustedFitness;
                            }
                        }
                    }

                    playoffEvalTime = playoffTimer.total();
                    cout<<"At end of playoffs, team champ genomes are red: "<<redTeamPlayoffWinnerGenomeID<<", and blue: "<<blueTeamPlayoffWinnerGenomeID<<endl;
                    cout<<"Playoff Tournament completed in "<<playoffEvalTime<<" seconds"<<endl;
                    cout<<"------------------------------------------------------------"<<endl;
                }



                /****************************************************
                //            Novelty Search Stuff
                *****************************************************/
                vector <bool> redTeamMemberPassedMinCriteria;
                vector <bool> blueTeamMemberPassedMinCriteria;
                int           numNcdResultsWithMinCriteria;

                // If we're using minimal criteria,
                // we need to a record of who passed the
                // criteria so we only schedule
                // calculations for those agents
                // who passed the criteria. The records
                // will also be useful later for
                // flunking the scores of those who
                // didn't pass, before calling Epoch.
                if (UseNoveltySearch && UseMinimalCriteria)
                {
                    int redCounter  = 0;
                    int blueCounter = 0;
                    for (unsigned int i = 0; i < redTeamFitnessScores.size(); i++)
                    {
                        if (redTeamFitnessScores.at(i) >= param_minimalCriteria)
                        {
                            redCounter++;
                            redTeamMemberPassedMinCriteria.push_back(true);
                        }
                        else
                        {
                            redTeamMemberPassedMinCriteria.push_back(false);
                        }
                    }
                    redTeamNumPassedMinCriteria = redCounter;
                    cout<<"Red team has "<<redCounter<<" agents who pass minimal criteria"<<endl;

                    for (unsigned int i = 0; i < blueTeamFitnessScores.size(); i++)
                    {
                        if (blueTeamFitnessScores.at(i) >= param_minimalCriteria)
                        {
                            blueCounter++;
                            blueTeamMemberPassedMinCriteria.push_back(true);
                        }
                        else
                        {
                            blueTeamMemberPassedMinCriteria.push_back(false);
                        }
                    }
                    blueTeamNumPassedMinCriteria = blueCounter;

                    numNcdResultsWithMinCriteria = redCounter + blueCounter;
                    cout<<"Blue team has "<<blueCounter<<" agents who pass minimal criteria"<<endl;
                    cout<<"Will schedule NCD evals for "<<numNcdResultsWithMinCriteria<<" members"<<endl;
                }



                // These vectors will contain the novelty scores
                // that we'll pass to the Epoch method. Basically
                // replacing the fitness scores with novelty measures
                // using NCD.
                vector <double> blueTeamNoveltyScores(param_numAgents);
                vector <double> redTeamNoveltyScores(param_numAgents);
                ncdEvalData ncdResults;

                vector <ncdScoreReferenceData> redTeamNoveltyRefVector;
                vector <ncdScoreReferenceData> blueTeamNoveltyRefVector;
                ncdScoreReferenceData noveltyRefData;

                if (UseNoveltySearch)
                {
                    cout<<"Starting novel behavior calculations..."<<endl;
                    blueTeamNoveltyRefVector.clear();
                    redTeamNoveltyRefVector.clear();

                    // We don't need to update any
                    // archive behaviors if we're using
                    // static stimulus.
                    if (!UseStaticStimNovelty)
                    {
                        // Before you run the novelty calculations,
                        // you need to update the behavior data of
                        // the agents in the novelty archive, so
                        // that they competed against the same
                        // opponent as this generations population.
                        // This should help keep things on the level.

                        unsigned int redTeamArchiveSize;
                        unsigned int blueTeamArchiveSize;

                        redTeamArchiveSize  = getArchiveSize(REDTEAM);
                        blueTeamArchiveSize = getArchiveSize(BLUETEAM);

                        if (redTeamArchiveSize != redTeamGeneticsLab.GetNumCustomArchivers())
                        {
                            cout<<"Error in job manager, custom archive size mismatch for red team!"<<endl;
                            cout<<"NCD code base says red team archive size: "<<redTeamArchiveSize<<", Cga class: "<<redTeamGeneticsLab.GetNumCustomArchivers()<<endl;
                            DONE = true;
                            exit(1);
                        }

                        if (blueTeamArchiveSize != blueTeamGeneticsLab.GetNumCustomArchivers())
                        {
                            cout<<"Error in job manager, custom archive size mismatch for blue team!"<<endl;
                            cout<<"NCD code base says blue team archive size: "<<blueTeamArchiveSize<<", Cga class: "<<blueTeamGeneticsLab.GetNumCustomArchivers()<<endl;
                            DONE = true;
                            exit(1);
                        }


                        // We only have to worry about this
                        // if there are actually archived
                        // behavior
                        if (redTeamArchiveSize > 0)
                        {
                            // Make some robots with the brains of
                            // those in the novelty archive
                            redTeamNoveltyArchiveRobots.clear();
                            redTeamNoveltyArchiveRobots.resize(param_numExecutionThreads);

                            vector <CNeuralNet*> redTeamNoveltyArchiveBrains;

                            for (int i = 0; i < param_numExecutionThreads; i++)
                            {
                                redTeamNoveltyArchiveBrains = redTeamGeneticsLab.GetCustomArchivePhenotypesDuplicate();

                                for (int j = 0; j < redTeamNoveltyArchiveBrains.size(); j++)
                                {
                                    redTeamNoveltyArchiveRobots[i].push_back(new RobotAgent());
                                    redTeamNoveltyArchiveRobots[i][j]->insertNewBrain(redTeamNoveltyArchiveBrains[j]);
                                }
                            }
                        }

                        if (blueTeamArchiveSize > 0)
                        {
                            // Make some robots with the brains of
                            // those in the novelty archive
                            blueTeamNoveltyArchiveRobots.clear();
                            blueTeamNoveltyArchiveRobots.resize(param_numExecutionThreads);

                            vector <CNeuralNet*> blueTeamNoveltyArchiveBrains;

                            for (int i = 0; i < param_numExecutionThreads; i++)
                            {
                                blueTeamNoveltyArchiveBrains = blueTeamGeneticsLab.GetCustomArchivePhenotypesDuplicate();

                                for (int j = 0; j < blueTeamNoveltyArchiveBrains.size(); j++)
                                {
                                    blueTeamNoveltyArchiveRobots[i].push_back(new RobotAgent());
                                    blueTeamNoveltyArchiveRobots[i][j]->insertNewBrain(blueTeamNoveltyArchiveBrains[j]);
                                }
                            }
                        }


                        // Schedule the execution threads to run the new "matches"
                        // of the genomes in the novelty archives.
                        int numResultsToLookFor = redTeamArchiveSize + blueTeamArchiveSize;

                        if (numResultsToLookFor > 0)
                        {
                            cout<<"Scheduling "<<numResultsToLookFor<<" archived behavior refresh matches..."<<endl;
                            setupNoveltyArchiveRefreshJobs(redTeamArchiveSize, blueTeamArchiveSize);

                            // Wait for archive behavior matches to finish....
                            while (jobManagerGetArchBehaviorResultsQueueSize() < numResultsToLookFor)
                            {
                                nanosleep(&longSleepTime, NULL);
                            }

                            // We got 'em all. We don't need the values from the
                            // queue, the "data" is stored in the NCD function
                            // behavioral data. Just clear the results queue.
                            jobManagerClearArchBehaviorResultsQueue();
                        }
                        else
                        {
                            cout<<"No archived behaviors, refresh matches being skipped..."<<endl;
                        }
                    }



                    // Now do the novelty comparisons for the standard population
                    cout<<"Scheduling novelty comparisons using NCD for general population..."<<endl;

                    double redTeamMaxNovelty    = 0.0;
                    double redTeamMinNovelty    = 99999.0;
                    double redTeamMeanNovelty   = 0.0;
                    double redTeamNoveltyStdDev = 0.0;

                    double blueTeamMaxNovelty    = 0.0;
                    double blueTeamMinNovelty    = 99999.0;
                    double blueTeamMeanNovelty   = 0.0;
                    double blueTeamNoveltyStdDev = 0.0;

                    double redTeamMaxAdjustedNovelty  = 0.0;
                    double redTeamMinAdjustedNovelty  = 99999.0;
                    double blueTeamMaxAdjustedNovelty = 0.0;
                    double blueTeamMinAdjustedNovelty = 99999.0;


                    noveltyEvalTimer.reset();


                    // Schedule novelty search NCD calculations
                    // by the execution threads. Theses NCD
                    // results include testing against the
                    // EXISTING archive. Determining whether
                    // new additions to the archive are in
                    // order is handled further down
                    // The teamMemberPassMinCriteria boolean vectors
                    // are ignored by setupNcdNumberCrunchingJobs
                    // unless we're in UseMinimalCriteria mode
                    setupNcdNumberCrunchingJobs(redTeamMemberPassedMinCriteria,
                                                blueTeamMemberPassedMinCriteria);

                    int numNcdResultsToLookFor;

                    if (UseMinimalCriteria)
                    {
                        numNcdResultsToLookFor = numNcdResultsWithMinCriteria;
                    }
                    else
                    {
                        numNcdResultsToLookFor = param_numAgents * 2;
                    }
                    cout<<"Waiting for "<<numNcdResultsToLookFor<<" ncd calculation results..."<<endl;


                    // Wait for results....
                    while (jobManagerGetNcdCalcResultsQueueSize() < numNcdResultsToLookFor)
                    {
                        nanosleep(&sleepTime, NULL);
                    }

                    cout<<"Behavioral novelty comparisons with NCD complete, processing results."<<endl;



                    // Deal with the results now that they're all in....
                    int numNcdResultsToProcess = jobManagerGetNcdCalcResultsQueueSize();

                    // Setup novelty score vectors for use in Epoch
                    // The results aren't in order, but they have index
                    // references, so we can piece together the results
                    // in the correct order needed for the Genetic Algorithm
                    // Epoch method
                    double calculatedNoveltyScore;

                    for (int i = 0; i < numNcdResultsToProcess; i++)
                    {
                        if (!jobManagerGetNcdCalcResults(ncdResults))
                        {
                            cout<<"NCD results queue empty when it shouldn't be!"<<endl;
                            exit(1);
                        }

                        // Put the ncd scores in the right spot in the vector
                        if (ncdResults.team == REDTEAM)
                        {
                            redTeamNoveltyScores.erase(redTeamNoveltyScores.begin()  + ncdResults.robotIndex);
                            redTeamNoveltyScores.insert(redTeamNoveltyScores.begin() + ncdResults.robotIndex,
                                                        ncdResults.ncdScore);
                        }
                        else
                        {
                            blueTeamNoveltyScores.erase(blueTeamNoveltyScores.begin()  + ncdResults.robotIndex);
                            blueTeamNoveltyScores.insert(blueTeamNoveltyScores.begin() + ncdResults.robotIndex,
                                                        ncdResults.ncdScore);
                        }
                    }


                    // Setup vector of reference data that is sortable, to make
                    // handling of the archive stuff easier
                    // Blue and red teams have same number of agents, so
                    // can throw this into a single loop
                    for (unsigned int i = 0; i < redTeamNoveltyScores.size(); i++)
                    {
                        // Handle the red team...
                        noveltyRefData.robotIndex = i;
                        noveltyRefData.ncdScore   = redTeamNoveltyScores[i];

                        redTeamNoveltyRefVector.push_back(noveltyRefData);

                        if (redTeamNoveltyScores[i] > redTeamMaxNovelty)
                        {
                           redTeamMaxNovelty = redTeamNoveltyScores[i];
                        }

                        if (redTeamNoveltyScores[i] < redTeamMinNovelty)
                        {
                            redTeamMinNovelty = redTeamNoveltyScores[i];
                        }

                        redTeamMeanNovelty += redTeamNoveltyScores[i];

                        // Handle the blue team...
                        noveltyRefData.ncdScore = blueTeamNoveltyScores[i];
                        blueTeamNoveltyRefVector.push_back(noveltyRefData);


                        if (blueTeamNoveltyScores[i] > blueTeamMaxNovelty)
                        {
                            blueTeamMaxNovelty = blueTeamNoveltyScores[i];
                        }

                        if (blueTeamNoveltyScores[i] < blueTeamMinNovelty)
                        {
                            blueTeamMinNovelty = blueTeamNoveltyScores[i];
                        }

                        blueTeamMeanNovelty += blueTeamNoveltyScores[i];
                    }

                    // Figure out the mean...
                    redTeamMeanNovelty  /= (double)redTeamNoveltyScores.size();
                    blueTeamMeanNovelty /= (double)blueTeamNoveltyScores.size();


                    // Figure out the standard deviations
                    double deviation;
                    double redTeamSumSquaredDeviations  = 0.0;
                    double blueTeamSumSquaredDeviations = 0.0;

                    for (unsigned int i = 0; i < redTeamNoveltyScores.size(); i++)
                    {
                        deviation = redTeamNoveltyScores[i] - redTeamMeanNovelty;
                        redTeamSumSquaredDeviations += (deviation * deviation);

                        deviation = blueTeamNoveltyScores[i] - blueTeamMeanNovelty;
                        blueTeamSumSquaredDeviations += (deviation * deviation);
                    }

                    redTeamNoveltyStdDev  = sqrt(redTeamSumSquaredDeviations/(redTeamNoveltyScores.size()   - 1));
                    blueTeamNoveltyStdDev = sqrt(blueTeamSumSquaredDeviations/(blueTeamNoveltyScores.size() - 1));

                    // Now go back through and scale the novelty score values.
                    // A value of 1 should be the lowest amount of novelty.
                    for (unsigned int i = 0; i < redTeamNoveltyScores.size(); i++)
                    {
                        calculatedNoveltyScore  = redTeamNoveltyScores.at(i);
                        calculatedNoveltyScore  = (calculatedNoveltyScore - redTeamMinNovelty) * 100.0;
                        calculatedNoveltyScore += 1.0;
                        calculatedNoveltyScore  = pow(calculatedNoveltyScore, 2);

                        if (calculatedNoveltyScore > redTeamMaxAdjustedNovelty)
                        {
                            redTeamMaxAdjustedNovelty = calculatedNoveltyScore;
                        }

                        if (calculatedNoveltyScore < redTeamMinAdjustedNovelty)
                        {
                            redTeamMinAdjustedNovelty = calculatedNoveltyScore;
                        }

                        redTeamNoveltyScores.erase(redTeamNoveltyScores.begin() + i);
                        redTeamNoveltyScores.insert(redTeamNoveltyScores.begin() + i, calculatedNoveltyScore);
                    }

                    for (unsigned int i = 0; i < blueTeamNoveltyScores.size(); i++)
                    {
                        calculatedNoveltyScore  = blueTeamNoveltyScores.at(i);
                        calculatedNoveltyScore  = (calculatedNoveltyScore - blueTeamMinNovelty) * 100.0;
                        calculatedNoveltyScore += 1.0;
                        calculatedNoveltyScore  = pow(calculatedNoveltyScore, 2);

                        if (calculatedNoveltyScore > blueTeamMaxAdjustedNovelty)
                        {
                            blueTeamMaxAdjustedNovelty = calculatedNoveltyScore;
                        }

                        if (calculatedNoveltyScore < blueTeamMinAdjustedNovelty)
                        {
                            blueTeamMinAdjustedNovelty = calculatedNoveltyScore;
                        }

                        blueTeamNoveltyScores.erase(blueTeamNoveltyScores.begin() + i);
                        blueTeamNoveltyScores.insert(blueTeamNoveltyScores.begin() + i, calculatedNoveltyScore);
                    }


                    // If we're using minimal criteria, make sure we
                    // zero the novelty score of any agents who
                    // didn't pass the minimum criteria
                    if (UseMinimalCriteria)
                    {
                        for (unsigned int i = 0; i < redTeamNoveltyScores.size(); i++)
                        {
                            if (!redTeamMemberPassedMinCriteria.at(i))
                            {
                                redTeamNoveltyScores.erase(redTeamNoveltyScores.begin() + i);
                                redTeamNoveltyScores.insert(redTeamNoveltyScores.begin() + i, 0.0);
                            }
                        }

                        for (unsigned int i = 0; i < blueTeamNoveltyScores.size(); i++)
                        {
                            if (!blueTeamMemberPassedMinCriteria.at(i))
                            {
                                blueTeamNoveltyScores.erase(blueTeamNoveltyScores.begin() + i);
                                blueTeamNoveltyScores.insert(blueTeamNoveltyScores.begin() + i, 0.0);
                            }
                        }
                    }


                    cout<<"Red team max/min novelty score: ("<<redTeamMaxNovelty<<", "<<redTeamMinNovelty<<")"<<endl;
                    cout<<"Red team max/min novelty score adjusted: ("<<redTeamMaxAdjustedNovelty<<", "<<redTeamMinAdjustedNovelty<<")"<<endl;
                    cout<<"Red team novelty mean (unadjusted): "<<redTeamMeanNovelty<<", stdDev: "<<redTeamNoveltyStdDev<<endl;

                    cout<<"Blue team max/min novelty score: ("<<blueTeamMaxNovelty<<", "<<blueTeamMinNovelty<<")"<<endl;
                    cout<<"Blue team max/min novelty score adjusted: ("<<blueTeamMaxAdjustedNovelty<<", "<<blueTeamMinAdjustedNovelty<<")"<<endl;
                    cout<<"Blue team novelty mean (unadjusted): "<<blueTeamMeanNovelty<<", stdDev: "<<blueTeamNoveltyStdDev<<endl;

                    cout<<"Novelty search NCD evals done in: "<<noveltyEvalTimer.total()<<" seconds."<<endl;
                    cout<<"------------------------------------------------------------"<<endl;



                    // Release behavior data memory
                    // we don't need anymore this generation
                    // Need to go ahead and do this before
                    // we add in the new members of the
                    // archive.
                    clearBehaviorDataMemory();


                    // Now test the best agents to see if they should go into the archive....
                    cout<<"Starting novelty archive inclusion tests..."<<endl;

                    // Sort the vectors so we have easy access to the top
                    // param_numToBeTestedForArchive
                    // highest novelty scores
                    sort(redTeamNoveltyRefVector.begin(), redTeamNoveltyRefVector.end());
                    sort(blueTeamNoveltyRefVector.begin(), blueTeamNoveltyRefVector.end());

                    // Submit the top novelty agents to the NCD code
                    // for possible inclusion in the archive
                    // The ref vector is sorted lowest novelty to highest,
                    // so we want the last ones of the vector (highest novelty score)
                    int redTeamNewArchivedBehaviors  = 0;
                    int blueTeamNewArchivedBehaviors = 0;

                    int newArchiverGenomeID;

                    for (unsigned int i = redTeamNoveltyRefVector.size() - param_maxNewArchiveMembersPerGen;
                         i < redTeamNoveltyRefVector.size();
                         i++)
                    {
                        if (redTeamNoveltyRefVector[i].ncdScore >= param_archiveInclusionCutoff)
                        {
                            newArchiverGenomeID = redTeamRobots[0][redTeamNoveltyRefVector[i].robotIndex]->getBrainGenomeID();

                            addRobotToArchive(REDTEAM, newArchiverGenomeID);
                            redTeamGeneticsLab.AddGenomeToCustomArchive(redTeamNoveltyRefVector[i].robotIndex, newArchiverGenomeID);

                            redTeamNewArchivedBehaviors++;
                        }
                    }

                    for (unsigned int i = blueTeamNoveltyRefVector.size() - param_maxNewArchiveMembersPerGen;
                         i < blueTeamNoveltyRefVector.size();
                         i++)
                    {
                        if (blueTeamNoveltyRefVector[i].ncdScore >= param_archiveInclusionCutoff)
                        {
                            newArchiverGenomeID = blueTeamRobots[0][blueTeamNoveltyRefVector[i].robotIndex]->getBrainGenomeID();

                            addRobotToArchive(BLUETEAM, newArchiverGenomeID);
                            blueTeamGeneticsLab.AddGenomeToCustomArchive(blueTeamNoveltyRefVector[i].robotIndex, newArchiverGenomeID);

                            blueTeamNewArchivedBehaviors++;
                        }
                    }

                    cout<<"New behaviors added to archives are: red("<<redTeamNewArchivedBehaviors<<"), blue("<<blueTeamNewArchivedBehaviors<<")"<<endl;
                    cout<<"------------------------------------------------------------"<<endl;
                }







                /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                      EPOCH TIME!
                  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
                cout<<"----------EPOCH----------"<<endl;






                /*****************************************************
                // Standard Tournament/Playoff tournament, and Novelty
                // Search complete, move onto Epoch,
                // generation champ and dominance tournament, and then
                // the next generation
                ******************************************************/
                // We need to provide the genetic algorithm the scores
                // by which evolution will be driven.
                // If we're using novelty search, this will be the
                // novelty score. If not, then the fitness scores.
                // Going forward, for generation champ and
                // dominance tournament evals, we need the best
                // PERFORMING genomes, not the most novel.
                // So if we're NOT using novelty, we
                // can simply get this info from the
                // genetic algorithm. If we are using
                // novelty, this won't work, because it
                // would return the most novel genome. We
                // don't care about novelty post-Epoch.
                if (UseNoveltySearch)
                {
                    // Novelty search based experiment

                    blueChampNumNeurons          = blueTeamRobots[0][blueTeamChampIndex]->getNumberOfBrainNeurons();
                    blueChampFitnessScore        = blueTeamFitnessScores.at(blueTeamChampIndex);
                    blueChampNumBrainConnections = blueTeamRobots[0][blueTeamChampIndex]->getNumberOfBrainConnections();
                    blueChampNumBrainHiddenNodes = blueTeamRobots[0][blueTeamChampIndex]->getNumberOfBrainHiddenNodes();
                    blueChampGenomeID            = blueTeamRobots[0][blueTeamChampIndex]->getBrainGenomeID();

                    redChampNumNeurons           = redTeamRobots[0][redTeamChampIndex]->getNumberOfBrainNeurons();
                    redChampFitnessScore         = redTeamFitnessScores.at(redTeamChampIndex);
                    redChampNumBrainConnections  = redTeamRobots[0][redTeamChampIndex]->getNumberOfBrainConnections();
                    redChampNumBrainHiddenNodes  = redTeamRobots[0][redTeamChampIndex]->getNumberOfBrainHiddenNodes();
                    redChampGenomeID             = redTeamRobots[0][redTeamChampIndex]->getBrainGenomeID();

                    // Manually controlling the hall of fame in Novely Search mode
                    redTeamGeneticsLab.AddGenomeToHallOfFame(redTeamChampIndex, redChampGenomeID);
                    blueTeamGeneticsLab.AddGenomeToHallOfFame(blueTeamChampIndex, blueChampGenomeID);

                    // Now run epoch
                    redTeamGeneticsLab.Epoch(redTeamNoveltyScores);
                    blueTeamGeneticsLab.Epoch(blueTeamNoveltyScores);
                }
                else
                {
                    // Standard objective/fitness based experiment
                    // If we're using "random" evolution, provide
                    // random fitness scores to all genomes.
                    if (UseRandomEvolution)
                    {
                        vector <double> randomFitnessValues;

                        for (int i = 0; i < param_numAgents; i++)
                        {
                            randomFitnessValues.push_back(RandDouble() + 1.0);
                        }

                        redTeamGeneticsLab.Epoch(randomFitnessValues);
                        blueTeamGeneticsLab.Epoch(randomFitnessValues);
                    }
                    else // We're running for real, use actual fitness scores
                    {
                        redTeamGeneticsLab.Epoch(redTeamFitnessScores);
                        blueTeamGeneticsLab.Epoch(blueTeamFitnessScores);
                    }

                    CNeuralNet* redTeamChampBrain  = redTeamGeneticsLab.GetSingleBestPhenotypeFromLastGeneration();
                    CNeuralNet* blueTeamChampBrain = blueTeamGeneticsLab.GetSingleBestPhenotypeFromLastGeneration();

                    blueChampNumNeurons          = blueTeamChampBrain->getNumberOfNeurons();
                    blueChampFitnessScore        = blueTeamChampBrain->getRawFitness();
                    blueChampNumBrainConnections = blueTeamChampBrain->getNumberOfConnections();
                    blueChampNumBrainHiddenNodes = blueTeamChampBrain->getNumberOfHiddenNodes();
                    blueChampGenomeID            = blueTeamChampBrain->getGenomeID();

                    redChampNumNeurons           = redTeamChampBrain->getNumberOfNeurons();
                    redChampFitnessScore         = redTeamChampBrain->getRawFitness();
                    redChampNumBrainConnections  = redTeamChampBrain->getNumberOfConnections();
                    redChampNumBrainHiddenNodes  = redTeamChampBrain->getNumberOfHiddenNodes();
                    redChampGenomeID             = redTeamChampBrain->getGenomeID();
                }




                // If we're recording data, save the team champ DNA from their genomes
                if (recordData)
                {
                    stringstream fileNameSS;
                    string       fileName;

                    // Save the blue team champ
                    fileNameSS<<"./genomeFiles/blueTeamChamp_gen-"<<generationCounter<<"_genomeID-"<<blueChampGenomeID;
                    fileName = fileNameSS.str();

                    if (!blueTeamGeneticsLab.WriteGenomeToFile(blueChampGenomeID, fileName))
                    {
                        cout<<"Error saving blue champ genome dna to file!"<<endl;
                        DONE = true;
                        exit(1);
                    }

                    // Clear the stringstream, and save the red team champ now
                    fileNameSS.str("");
                    fileNameSS<<"./genomeFiles/redTeamChamp_gen-"<<generationCounter<<"_genomeID-"<<redChampGenomeID;
                    fileName = fileNameSS.str();

                    if (!redTeamGeneticsLab.WriteGenomeToFile(redChampGenomeID, fileName))
                    {
                        cout<<"Error saving red champ genome dna to file!"<<endl;
                        DONE = true;
                        exit(1);
                    }
                }





                cout<<"++++BLUE TEAM: avg num neurons ("<<blueTeamMeanNumNeurons<<"), avg num connections ("<<
                      blueTeamMeanNumConnections<<"), avg num hidden nodes ("<<blueTeamMeanHiddenNodes<<")"<<endl;
                cout<<"++BLUE champ genomeID is: "<<blueChampGenomeID<<", number of neurons: "<<blueChampNumNeurons<<endl;
                cout<<"++BLUE champ fitness score is: "<<blueChampFitnessScore<<" (out of 625 before playoff bonuses)"<<endl;
                cout<<"++BLUE champ num connections: "<<blueChampNumBrainConnections<<", num hidden nodes: "<<blueChampNumBrainHiddenNodes<<endl;
                cout<<"++BLUE team has "<<blueTeamGeneticsLab.GetNumberOfGenotypesSharingTopFitness()<<" genomes tied for team champ"<<endl;

                cout<<"----RED TEAM: avg num neurons ("<<redTeamMeanNumNeurons<<"), avg num connections ("<<
                      redTeamMeanNumConnections<<"), avg num hidden nodes ("<<redTeamMeanHiddenNodes<<")"<<endl;
                cout<<"--RED champ genomeID is: "<<redChampGenomeID<<", number of neurons: "<<redChampNumNeurons<<endl;
                cout<<"--RED champ fitness score is: "<<redChampFitnessScore<<" (out of 625 before playoff bonuses)"<<endl;
                cout<<"--RED champ num connections: "<<redChampNumBrainConnections<<", num hidden nodes: "<<redChampNumBrainHiddenNodes<<endl;
                cout<<"--RED team has "<<redTeamGeneticsLab.GetNumberOfGenotypesSharingTopFitness()<<" genomes tied for team champ"<<endl;

                cout<<"Post EPOC, number of species are RedTeam("<<redTeamGeneticsLab.NumSpecies()<<"), BlueTeam("<<blueTeamGeneticsLab.NumSpecies()<<")"<<endl;


                // Adjust the compatibility threshold for the populations
                // based on the number of species.
                double currentCompatThreshold;
                int    currentNumSpecies;

                currentCompatThreshold = redTeamGeneticsLab.GetCurrentCompatibilityThreshold();
                currentNumSpecies      = redTeamGeneticsLab.NumSpecies();

                if (currentNumSpecies < param_targetNumSpecies)
                {
                    // Adjust for more species
                    redTeamGeneticsLab.SetCompatibilityThreshold(currentCompatThreshold - compatibilityThresholdAdjustment);
                }
                else if (currentNumSpecies > param_targetNumSpecies)
                {
                    // Adjust for fewer species
                    redTeamGeneticsLab.SetCompatibilityThreshold(currentCompatThreshold + compatibilityThresholdAdjustment);
                }

                currentCompatThreshold = blueTeamGeneticsLab.GetCurrentCompatibilityThreshold();
                currentNumSpecies      = blueTeamGeneticsLab.NumSpecies();

                if (currentNumSpecies < param_targetNumSpecies)
                {
                    // Adjust for more species
                    blueTeamGeneticsLab.SetCompatibilityThreshold(currentCompatThreshold - compatibilityThresholdAdjustment);
                }
                else if (currentNumSpecies > param_targetNumSpecies)
                {
                    // Adjust for fewer species
                    blueTeamGeneticsLab.SetCompatibilityThreshold(currentCompatThreshold + compatibilityThresholdAdjustment);
                }
                // Done adjusting species compatibility thresholds




                // Insert the new brains for the general population
                vector <CNeuralNet*> newBrainsRedTeam;
                vector <CNeuralNet*> newBrainsBlueTeam;

                for (int i = 0; i < param_numExecutionThreads; i++)
                {
                    newBrainsRedTeam  = redTeamGeneticsLab.GetDuplicateBrainsForPopulation();
                    newBrainsBlueTeam = blueTeamGeneticsLab.GetDuplicateBrainsForPopulation();

                    for (int j = 0; j < param_numAgents; j++)
                    {
                        redTeamRobots[i][j]->insertNewBrain(newBrainsRedTeam[j]);
                        blueTeamRobots[i][j]->insertNewBrain(newBrainsBlueTeam[j]);
                    }
                }
                // End general population brain switch



                // Get the brains for the species champs
                vector <CNeuralNet*> redTeamSpeciesChampBrains;
                vector <CNeuralNet*> blueTeamSpeciesChampBrains;

                for (int i = 0; i < param_numExecutionThreads; i++)
                {
                    redTeamSpeciesChampBrains  = redTeamGeneticsLab.GetLeaderPhenotypesFromEachSpeciesDuplicate();
                    blueTeamSpeciesChampBrains = blueTeamGeneticsLab.GetLeaderPhenotypesFromEachSpeciesDuplicate();

                    for (int j = 0; j < param_numBestAgentsPerTeam; j++)
                    {
                        if (j < redTeamSpeciesChampBrains.size())
                        {
                            redTeamBestRobots[i][j]->insertNewBrain(redTeamSpeciesChampBrains[j]);
                        }
                        else
                        {
                            // Don't have enough species yet!
                            // Just give the extras a link to the first brain.
                            // The number of species will
                            // sort itself out after a few generations
                            redTeamBestRobots[i][j]->insertNewBrain(redTeamSpeciesChampBrains[0]);
                        }

                        if (j < blueTeamSpeciesChampBrains.size())
                        {
                            blueTeamBestRobots[i][j]->insertNewBrain(blueTeamSpeciesChampBrains[j]);
                        }
                        else
                        {
                            // Don't have enough species yet!
                            // Just give the extras a link to the first brain.
                            // The number of species will
                            // sort itself out after a few generations
                            blueTeamBestRobots[i][j]->insertNewBrain(blueTeamSpeciesChampBrains[0]);
                        }
                    }
                }
                // End species champ brain switch



                // Get the brains of the hall of famers
                vector <CNeuralNet*> redTeamHallOfFamerBrains;
                vector <CNeuralNet*> blueTeamHallOfFamerBrains;

                redTeamHallOfFameRobots.clear();
                blueTeamHallOfFameRobots.clear();

                redTeamHallOfFameRobots.resize(param_numExecutionThreads);
                blueTeamHallOfFameRobots.resize(param_numExecutionThreads);

                for (int i = 0; i < param_numExecutionThreads; i++)
                {
                    redTeamHallOfFamerBrains  = redTeamGeneticsLab.GetHallOfFamePhenotypesDuplicate();
                    blueTeamHallOfFamerBrains = blueTeamGeneticsLab.GetHallOfFamePhenotypesDuplicate();

                    for (int j = 0; j < redTeamHallOfFamerBrains.size(); j++)
                    {
                        redTeamHallOfFameRobots[i].push_back(new RobotAgent());
                        redTeamHallOfFameRobots[i][j]->insertNewBrain(redTeamHallOfFamerBrains[j]);
                    }

                    for (int j = 0; j < blueTeamHallOfFamerBrains.size(); j++)
                    {
                        blueTeamHallOfFameRobots[i].push_back(new RobotAgent());
                        blueTeamHallOfFameRobots[i][j]->insertNewBrain(blueTeamHallOfFamerBrains[j]);
                    }
                }
                // Done with hall of famers brain switch


                standardEvalTime = standardEvalTimer.total();
                cout<<"Generation "<<generationCounter<<" standard eval and epoch completed in "<<standardEvalTime<<" seconds"<<endl;



                /*****************************************************
                // Schedule the generation champion tournament, using
                // references to hall of fame vectors for consistency.
                // Last hall of fame entry for both teams will be the
                // team champ from the generation. These vectors aren't
                // setup until after Epoch.
                ******************************************************/
                cout<<"------------------------------------------------------------"<<endl;
                cout<<"Starting generation champ tournament..."<<endl;
                genChampTimer.reset();
                redChampTournyScore  = 0;
                blueChampTournyScore = 0;

                // Team champs will be the last robots added to the hall of fame,
                // so the number of hall-of-famers minus 1
                int teamChampIndex = blueTeamGeneticsLab.GetNumHallOfFamers() - 1;

                if (teamChampIndex != (redTeamGeneticsLab.GetNumHallOfFamers() - 1))
                {
                    cout<<"Error, in job manager, indexes of hall of famers should match between teams!"<<endl;
                    DONE = true;
                    exit(1);
                }

                setupGenerationChampJobs(teamChampIndex, teamChampIndex);


                // Check results from generation champion tournament
                while (jobManagerGetGenChampResultsQueueSize() < param_numTournamentResultRecords)
                {
                    nanosleep(&sleepTime, NULL);
                }

                generationChampEvalData genChampResultsData;
                hostTeamType genChampTeam;

                // We have all the generation champ evals done, check the results
                while (jobManagerGetGenChampResultsQueueSize() != 0)
                {
                    if (jobManagerGetGenChampResults(genChampResultsData))
                    {
                        redChampTournyScore  += genChampResultsData.redChampScore;
                        blueChampTournyScore += genChampResultsData.blueChampScore;
                    }
                }


                cout<<"At end of generation champ tourny, red champ score: "<<redChampTournyScore<<", blue champ score: "<<blueChampTournyScore<<endl;
                if (redChampTournyScore > blueChampTournyScore)
                {
                    cout<<"RED team has the generation champ."<<endl;
                    genChampTeam     = REDTEAM;
                    genTournyTied    = false;
                }
                else if (blueChampTournyScore > redChampTournyScore)
                {
                    cout<<"BLUE team has the generation champ."<<endl;
                    genChampTeam  = BLUETEAM;
                    genTournyTied = false;
                }
                else // Generation tourny tie handler
                {
                    // Well, crap. They tied. See if one had a higher fitness during
                    // normal evaluations
                    if (redChampFitnessScore > blueChampFitnessScore)
                    {
                        genChampTeam  = REDTEAM;
                        genTournyTied = true;
                        cout<<"TIE, picked RED team champ based on fitness"<<endl;
                    }
                    else if (redChampFitnessScore < blueChampFitnessScore)
                    {
                        genChampTeam  = BLUETEAM;
                        genTournyTied = true;
                        cout<<"TIE, picked BLUE team champ based on fitness"<<endl;

                    }
                    else
                    {
                        // Well, they tied there too. Pick one at random
                        double randFloat = RandDouble();
                        genTournyTied    = true;

                        if (randFloat < 0.5)
                        {
                            genChampTeam = REDTEAM;
                            cout<<"TIE, randomly picked RED team champ."<<endl;
                        }
                        else
                        {
                            genChampTeam = BLUETEAM;
                            cout<<"TIE, randomly picked BLUE team champ."<<endl;
                        }
                    }
                }

                switch (genChampTeam)
                {
                case REDTEAM:
                    numRedTeamGenChamps++;
                    break;

                case BLUETEAM:
                    numBlueTeamGenChamps++;
                    break;
                }

                generationChampEvalTime = genChampTimer.total();
                cout<<"Generation champ tourny completed in "<<generationChampEvalTime<<" seconds"<<endl;
                cout<<"------------------------------------------------------------"<<endl;



                /*****************************************************
                // Schedule the Dominance tournament
                ******************************************************/
                // setup dominance tournament...
                cout<<"Starting Dominance Tournament..."<<endl;
                dominanceTournamentTimer.reset();
                dominantStrategyReferenceData dominantStrategyData;
                dominanceTournamentEvalData   dominanceTournyResultsData;
                bool generationChampDefeated = false;

                // Reset some values
                whichDominantStratWon = 0;
                winningDomStratScore  = 0;
                losingChallengerScore = 0;

                // If this is the first generation, the first generation champ IS the first
                // dominant strategy. The generation counter has already been
                // incremented at this point, so subtract one.
                if (generationCounter == 1)
                {
                    dominantStrategyData.dominantStratTeam = genChampTeam;
                    dominantStrategyData.stratHofIndex     = teamChampIndex;
                    jobManagerAddDominantStrategy(dominantStrategyData);
                    newDominantStrat = true;

                    if (genChampTeam == REDTEAM)
                    {
                        numRedTeamDomStrats++;
                        cout<<"Adding first dominant strategy for first gen, RED team"<<endl;
                    }
                    else
                    {
                        numBlueTeamDomStrats++;
                        cout<<"Adding first dominant strategy for first gen, BLUE team"<<endl;
                    }
                }
                else
                {
                    // It's not the first generation, so we need to do dominance tournaments
                    //
                    // The way this works is that the generation champ competes against prior
                    // dominant strategies. If it defeats them *all* in a tournament, it gets
                    // added as the latest and greatest dominant strategy. If it loses any
                    // of the tournaments against a prior dominant strategy, we can skip the
                    // rest since it won't be a new dominant strategy

                    // First, make sure the generation champ isn't already
                    // listed as a dominant strategy. It's quite possible for a
                    // previous dominant strategy to remain in the population and
                    // make it to the dominance tournament again. Just skip the dominance
                    // tournament if the genome is already listed
                    int genChampGenomeID;

                    switch (genChampTeam)
                    {
                    case REDTEAM:
                        genChampGenomeID = redChampGenomeID;
                        break;

                    case BLUETEAM:
                        genChampGenomeID = blueChampGenomeID;
                        break;

                    default:
                        cout<<"Error in jobManager, dominance tournament setup, invalid genChampTeam."<<endl;
                        DONE = true;
                        exit(1);
                    }

                    // Check if the gen champ is a dominant strat already
                    if (!jobManagerCheckDominantStratsForGenome(genChampTeam, genChampGenomeID))
                    {
                        dominantStratTournySkipped = false;

                        // Generation champ is not already a dominant strategy, so run the tournament
                        for (int i = 0; i < jobManagerGetNumberOfDominantStrategies(); i++)
                        {
                            genChampTournyScore  = 0;
                            dominantStratScore   = 0;
                            jobManagerClearDominanceTournyResultsQueue();

                            // Schedule the tournament against a dominant strategy....
                            setupDominanceTournamentJobs(i, genChampTeam, teamChampIndex);


                            // Watch for tournament results, and if the generation
                            // champ won, continue onto the next dominant strategy.
                            while (jobManagerGetDominanceTournyResultsQueueSize() < param_numTournamentResultRecords)
                            {
                                nanosleep(&sleepTime, NULL);
                            }

                            while (jobManagerGetDominanceTournyResultsQueueSize() != 0)
                            {
                                if (jobManagerGetDominanceTournyResults(dominanceTournyResultsData))
                                {
                                    genChampTournyScore += dominanceTournyResultsData.genChampScore;
                                    dominantStratScore  += dominanceTournyResultsData.dominantStrategyScore;
                                }
                            }

                            cout<<"--Dominance tournament round "<<i+1<<" completed:"<<endl;
                            cout<<"----Generation champ score: "<<genChampTournyScore<<", Dominant Strat score: "<<dominantStratScore<<endl;

                            if (genChampTournyScore <= dominantStratScore)
                            {
                                // Generation champ lost a match, so it's not a new dominant strategy
                                cout<<"LOST: Generation champ lost a match in the dominance tournament, moving on..."<<endl;
                                generationChampDefeated = true;

                                whichDominantStratWon = i+1;
                                winningDomStratScore  = dominantStratScore;
                                losingChallengerScore = genChampTournyScore;
                                break;
                            }
                        }
                    }
                    else
                    {
                        cout<<"SKIPPING dominance tournament, generation champ is already listed as a dominant strategy."<<endl;
                        dominantStratTournySkipped = true;
                        generationChampDefeated    = true;
                        whichDominantStratWon      = 0;
                        winningDomStratScore       = 0;
                        losingChallengerScore      = 0;
                    }

                    if (!generationChampDefeated)
                    {
                        // Generation champ made it through defeating all
                        // prior dominanat strategies, therefore it becomes the
                        // latest and greatest dominant strategy
                        newDominantStrat = true;

                        if (genChampTeam == REDTEAM)
                        {
                            numRedTeamDomStrats++;
                            cout<<"VICTORY! New dominant strategy by RED team, hall of fame index: "<<teamChampIndex<<endl;
                        }
                        else
                        {
                            numBlueTeamDomStrats++;
                            cout<<"VICTORY! New dominant strategy by BLUE team, hall of fame index: "<<teamChampIndex<<endl;
                        }

                        lastGenWithNewDomStrat = generationCounter;
                        gensSinceNewDomStrat   = 0;

                        dominantStrategyData.dominantStratTeam = genChampTeam;
                        dominantStrategyData.stratHofIndex     = teamChampIndex;
                        jobManagerAddDominantStrategy(dominantStrategyData);
                    }
                    else
                    {
                        newDominantStrat = false;
                        gensSinceNewDomStrat = generationCounter - lastGenWithNewDomStrat;
                    }

                    cout<<"At end of tournaments, number of dominant strategies is: "<<jobManagerGetNumberOfDominantStrategies()<<endl;
                    cout<<"Number of generations since new dominant strategy: "<<gensSinceNewDomStrat<<endl;
                    dominanceTournyEvalTime = dominanceTournamentTimer.total();
                    cout<<"Dominance Tournament completed in "<<dominanceTournyEvalTime<<" seconds."<<endl;
                }


                // Get the brain stats for the "best" dominant strategy
                dominantStrategyReferenceData bestStratData;
                jobMangerGetLatestDominantStrategyRefData(bestStratData);

                if (bestStratData.dominantStratTeam == REDTEAM)
                {
                    lastDominantStratNumNeurons          =
                            redTeamHallOfFameRobots[0][bestStratData.stratHofIndex]->getNumberOfBrainNeurons();
                    lastDominantStratNumBrainConnections =
                            redTeamHallOfFameRobots[0][bestStratData.stratHofIndex]->getNumberOfBrainConnections();
                    lastDominantStratNumBrainHiddenNodes =
                            redTeamHallOfFameRobots[0][bestStratData.stratHofIndex]->getNumberOfBrainHiddenNodes();
                    dominantStratGenomeID                =
                            redTeamHallOfFameRobots[0][bestStratData.stratHofIndex]->getBrainGenomeID();
                }
                else
                {
                    lastDominantStratNumNeurons          =
                            blueTeamHallOfFameRobots[0][bestStratData.stratHofIndex]->getNumberOfBrainNeurons();
                    lastDominantStratNumBrainConnections =
                            blueTeamHallOfFameRobots[0][bestStratData.stratHofIndex]->getNumberOfBrainConnections();
                    lastDominantStratNumBrainHiddenNodes =
                            blueTeamHallOfFameRobots[0][bestStratData.stratHofIndex]->getNumberOfBrainHiddenNodes();
                    dominantStratGenomeID                =
                            blueTeamHallOfFameRobots[0][bestStratData.stratHofIndex]->getBrainGenomeID();
                }


                // If we're recording data, go ahead and save the dna of dominant strategies
                // to a file for later use/analysis
                if (recordData && newDominantStrat)
                {
                    stringstream fileNameSS;
                    string       fileName;


                    if (bestStratData.dominantStratTeam == REDTEAM)
                    {
                        fileNameSS<<"./genomeFiles/dominantStrat-"<<jobManagerGetNumberOfDominantStrategies()<<"_REDTEAM_genomeID-"<<dominantStratGenomeID;
                        fileName = fileNameSS.str();

                        if (!redTeamGeneticsLab.WriteGenomeToFile(dominantStratGenomeID, fileName))
                        {
                            cout<<"Error saving genome dna to file!"<<endl;
                            DONE = true;
                            exit(1);
                        }
                    }
                    else
                    {
                        fileNameSS<<"./genomeFiles/dominantStrat-"<<jobManagerGetNumberOfDominantStrategies()<<"_BLUETEAM_genomeID-"<<dominantStratGenomeID;
                        fileName = fileNameSS.str();

                        if (!blueTeamGeneticsLab.WriteGenomeToFile(dominantStratGenomeID, fileName))
                        {
                            cout<<"Error saving genome dna to file!"<<endl;
                            DONE = true;
                            exit(1);
                        }
                    }
                }

                cout<<"Best dominant strategy num Neurons: ("<<lastDominantStratNumNeurons<<"), num Connections: ("<<
                      lastDominantStratNumBrainConnections<<"), num Hidden Nodes: ("<<lastDominantStratNumBrainHiddenNodes<<")"<<endl;


                /****************************************************
                Tournaments completed, setup next generation
                *****************************************************/
                generationCounter++;
                jobManagerClearResultsQueue();
                jobManagerClearGenChampResultsQueue();

                generationEvalsComplete = true;

                // Network connections to client GUI is abandonware at the moment.
                // no need to call this anymore
                //                netServer->updateGenerationCount(generationCounter,
                //                                                 blueChampIndex,
                //                                                 blueTeamRobots[0][blueChampIndex]->getNumberOfBrainNeurons(),
                //                                                 blueTeamFitnessScores.at(blueChampIndex),
                //                                                 blueTeamGeneticsLab.BestEverFitness(),
                //                                                 blueTeamGeneticsLab.NumSpecies(),
                //                                                 redChampIndex,
                //                                                 redTeamRobots[0][redChampIndex]->getNumberOfBrainNeurons(),
                //                                                 redTeamFitnessScores.at(redChampIndex),
                //                                                 redTeamGeneticsLab.BestEverFitness(),
                //                                                 redTeamGeneticsLab.NumSpecies());


                generationTime = generationTimer.total();
                cout<<"Entire generation completed in "<<generationTime<<" seconds."<<endl;

                totalRunTime = totalTimeAllGenerations.total();

                // Check to see if we're recording data, and if so,
                // send the relevant data for writing
                if (recordData)
                {
                    generationRecordData genData;

                    genData.generation                       = generationCounter - 1;
                    genData.noveltySearchBool                = UseNoveltySearch;
                    genData.minimumCriteriaBool              = UseMinimalCriteria;
                    genData.staticStimulusBool               = UseStaticStimNovelty;
                    genData.staticStimExpandedComparisonBool = StaticStimExpandedComparisons;
                    genData.brainOutputOnlyBool              = UseBrainOutputOnly;
                    genData.randomSearchBool                 = UseRandomEvolution;
                    genData.playoffRequiredBool              = playoffRequired;
                    genData.numRedTeamInPlayoff              = redTeamNumTiedChamps;
                    genData.numBlueTeamInPlayoff             = blueTeamNumTiedChamps;
                    genData.numRedTeamPassedMinCriteria      = redTeamNumPassedMinCriteria;
                    genData.numBlueTeamPassedMinCriteria     = blueTeamNumPassedMinCriteria;
                    genData.blueTeamChampScore               = blueChampFitnessScore;
                    genData.blueTeamChampGenomeID            = blueChampGenomeID;
                    genData.blueTeamChampNumNeurons          = blueChampNumNeurons;
                    genData.blueTeamChampNumConnections      = blueChampNumBrainConnections;
                    genData.blueTeamChampNumHiddenNodes      = blueChampNumBrainHiddenNodes;
                    genData.blueTeamMeanNumNeurons           = blueTeamMeanNumNeurons;
                    genData.blueTeamMeanNumConnections       = blueTeamMeanNumConnections;
                    genData.blueTeamMeanNumHiddenNodes       = blueTeamMeanHiddenNodes;
                    genData.redTeamChampScore                = redChampFitnessScore;
                    genData.redTeamChampGenomeID             = redChampGenomeID;
                    genData.redTeamChampNumNeurons           = redChampNumNeurons;
                    genData.redTeamChampNumConnections       = redChampNumBrainConnections;
                    genData.redTeamChampNumHiddenNodes       = redChampNumBrainHiddenNodes;
                    genData.redTeamMeanNumNeurons            = redTeamMeanNumNeurons;
                    genData.redTeamMeanNumConnections        = redTeamMeanNumConnections;
                    genData.redTeamMeanNumHiddenNodes        = redTeamMeanHiddenNodes;
                    genData.blueTeamNumSpecies               = blueTeamGeneticsLab.NumSpecies();
                    genData.redTeamNumSpecies                = redTeamGeneticsLab.NumSpecies();
                    genData.blueChampGenTournyScore          = blueChampTournyScore;
                    genData.redChampGenTournyScore           = redChampTournyScore;
                    genData.wasGenTournyTied                 = genTournyTied;
                    genData.genChampTeam                     = genChampTeam;
                    genData.numBlueTeamGenChamps             = numBlueTeamGenChamps;
                    genData.numRedTeamGenChamps              = numRedTeamGenChamps;
                    genData.numBlueTeamDomStrats             = numBlueTeamDomStrats;
                    genData.numRedTeamDomStrats              = numRedTeamDomStrats;
                    genData.wasDomStratTournySkipped         = dominantStratTournySkipped;
                    genData.wasThereNewDominantStrat         = newDominantStrat;
                    genData.newDominantStratFromTeam         = genChampTeam;
                    genData.whichDominantStratWon            = whichDominantStratWon;
                    genData.winningDominantStratScore        = winningDomStratScore;
                    genData.losingGenChampChallengerScore    = losingChallengerScore;
                    genData.numDominantStrategies            = jobManagerGetNumberOfDominantStrategies();
                    genData.numGensSinceNewDominantStrat     = gensSinceNewDomStrat;
                    genData.bestDominantStratGenomeID        = dominantStratGenomeID;
                    genData.bestDominantStratNumNeurons      = lastDominantStratNumNeurons;
                    genData.bestDominantStratNumConnections  = lastDominantStratNumBrainConnections;
                    genData.bestDominantStratNumHiddenNodes  = lastDominantStratNumBrainHiddenNodes;
                    genData.standardEvalTime                 = standardEvalTime;
                    genData.playoffEvalTime                  = playoffEvalTime;
                    genData.genChampEvalTime                 = generationChampEvalTime;
                    genData.dominanceTournyEvalTime          = dominanceTournyEvalTime;
                    genData.generationTime                   = generationTime;
                    genData.totalTime                        = totalRunTime;

                    recordGenerationData(genData);
                }
            }

            nanosleep(&sleepTime, NULL);
        }

     //   nanosleep(&sleepTime, NULL);
    }

    cout<<endl;
    cout<<"Experiment complete, "<<generationCounter-1<<" generations completed in "<<totalTimeAllGenerations.total()<<" seconds."<<endl;
    cout<<endl;

    DONE = true;
    evaluationMode = stopEvals;

    sleep(2);
    qtConsoleAppPtr->quit();

    cout<<"Job manager thread exiting..."<<endl;
}








// Threads that actually execute the competitive matches
// 
// An execution thread should take a single host agent
// to run all evaluations on. The agent should be 
// evaluated against 12 opponents:
// 4 highest current species champions
// and the 8 hall-of-famers from
// all time. According to original 
// paper, each eval is performed twice
// with the host starting once on the 
// east side, and once on the west side
// for a total of 24 evaluations. 
//
// Evaluations last 750 steps, or ticks,
// or until someone wins. 1 point for 
// winning, 0 for losing or not
// winning within the time allowed
// Looks like the 750 ticks should
// equal about 30 second realtime
//
// *Note, changed to using the same
//  of Hall-of-Famers throughout
//  an entire generation. Have
//  jobManagerThread randomly
//  select the hall-of-famers to use,
//  instead of having each agent get
//  a different set of HOF'ers.
//
// Also runs scheduled jobs for
// playoff tournaments,
// generation champion evals, and
// dominance tournament evals,
// which are all similarly structured,
// but differ in who the agents are
// competing against.
//
// Note that only standard evaluations
// will use novelty search and NCD,
// as the results from the standard
// evaluations are what influence
// the genetic algorithms
//
// NCD calculations are also
// handled by the execution threads
// after the standard tournament is
// completed.
void *runExecutionThread(void *iValue)
{
    struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = nanoSleepAmount;

    struct timespec longSleepTime;
    longSleepTime.tv_sec  = 0;
    longSleepTime.tv_nsec = longSleepAmount;

    Timer evalTimer;
    Timer exeThreadRealTimeTimer;

    unsigned int leftEvalTicks  = 0;
    unsigned int rightEvalTicks = 0;

//    netServer->updateGenerationCount(generationCounter, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


    bool  evaluationDone     = false;
    bool  leftEvalDone       = false;
    bool  rightEvalDone      = false;

    int   leftEvalHostScore  = 0;
    int   rightEvalHostScore = 0;
    int   totalHostScore     = 0;

    int   redChampScore      = 0;
    int   blueChampScore     = 0;

    int   genChampScore      = 0;
    int   dominantStratScore = 0;



    // Queue of agents to evaluate during
    // normal generation evaluations
    queue <evaluationJobData> myGenerationJobQueue;


    // Queue of playoff tournamet eval jobs
    queue <playoffJobData> myPlayoffJobQueue;


    // Queue of generation champ evaluations
    // run after the normal generation
    // evaluations
    queue <generationChampJobData> myGenChampJobQueue;


    // Queue of dominance tournament evaluations
    queue <dominanceTournamentJobData> myDominanceTournyJobQueue;


    // Queue of ncd calculations to do
    queue <ncdJobData> myNcdCalcJobQueue;


    // Queue of archived behavior refresh matches to do
    queue <archiveBehaviorRefreshJobData> myArchBehaviorJobQueue;


    // Queue of oppenents to test against
    queue <RobotAgent*> enemiesQueue;


    // Since a host/parasite competition
    // has to evaluated from both starting
    // positions, go ahead and run them
    // serially
    bool                haveGameWorlds = false;
    GameWorld          *gameWorld;
    RobotAgent         *hostRobot;
    RobotAgent         *parasiteRobot;

    // For playoff and generation champ evals
    RobotAgent         *redChampRobot;
    RobotAgent         *blueChampRobot;

    // For dominance tournament evals
    RobotAgent         *genChampRobot;
    RobotAgent         *dominantStratRobot;

    // For Archived behavior refresh matches
    RobotAgent         *archivedBehaviorRobot;

    hostStartPosition   currentEvalSide;



    // Data types for getting and reporting
    // standard generation evaluations
    evaluationJobData   jobData;
    agentEvaluationData jobResults;
    agentBehaviorData   behaviorData;
    unsigned int        enemyIndex = 0;

    // Data types for getting and reporting
    // playoff evaluations
    playoffJobData  playoffMatchJobData;
    playoffEvalData playoffJobResults;


    // Data types for getting and reporting
    // generation champ evaluations
    generationChampJobData  genChampJobData;
    generationChampEvalData genChampJobResults;


    // Data types for getting and reporting
    // dominance tournament champ evaluations
    dominanceTournamentJobData    dominanceTournyJobData;
    dominanceTournamentEvalData   dominanceTournyJobResults;
    dominantStrategyReferenceData dominantStratRefData;


    // Data types for getting and reporting
    // NCD calculation jobs/evaluations
    ncdJobData  ncdCalcJobData;
    ncdEvalData ncdResults;


    // Data types for getting and reporting
    // archived behavior refresh matche
    // jobs/evaluations
    archiveBehaviorRefreshJobData     archBehaviorJobData;
    archiveBehaviorRefreshResultsData archBehaviorResults;
    archiveAgentBehaviorData          archBehaviorData;


    // Vector of neural network stimulus values to use
    // if we're in static stimulus novelty search
    // mode
    vector <brainInputData> staticBrainInputVector;
    bool   agentNeedsStaticStimulation = true;

    if (UseStaticStimNovelty)
    {
        if (!UseNoveltySearch)
        {
            cout<<"Error in execution thread setup, static stimulus is true, when not in novelty search mode"<<endl;
            DONE = true;
            exit(1);
        }

        staticBrainInputVector = getStaticStimulusVector();
    }


    // Data used for testing purposes, stores data
    // needed to render a match later.
    domTournyRenderingDataType renderData;



    // Each execution thread has it's own identifying thread number
    int threadNumber = *((int *)iValue);
    THREAD_READY     = true;



    // Can't get going until the
    // job manager has initialized
    // the shared memory and locking semaphore
    while (!sharedMemoryReady)
    {
        sleep(1);
    }

    cout<<"Excution thread number "<<threadNumber<<" is online and waiting for a job"<<endl;

    while (!DONE)
    {
        // We have different queues of jobs
        // depending on which kind of evaluation mode
        // we're in, standard evaluations, generation champ tourny,
        // or dominance tournament.
        switch (evaluationMode)
        {
        case standardEvaluation:
            while (myGenerationJobQueue.empty() && (evaluationMode == standardEvaluation))
            {
                // I don't have any more jobs
                // on my to-do list, get some
                // more
                for (int i = 0; i < param_numJobsToLoad; i++)
                {
                    if (exeThreadGetStandardJobToDo(jobData))
                    {
                        myGenerationJobQueue.push(jobData);
                    }
                    else
                    {
                        // Global job queue is empty,
                        // stop trying to get jobs to do
                        break;
                    }
                }
            }
            break;

        case playoffTournament:
            while (myPlayoffJobQueue.empty() && (evaluationMode == playoffTournament))
            {
                // I don't have any more jobs on my to-do list, get some more
                for (int i = 0; i < param_numJobsToLoad; i++)
                {
                    if (exeThreadGetPlayoffJobToDo(playoffMatchJobData))
                    {
                        myPlayoffJobQueue.push(playoffMatchJobData);
                    }
                    else
                    {
                        // Global job queue is empty,
                        // stop trying to get jobs to do
                        break;
                    }
                }
            }
            break;

        case generationChampTournament:
            while (myGenChampJobQueue.empty() && (evaluationMode == generationChampTournament))
            {
                // I don't have any more gen champ
                // jobs on my to-do list, get some more
                for (int i = 0; i < param_numJobsToLoad; i++)
                {
                    if (exeThreadGetGenChampJobToDo(genChampJobData))
                    {
                        myGenChampJobQueue.push(genChampJobData);
                    }
                    else
                    {
                        // Global gen champ job queue is
                        // empty, stop trying to get
                        // jobs to do
                        break;
                    }
                }
            }
            break;

        case dominanceTournament:
            while (myDominanceTournyJobQueue.empty() && (evaluationMode == dominanceTournament))
            {
                // I don't have any more dominance tournament jobs
                // on my to-do list, get some more
                for (int i = 0; i < param_numJobsToLoad; i++)
                {
                    if (exeThreadGetDominanceTournyJobsToDo(dominanceTournyJobData))
                    {
                        myDominanceTournyJobQueue.push(dominanceTournyJobData);
                    }
                    else
                    {
                        // Global dominance tourny job queue is
                        // empty, stop trying to get jobs to do
                        break;
                    }
                }
            }
            break;

        case ncdCalculations:
            while (myNcdCalcJobQueue.empty() && (evaluationMode == ncdCalculations))
            {
                // I don't have any more NCD calculations jobs
                // on my to-do list, get some more
                for (int i = 0; i < param_numJobsToLoad; i++)
                {
                    if (exeThreadGetNcdCalcJobToDo(ncdCalcJobData))
                    {
                        myNcdCalcJobQueue.push(ncdCalcJobData);
                    }
                    else
                    {
                        // Global ncd calc job queue is empty,
                        // stop trying to get jobs to do
                        break;
                    }
                }
            }
            break;

        case archiveBehaviorMatches:
            while (myArchBehaviorJobQueue.empty() && (evaluationMode == archiveBehaviorMatches))
            {
                // I don't have any more archived behavior refresh jobs
                // on my to-do list, get some more
                for (int i = 0; i < param_numJobsToLoad; i++)
                {
                    if (exeThreadGetArchBehaviorJobToDo(archBehaviorJobData))
                    {
                        myArchBehaviorJobQueue.push(archBehaviorJobData);
                    }
                    else
                    {
                        // Global archived behavior refresh job queue
                        // is empty, stop trying to get jobs to do
                        break;
                    }
                }
            }
            break;

        case stopEvals:
            continue;
            break;

        default:
            cout<<"Invalid evaluationMode in execution thread!"<<endl;
            DONE = true;
            exit(1);
        }





        /*************************************
          I have some jobs to do, go about
          crunching simulations....
         ***********************************/
        switch (evaluationMode)
        {
        case standardEvaluation:
            // This case statement handles standard
            // generation evaluation jobs
            if (myGenerationJobQueue.empty())
            {
                continue;
            }

            jobData = myGenerationJobQueue.front();
            myGenerationJobQueue.pop();

            // While robots will undergo many
            // competitions, if we're using
            // static stimualtion novelty search,
            // we only need to run the static
            // stimulation once per robot.
            // This helps us track when to
            // do that
            if (UseStaticStimNovelty)
            {
                agentNeedsStaticStimulation = true;
            }

            // 8 hall of famers, and 4 of the best from the other team
            // Need to randomly select hall of famers.
            // In initial generations, you won't have enough
            // species champs, or hall-of-famers to do a
            // complete evaluation.
            for (int i = 0; i < param_numBestAgentsPerTeam; i++)
            {
                if (jobData.hostTeam == REDTEAM)
                {
                    enemiesQueue.push(blueTeamBestRobots[threadNumber].at(i));
                }
                else
                {
                    enemiesQueue.push(redTeamBestRobots[threadNumber].at(i));
                }
            }


            // We're battling param_numHallOfFamers, picked
            // randomly from the vector of hall of famers
            // The random selection is handled by the jobManagerThread
            // in the setupEvaluationJobs function, so the same
            // hall of famers is used throughout a generation
            for (int i = 0; i < param_numHallOfFamers; i++)
            {
                if (jobData.hostTeam == REDTEAM)
                {
                    enemiesQueue.push(blueTeamHallOfFameRobots[threadNumber].at(exeThreadGetHallOfFamerIndex(i)));
                }
                else
                {
                    enemiesQueue.push(redTeamHallOfFameRobots[threadNumber].at(exeThreadGetHallOfFamerIndex(i)));
                }
            }


            if (UseNoveltySearch)
            {
                enemyIndex = 0;
            }

            while (!enemiesQueue.empty())
            {
                switch (jobData.hostTeam)
                {
                case REDTEAM:
                    hostRobot = redTeamRobots[threadNumber].at(jobData.hostRobotIndex);
                    break;

                case BLUETEAM:
                    hostRobot = blueTeamRobots[threadNumber].at(jobData.hostRobotIndex);
                    break;
                }

                parasiteRobot = enemiesQueue.front();
                enemiesQueue.pop();

                if (!haveGameWorlds)
                {
                    // We haven't instantiated game worlds yet,
                    // go ahead and do that
                    gameWorld       = new GameWorld(hostStartLeft, hostRobot, parasiteRobot, standardFoodLayout, 0, 0);
                    currentEvalSide = hostStartLeft;
                    haveGameWorlds = true;
                }
                else
                {
                    // We already have game worlds, just reset them
                    gameWorld->resetForNewRun(hostStartLeft, hostRobot, parasiteRobot, standardFoodLayout, 0, 0);
                    currentEvalSide = hostStartLeft;
                }


                leftEvalTicks      = 0;
                rightEvalTicks     = 0;
                leftEvalHostScore  = 0;
                rightEvalHostScore = 0;
                evaluationDone     = false;

                evalTimer.reset();
                exeThreadRealTimeTimer.reset();

                while (!evaluationDone)
                {
                    // Run the experimental evaluation

                    // If we're in realtime mode,
                    // slow things down to normal time
                    if (timeMode == realtime)
                    {
                        if (exeThreadRealTimeTimer.total() >= 0.04)
                        {
                            exeThreadRealTimeTimer.reset();
                        }
                        else
                        {
                            nanosleep(&sleepTime, NULL);
                            continue;
                        }
                    }

                    if (PAUSE)
                    {
                        nanosleep(&longSleepTime, NULL);
                        continue;
                    }


                    switch (currentEvalSide)
                    {
                    case hostStartLeft:
                        if (leftEvalTicks < param_numTicks)
                        {
                            if (gameWorld->update())
                            {
                                // Eval ended early
                                leftEvalHostScore = gameWorld->getHostFinalScore();
                                leftEvalDone      = true;
                                currentEvalSide   = hostStartRight;
                                gameWorld->resetForNewRun(hostStartRight, hostRobot, parasiteRobot, standardFoodLayout, 0, 0);
                            }
                            else if (UseNoveltySearch && (leftEvalTicks < param_ncdNumTickSamples))
                            {
                                if (!UseStaticStimNovelty)
                                {
                                    behaviorData.robotIndex                  = jobData.hostRobotIndex;
                                    behaviorData.robotTeam                   = jobData.hostTeam;
                                    behaviorData.robotStartSide              = LEFT;
                                    behaviorData.enemyIndex                  = enemyIndex;
                                    behaviorData.enemyGenomeID               = parasiteRobot->getBrainGenomeID();
                                    behaviorData.behaviorSnapshot.inputData  = gameWorld->getRobotSensorValues(HOST);
                                    behaviorData.behaviorSnapshot.outputData = gameWorld->getRobotOutputValues(HOST);
                                    reportAgentBehavior(behaviorData);
                                }
                            }
                        }
                        else
                        {
                            // Eval ended due to expiration of evaluation time
                            leftEvalHostScore = 0;
                            leftEvalDone      = true;
                            currentEvalSide   = hostStartRight;
                            gameWorld->resetForNewRun(hostStartRight, hostRobot, parasiteRobot, standardFoodLayout, 0, 0);
                        }

                        leftEvalTicks++;
                        break;

                    case hostStartRight:
                        if (rightEvalTicks < param_numTicks)
                        {
                            if (gameWorld->update())
                            {
                                // Eval ended early
                                rightEvalHostScore = gameWorld->getHostFinalScore();
                                rightEvalDone      = true;
                            }
                            else if (UseNoveltySearch && (rightEvalTicks < param_ncdNumTickSamples))
                            {
                                if (!UseStaticStimNovelty)
                                {
                                    behaviorData.robotIndex                  = jobData.hostRobotIndex;
                                    behaviorData.robotTeam                   = jobData.hostTeam;
                                    behaviorData.robotStartSide              = RIGHT;
                                    behaviorData.enemyIndex                  = enemyIndex;
                                    behaviorData.enemyGenomeID               = parasiteRobot->getBrainGenomeID();
                                    behaviorData.behaviorSnapshot.inputData  = gameWorld->getRobotSensorValues(HOST);
                                    behaviorData.behaviorSnapshot.outputData = gameWorld->getRobotOutputValues(HOST);
                                    reportAgentBehavior(behaviorData);
                                }
                            }
                        }
                        else
                        {
                            // Eval ended due to expiration of evaluation time
                            rightEvalHostScore = 0;
                            rightEvalDone      = true;
                        }

                        rightEvalTicks++;
                        break;

                    default:
                        cout<<"Invalid currentEvalSide in evaluation thread!"<<endl;
                        DONE = true;
                        exit(1);
                    }

                    if (leftEvalDone && rightEvalDone)
                    {
                        // Evals finished
                        totalHostScore += leftEvalHostScore + rightEvalHostScore;
                        evaluationDone = true;
                        leftEvalDone   = false;
                        rightEvalDone  = false;

                        // If we're doing novelty search, the behavior data for these
                        // matches needs to be serialized so it can be
                        // processed with the NCD algorithms
                        if (UseNoveltySearch)
                        {
                            if (UseStaticStimNovelty)
                            {
                                // If we haven't already tested this robot, go ahead
                                // and do the static stimulation of it's brain
                                if (agentNeedsStaticStimulation)
                                {
                                    // Loop through all the preset neural network inputs in
                                    // static stimulus mode, and report the NN outputs
                                    brainOutputData             brainOutput;
                                    agentStaticStimBehaviorData robotBehaviorData;

                                    robotBehaviorData.robotIndex = jobData.hostRobotIndex;
                                    robotBehaviorData.genomeID   = hostRobot->getBrainGenomeID();
                                    robotBehaviorData.robotTeam  = jobData.hostTeam;

                                    for (unsigned int i = 0; i < staticBrainInputVector.size(); i++)
                                    {
                                        hostRobot->setBrainInputs(staticBrainInputVector.at(i));
                                        hostRobot->update();
                                        hostRobot->getBrainOutputs(brainOutput);
                                        robotBehaviorData.brainOutputVector.push_back(brainOutput);
                                    }

                                    // Send and serialize data....
                                    reportAndSerializeAgentStaticStimBehavior(robotBehaviorData);
                                    agentNeedsStaticStimulation = false;
                                }
                            }
                            else // Standard dynamic match novelty search...
                            {
                                serializeAgentBehavior(jobData.hostTeam, jobData.hostRobotIndex,
                                                       enemyIndex, parasiteRobot->getBrainGenomeID(),
                                                       LEFT);

                                serializeAgentBehavior(jobData.hostTeam, jobData.hostRobotIndex,
                                                       enemyIndex, parasiteRobot->getBrainGenomeID(),
                                                       RIGHT);
                            }
                            enemyIndex++;
                        }
                    }
                }
            }

            // Evaluation done, cleanup, report back
            // and setup for another evaluation
            jobResults.hostTeam       = jobData.hostTeam;
            jobResults.hostRobotIndex = jobData.hostRobotIndex;
            jobResults.score          = totalHostScore;
            exeThreadReportJobResults(jobResults);

            evaluationDone = false;
            totalHostScore = 0;
            break;




        case playoffTournament:
            // This case statment handles
            // playoff tournament jobs
            if (myPlayoffJobQueue.empty())
            {
                continue;
            }

            playoffMatchJobData = myPlayoffJobQueue.front();
            myPlayoffJobQueue.pop();

            redChampRobot  = redTeamRobots[threadNumber].at(playoffMatchJobData.redTeamContestantRobotIndex);
            blueChampRobot = blueTeamRobots[threadNumber].at(playoffMatchJobData.blueTeamContestantRobotIndex);

            if (redChampRobot->getBrainGenomeID() != playoffMatchJobData.redTeamContestantGenomeId)
            {
                cout<<"Error in Playoff execution, planned genomeID doesn't match fetched robot genomeID for red team"<<endl;
                DONE = true;
                exit(1);
            }

            // Just double check to make sure the genome id is what it's supposed to be
            if (blueChampRobot->getBrainGenomeID() != playoffMatchJobData.blueTeamContestantGenomeId)
            {
                cout<<"Error in Playoff execution, planned genomeID doesn't match fetched robot genomeID for blue team"<<endl;
                DONE = true;
                exit(1);
            }

            // If we're in a playoff tournament, we already
            // have a game world instantiated. Simply reuse it.
            currentEvalSide = hostStartLeft;
            gameWorld->resetForNewRun(hostStartLeft, redChampRobot, blueChampRobot,
                                      playoffMatchJobData.foodLayout,
                                      playoffMatchJobData.firstExtraFoodPosIndex,
                                      playoffMatchJobData.secondExtraFoodPosIndex);

            leftEvalTicks  = 0;
            rightEvalTicks = 0;
            redChampScore  = 0;
            blueChampScore = 0;
            evaluationDone = false;

            while (!evaluationDone)
            {
                // Run the playoff tournament evaluation

                // If we're in realtime mode,
                // slow things down to normal time
                if (timeMode == realtime)
                {
                    if (exeThreadRealTimeTimer.total() >= 0.04)
                    {
                        exeThreadRealTimeTimer.reset();
                    }
                    else
                    {
                        nanosleep(&sleepTime, NULL);
                        continue;
                    }
                }

                if (PAUSE)
                {
                    nanosleep(&longSleepTime, NULL);
                    continue;
                }


                switch (currentEvalSide)
                {
                case hostStartLeft:
                    if (leftEvalTicks < param_numTicks)
                    {
                        if (gameWorld->update())
                        {
                            // Eval ended early
                            redChampScore   = gameWorld->getHostFinalScore();
                            blueChampScore  = gameWorld->getParasiteFinalScore();
                            leftEvalDone    = true;
                            currentEvalSide = hostStartRight;
                            gameWorld->resetForNewRun(hostStartRight, redChampRobot, blueChampRobot,
                                                      playoffMatchJobData.foodLayout,
                                                      playoffMatchJobData.firstExtraFoodPosIndex,
                                                      playoffMatchJobData.secondExtraFoodPosIndex);
                        }
                    }
                    else
                    {
                        // Eval ended due to expiration of evaluation time
                        redChampScore   = 0;
                        blueChampScore  = 0;
                        leftEvalDone    = true;
                        currentEvalSide = hostStartRight;
                        gameWorld->resetForNewRun(hostStartRight, redChampRobot, blueChampRobot,
                                                  playoffMatchJobData.foodLayout,
                                                  playoffMatchJobData.firstExtraFoodPosIndex,
                                                  playoffMatchJobData.secondExtraFoodPosIndex);
                    }

                    leftEvalTicks++;
                    break;

                case hostStartRight:
                    if (rightEvalTicks < param_numTicks)
                    {
                        if (gameWorld->update())
                        {
                            // Eval ended early
                            redChampScore  += gameWorld->getHostFinalScore();
                            blueChampScore += gameWorld->getParasiteFinalScore();
                            rightEvalDone   = true;
                        }
                    }
                    else
                    {
                        // Eval ended due to expiration of evaluation time
                        rightEvalDone = true;
                    }

                    rightEvalTicks++;
                    break;

                default:
                    cout<<"Invalid currentEvalSide in evaluation thread!"<<endl;
                    DONE = true;
                    exit(1);
                }

                if (leftEvalDone && rightEvalDone)
                {
                    // Evals finished
                    evaluationDone = true;
                    leftEvalDone   = false;
                    rightEvalDone  = false;
                }
            }

            // Report back the results
            playoffJobResults.redTeamContestantGenomeId   = playoffMatchJobData.redTeamContestantGenomeId;
            playoffJobResults.redTeamContestantRobotIndex = playoffMatchJobData.redTeamContestantRobotIndex;
            playoffJobResults.redTeamContestantDataIndex  = playoffMatchJobData.redTeamContestantDataIndex;
            playoffJobResults.redTeamContestantScore      = redChampScore;

            playoffJobResults.blueTeamContestantGenomeId   = playoffMatchJobData.blueTeamContestantGenomeId;
            playoffJobResults.blueTeamContestantRobotIndex = playoffMatchJobData.blueTeamContestantRobotIndex;
            playoffJobResults.blueTeamContestantDataIndex  = playoffMatchJobData.blueTeamContestantDataIndex;
            playoffJobResults.blueTeamContestantScore      = blueChampScore;

            exeThreadReportPlayoffResults(playoffJobResults);

            evaluationDone = false;
            redChampScore  = 0;
            blueChampScore = 0;
            break;


        case generationChampTournament:
            // This case statement handles generation
            // champ evaluation jobs
            if (myGenChampJobQueue.empty())
            {
                continue;
            }

            genChampJobData = myGenChampJobQueue.front();
            myGenChampJobQueue.pop();

            redChampRobot  = redTeamHallOfFameRobots[threadNumber].at(genChampJobData.redTeamChampIndex);
            blueChampRobot = blueTeamHallOfFameRobots[threadNumber].at(genChampJobData.blueTeamChampIndex);

            // If we're in a generation champ tourny,
            // we have a game world instantiated already. Simply reuse it.
            currentEvalSide = hostStartLeft;
            gameWorld->resetForNewRun(hostStartLeft, redChampRobot, blueChampRobot,
                                      genChampJobData.foodLayout, genChampJobData.firstExtraFoodPosIndex,
                                      genChampJobData.secondExtraFoodPosIndex);

            leftEvalTicks  = 0;
            rightEvalTicks = 0;
            redChampScore  = 0;
            blueChampScore = 0;
            evaluationDone = false;

            while (!evaluationDone)
            {
                // Run the generation tourny evaluation

                // If we're in realtime mode,
                // slow things down to normal time
                if (timeMode == realtime)
                {
                    if (exeThreadRealTimeTimer.total() >= 0.04)
                    {
                        exeThreadRealTimeTimer.reset();
                    }
                    else
                    {
                        nanosleep(&sleepTime, NULL);
                        continue;
                    }
                }

                if (PAUSE)
                {
                    nanosleep(&longSleepTime, NULL);
                    continue;
                }


                switch (currentEvalSide)
                {
                case hostStartLeft:
                    if (leftEvalTicks < param_numTicks)
                    {
                        if (gameWorld->update())
                        {
                            // Eval ended early
                            redChampScore   = gameWorld->getHostFinalScore();
                            blueChampScore  = gameWorld->getParasiteFinalScore();
                            leftEvalDone    = true;
                            currentEvalSide = hostStartRight;
                            gameWorld->resetForNewRun(hostStartRight, redChampRobot, blueChampRobot,
                                                      genChampJobData.foodLayout, genChampJobData.firstExtraFoodPosIndex,
                                                      genChampJobData.secondExtraFoodPosIndex);
                        }
                    }
                    else
                    {
                        // Eval ended due to expiration of evaluation time
                        redChampScore   = 0;
                        blueChampScore  = 0;
                        leftEvalDone    = true;
                        currentEvalSide = hostStartRight;
                        gameWorld->resetForNewRun(hostStartRight, redChampRobot, blueChampRobot,
                                                  genChampJobData.foodLayout, genChampJobData.firstExtraFoodPosIndex,
                                                  genChampJobData.secondExtraFoodPosIndex);
                    }

                    leftEvalTicks++;
                    break;

                case hostStartRight:
                    if (rightEvalTicks < param_numTicks)
                    {
                        if (gameWorld->update())
                        {
                            // Eval ended early
                            redChampScore  += gameWorld->getHostFinalScore();
                            blueChampScore += gameWorld->getParasiteFinalScore();
                            rightEvalDone   = true;
                        }
                    }
                    else
                    {
                        // Eval ended due to expiration of evaluation time
                        rightEvalDone = true;
                    }

                    rightEvalTicks++;
                    break;

                default:
                    cout<<"Invalid currentEvalSide in evaluation thread!"<<endl;
                    DONE = true;
                    exit(1);
                }

                if (leftEvalDone && rightEvalDone)
                {
                    // Evals finished
                    evaluationDone = true;
                    leftEvalDone   = false;
                    rightEvalDone  = false;
                }
            }

            // Report back the results
            genChampJobResults.redChampScore  = redChampScore;
            genChampJobResults.blueChampScore = blueChampScore;
            exeThreadReportGenChampResults(genChampJobResults);

            evaluationDone = false;
            redChampScore  = 0;
            blueChampScore = 0;
            break;




        case dominanceTournament:
            // This case statement handles dominance tournament
            // evaluation jobs
            if (myDominanceTournyJobQueue.empty())
            {
                continue;
            }

            dominanceTournyJobData = myDominanceTournyJobQueue.front();
            myDominanceTournyJobQueue.pop();

            if (dominanceTournyJobData.genChampTeam == REDTEAM)
            {
                genChampRobot = redTeamHallOfFameRobots[threadNumber].at(dominanceTournyJobData.genChampIndex);
            }
            else
            {
                genChampRobot = blueTeamHallOfFameRobots[threadNumber].at(dominanceTournyJobData.genChampIndex);
            }


            // Get the reference data for the dominant strategy to use in the eval
            exeThreadGetDominantStrategyRefData(dominanceTournyJobData.dominanceStrategyIndex,
                                                dominantStratRefData);

            if (dominantStratRefData.dominantStratTeam == REDTEAM)
            {
                dominantStratRobot = redTeamHallOfFameRobots[threadNumber].at(dominantStratRefData.stratHofIndex);
            }
            else
            {
                dominantStratRobot = blueTeamHallOfFameRobots[threadNumber].at(dominantStratRefData.stratHofIndex);
            }


            // If we're in a dominance tournament, we already have a game world instantiated, simply reuse it.
            currentEvalSide = hostStartLeft;
            gameWorld->resetForNewRun(hostStartLeft, genChampRobot, dominantStratRobot,
                                      dominanceTournyJobData.foodLayout,
                                      dominanceTournyJobData.firstExtraFoodPosIndex,
                                      dominanceTournyJobData.secondExtraFoodPosIndex);

            leftEvalTicks      = 0;
            rightEvalTicks     = 0;
            genChampScore      = 0;
            dominantStratScore = 0;
            evaluationDone     = false;

            while (!evaluationDone)
            {
                // Run the dominance tourny evaluation

                // If we're in realtime mode,
                // slow things down to normal time
                if (timeMode == realtime)
                {
                    if (exeThreadRealTimeTimer.total() >= 0.04)
                    {
                        exeThreadRealTimeTimer.reset();
                    }
                    else
                    {
                        nanosleep(&sleepTime, NULL);
                        continue;
                    }
                }

                if (PAUSE)
                {
                    nanosleep(&longSleepTime, NULL);
                    continue;
                }


                switch (currentEvalSide)
                {
                case hostStartLeft:
                    if (leftEvalTicks < param_numTicks)
                    {
                        if (gameWorld->update())
                        {
                            // Evaluation ended early
                            genChampScore      = gameWorld->getHostFinalScore();
                            dominantStratScore = gameWorld->getParasiteFinalScore();

                            leftEvalDone       = true;
                            currentEvalSide    = hostStartRight;
                            gameWorld->resetForNewRun(hostStartRight, genChampRobot, dominantStratRobot,
                                                      dominanceTournyJobData.foodLayout,
                                                      dominanceTournyJobData.firstExtraFoodPosIndex,
                                                      dominanceTournyJobData.secondExtraFoodPosIndex);
                        }
                        else if (recordDomTournyRenderingData) // testing BS code
                        {
                            // This is testing code
                            renderData.renderGeneration                = generationCounter;
                            renderData.renderTick                      = leftEvalTicks;

                            renderData.renderGenChampScore             = 0;
                            renderData.renderDominantStratScore        = 0;

                            renderData.renderGenChampGenomeID          = genChampRobot->getBrainGenomeID();
                            renderData.renderDominantStratGenomeID     = dominantStratRobot->getBrainGenomeID();

                            renderData.renderGenomeChampPosData        = gameWorld->getHostPosition();
                            renderData.renderDominantStratPosData      = gameWorld->getParasitePosition();

                            renderData.renderDominantStratNumber       = dominanceTournyJobData.dominanceStrategyIndex;

                            renderData.renderGenChampNumFoodEaten      = gameWorld->getNumFoodEaten(HOST);
                            renderData.renderDominantStratNumFoodEaten = gameWorld->getNumFoodEaten(PARASITE);
                            renderData.renderNumFoodPieces             = gameWorld->getNumFoodPieces();

                            for (int i = 0; i < renderData.renderNumFoodPieces; i++)
                            {
                                renderData.renderFoodData[i] = gameWorld->getFoodData(i);
                            }

                            renderData.renderGenChampEnergy            = gameWorld->getRobotEnergy(HOST);
                            renderData.renderDominantStratEnergy       = gameWorld->getRobotEnergy(PARASITE);

                            renderData.renderGenChampSensorData        = gameWorld->getRobotSensorValues(HOST);
                            renderData.renderDominantStratSensorData   = gameWorld->getRobotSensorValues(PARASITE);

                            recordRenderingDataForDomTourny(renderData);
                        }
                    }
                    else
                    {
                        // Eval ended due to expiration of evaluation time
                        genChampScore      = 0;
                        dominantStratScore = 0;
                        leftEvalDone       = true;
                        currentEvalSide    = hostStartRight;
                        gameWorld->resetForNewRun(hostStartRight, genChampRobot, dominantStratRobot,
                                                  dominanceTournyJobData.foodLayout,
                                                  dominanceTournyJobData.firstExtraFoodPosIndex,
                                                  dominanceTournyJobData.secondExtraFoodPosIndex);
                    }

                    leftEvalTicks++;
                    break;

                case hostStartRight:
                    if (rightEvalTicks < param_numTicks)
                    {
                        if (gameWorld->update())
                        {
                            // Eval ended early
                            genChampScore      += gameWorld->getHostFinalScore();
                            dominantStratScore += gameWorld->getParasiteFinalScore();

                            rightEvalDone       = true;
                        }
                        else if (recordDomTournyRenderingData) // testing BS code
                        {
                            // This is testing code
                            renderData.renderGeneration                = generationCounter;
                            renderData.renderTick                      = rightEvalTicks;

                            renderData.renderGenChampScore             = 0;
                            renderData.renderDominantStratScore        = 0;

                            renderData.renderGenChampGenomeID          = genChampRobot->getBrainGenomeID();
                            renderData.renderDominantStratGenomeID     = dominantStratRobot->getBrainGenomeID();

                            renderData.renderGenomeChampPosData        = gameWorld->getHostPosition();
                            renderData.renderDominantStratPosData      = gameWorld->getParasitePosition();

                            renderData.renderDominantStratNumber       = dominanceTournyJobData.dominanceStrategyIndex;

                            renderData.renderGenChampNumFoodEaten      = gameWorld->getNumFoodEaten(HOST);
                            renderData.renderDominantStratNumFoodEaten = gameWorld->getNumFoodEaten(PARASITE);
                            renderData.renderNumFoodPieces             = gameWorld->getNumFoodPieces();

                            for (int i = 0; i < renderData.renderNumFoodPieces; i++)
                            {
                                renderData.renderFoodData[i] = gameWorld->getFoodData(i);
                            }

                            renderData.renderGenChampEnergy            = gameWorld->getRobotEnergy(HOST);
                            renderData.renderDominantStratEnergy       = gameWorld->getRobotEnergy(PARASITE);

                            renderData.renderGenChampSensorData        = gameWorld->getRobotSensorValues(HOST);
                            renderData.renderDominantStratSensorData   = gameWorld->getRobotSensorValues(PARASITE);

                            recordRenderingDataForDomTourny(renderData);
                        }
                    }
                    else
                    {
                        rightEvalDone = true;
                    }

                    rightEvalTicks++;
                    break;

                default:
                    cout<<"Invalid currentEvalSide in evaluation thread!"<<endl;
                    DONE = true;
                    exit(1);
                }

                if (leftEvalDone && rightEvalDone)
                {
                    // Evals are finished
                    evaluationDone = true;
                    leftEvalDone   = false;
                    rightEvalDone  = false;
                }
            }

            // Report back the results
            dominanceTournyJobResults.genChampScore         = genChampScore;
            dominanceTournyJobResults.dominantStrategyScore = dominantStratScore;
            exeThreadReportDominanceTournyResults(dominanceTournyJobResults);

            // This is testing code
            if (recordDomTournyRenderingData)
            {
                renderData.renderGenChampScore      = genChampScore;
                renderData.renderDominantStratScore = dominantStratScore;

                recordRenderingDataForDomTourny(renderData);
            }


            evaluationDone     = false;
            genChampScore      = 0;
            dominantStratScore = 0;
            break;


        case ncdCalculations:
            // This case statement handles
            // calculating NCD scores
            if (myNcdCalcJobQueue.empty())
            {
                continue;
            }

            // Get a job to do
            ncdCalcJobData = myNcdCalcJobQueue.front();
            myNcdCalcJobQueue.pop();


            // Get the NCD value for this robot. The actual
            // handling of who to compare the behavior data to
            // is contained in the calculateNcdScoreForRobot
            // function in ncdBehaviorFuncs.cpp
            ncdResults.ncdScore   = calculateNcdScoreForRobot(ncdCalcJobData.team, ncdCalcJobData.robotIndex);

            ncdResults.team       = ncdCalcJobData.team;
            ncdResults.robotIndex = ncdCalcJobData.robotIndex;

            // Report back the results
            exeThreadReportNcdCalculations(ncdResults);
            break;




        case archiveBehaviorMatches:
            // This case statement handles the matches
            // for archived behaviors
            if (myArchBehaviorJobQueue.empty())
            {
                continue;
            }

            // We shouldn't be here if we're not running novelty search version of competitive coevolution
            if (!UseNoveltySearch)
            {
                cout<<"Error in execution threads, running archived behavior matches, when not in novelty search mode."<<endl;
                DONE = true;
                exit(1);
            }

            // Get a job to do
            archBehaviorJobData = myArchBehaviorJobQueue.front();
            myArchBehaviorJobQueue.pop();


            // The archived behavior will compete
            // against a single parasite from
            // the list of 12 that robots fight
            // in standard tournaments.
            switch (archBehaviorJobData.team)
            {
            case REDTEAM:
                archivedBehaviorRobot = redTeamNoveltyArchiveRobots[threadNumber].at(archBehaviorJobData.index);
                parasiteRobot         = blueTeamBestRobots[threadNumber].at(0);
                break;

            case BLUETEAM:
                archivedBehaviorRobot = blueTeamNoveltyArchiveRobots[threadNumber].at(archBehaviorJobData.index);
                parasiteRobot         = redTeamBestRobots[threadNumber].at(0);
                break;
            }

            // We already have a gameworld instantiated at this point, so just reuse it
            gameWorld->resetForNewRun(hostStartLeft, archivedBehaviorRobot, parasiteRobot, standardFoodLayout, 0, 0);

            leftEvalTicks  = 0;
            evaluationDone = false;
            exeThreadRealTimeTimer.reset();

            while (!evaluationDone)
            {
                // Run the match
                // If we're in realtime mode,
                // slow things down to normal time
                if (timeMode == realtime)
                {
                    if (exeThreadRealTimeTimer.total() >= 0.04)
                    {
                        exeThreadRealTimeTimer.reset();
                    }
                    else
                    {
                        nanosleep(&sleepTime, NULL);
                        continue;
                    }
                }

                if (PAUSE)
                {
                    nanosleep(&longSleepTime, NULL);
                    continue;
                }

                // We're only running left side start for
                // these matches, so we don't have to worry
                // about the currentEvalSide variable
                if (leftEvalTicks < param_numTicks)
                {
                    if (gameWorld->update())
                    {
                        // Eval ended early
                        evaluationDone = true;
                    }
                    else if (UseNoveltySearch && (leftEvalTicks < param_ncdNumTickSamples))
                    {
                        archBehaviorData.robotIndex = archBehaviorJobData.index;
                        archBehaviorData.robotTeam  = archBehaviorJobData.team;
                        archBehaviorData.genomeID   = archivedBehaviorRobot->getBrainGenomeID();

                        archBehaviorData.behaviorSnapshot.inputData  = gameWorld->getRobotSensorValues(HOST);
                        archBehaviorData.behaviorSnapshot.outputData = gameWorld->getRobotOutputValues(HOST);

                        reportArchivedAgentBehavior(archBehaviorData);
                    }
                }
                else
                {
                    // Eval ended due to expiration of evaluation time
                    evaluationDone = true;
                }

                leftEvalTicks++;

                if (evaluationDone)
                {
                    // serialize the behavior data...
                    serializeArchivedAgentBehavior(archBehaviorJobData.team, archBehaviorJobData.index, archivedBehaviorRobot->getBrainGenomeID());

                    // Report that this eval is complete to the job manager thread
                    archBehaviorResults.index = archBehaviorJobData.index;
                    archBehaviorResults.team  = archBehaviorJobData.team;
                    archBehaviorResults.done  = true;

                    exeThreadReportArchBehaviorMatch(archBehaviorResults);
                }
            }
            break;


        case stopEvals:
            continue;
            break;


        default:
            cout<<"Invalid evaluationMode in execution thread!"<<endl;
            DONE = true;
            exit(1);
        }
    }

    // Execution thread is completed
    delete gameWorld;
    cout<<"Execution thread "<<threadNumber<<" exiting..."<<endl;
}







/**********************************************************************
             Main functions
*********************************************************************/



// Handle signals
void signalHandler(int signal)
{
    switch (signal)
    {
    case SIGINT: // Time to exit
        cout<<"Caught a SIGINT!"<<endl;
        evaluationMode = stopEvals;
        DONE           = true;

        sleep(2);
        qtConsoleAppPtr->quit();
        break;

    default:
        // Other signals...
        break;
    }
}




void printProgramUsage()
{
    cout<<"Call program with no arguments for running objective NEAT without data recording, or with 1 arguments (datafile name) for data recording"<<endl;
    cout<<"If you want to use Novelty Search call the program like: "<<endl;
    cout<<"competitiveCoevolution [NOVELTY_MODE] [dataFileName for recording, optional]"<<endl;
    cout<<"Where NOVELTY_MODE is one of the following:"<<endl;
    cout<<"NOVELTY - Novelty search, uses both neural network inputs and outputs as behavior data"<<endl;
    cout<<"MCNOVELTY - Minimal Criteria Novelty search"<<endl;
    cout<<"NOVELTY_OUTPUTS - Novelty search with neural network outputs only as behavior data"<<endl;
    cout<<"MCNOVELTY_OUTPUTS - Minimal Criteria Novelty search with neural network outputs only as behavior data"<<endl;
    cout<<"NOVELTY_STATICSTIM - Novelty search with static stimulation for neural network behavior data"<<endl;
    cout<<"MCNOVELTY_STATICSTIM - Minimal Criteria Novelty search with static stimulation for neural network behavior data"<<endl;
    cout<<"NOVELTY_STATICSTIM_EC - Novelty search with static stimulation and extended comparisons for neural network behavior data"<<endl;
    cout<<"MCNOVELTY_STATICSTIM_EC - Minimal Criteria Novelty search with static stimulation and extended comparisons for neural network behavior data"<<endl;
    cout<<endl;
    cout<<"For random evolution, strictly for comparison, pass RANDOM in for NOVELTY_MODE"<<endl;
}




// Parses command line arguments passed to
// the program on startup, like for data
// collection, or whether or not to use
// novelty search
void parseArguments(int argc, char *argv[])
{
    switch (argc)
    {
    case 1:
        // default mode, no data collection
        UseNoveltySearch     = false;
        UseMinimalCriteria   = false;
        UseStaticStimNovelty = false;
        UseBrainOutputOnly   = false;

        recordData = false;
        break;

    case 2:
        // Looks like arguments for data collection
        if (!strcmp(argv[1],"NOVELTY"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = false;
            UseStaticStimNovelty = false;
            UseBrainOutputOnly   = false;

            recordData           = false;
        }
        else if (!strcmp(argv[1],"MCNOVELTY"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = true;
            UseStaticStimNovelty = false;
            UseBrainOutputOnly   = false;

            recordData           = false;
        }
        else if (!strcmp(argv[1], "NOVELTY_OUTPUTS"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = false;
            UseStaticStimNovelty = false;
            UseBrainOutputOnly   = true;

            recordData           = false;
        }
        else if (!strcmp(argv[1], "MCNOVELTY_OUTPUTS"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = true;
            UseStaticStimNovelty = false;
            UseBrainOutputOnly   = true;

            recordData           = false;
        }
        else if (!strcmp(argv[1], "NOVELTY_STATICSTIM"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = false;
            UseStaticStimNovelty = true;
            UseBrainOutputOnly   = false;

            recordData           = false;
        }
        else if (!strcmp(argv[1], "MCNOVELTY_STATICSTIM"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = true;
            UseStaticStimNovelty = true;
            UseBrainOutputOnly   = false;

            recordData           = false;
        }
        else if (!strcmp(argv[1], "NOVELTY_STATICSTIM_EC"))
        {
            UseNoveltySearch              = true;
            UseMinimalCriteria            = false;
            UseStaticStimNovelty          = true;
            StaticStimExpandedComparisons = true;
            UseBrainOutputOnly            = false;

            recordData                    = false;
        }
        else if (!strcmp(argv[1], "MCNOVELTY_STATICSTIM_EC"))
        {
            UseNoveltySearch              = true;
            UseMinimalCriteria            = true;
            UseStaticStimNovelty          = true;
            StaticStimExpandedComparisons = true;
            UseBrainOutputOnly            = false;

            recordData                    = false;
        }
        else if (!strcmp(argv[1], "RANDOM"))
        {
            UseNoveltySearch              = false;
            UseMinimalCriteria            = false;
            UseStaticStimNovelty          = false;
            StaticStimExpandedComparisons = false;
            UseBrainOutputOnly            = false;
            UseRandomEvolution            = true;

            recordData                    = false;
        }
        else if (!strcmp(argv[1], "help"))
        {
            printProgramUsage();
            exit(0);
        }
        else // argument should be the datafile to record data to
        {
            dataFileName = argv[1];
            recordData   = true;
        }
        break;

    case 3:
        if (!strcmp(argv[1], "NOVELTY"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = false;
            UseStaticStimNovelty = false;
            UseBrainOutputOnly   = false;
        }
        else if (!strcmp(argv[1],"MCNOVELTY"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = true;
            UseStaticStimNovelty = false;
            UseBrainOutputOnly   = false;
        }
        else if (!strcmp(argv[1],"NOVELTY_OUTPUTS"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = false;
            UseStaticStimNovelty = false;
            UseBrainOutputOnly   = true;
        }
        else if (!strcmp(argv[1],"MCNOVELTY_OUTPUTS"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = true;
            UseStaticStimNovelty = false;
            UseBrainOutputOnly   = true;
        }
        else if (!strcmp(argv[1],"NOVELTY_STATICSTIM"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = false;
            UseStaticStimNovelty = true;
            UseBrainOutputOnly   = false;
        }
        else if (!strcmp(argv[1],"MCNOVELTY_STATICSTIM"))
        {
            UseNoveltySearch     = true;
            UseMinimalCriteria   = true;
            UseStaticStimNovelty = true;
            UseBrainOutputOnly   = false;
        }
        else if (!strcmp(argv[1], "NOVELTY_STATICSTIM_EC"))
        {
            UseNoveltySearch              = true;
            UseMinimalCriteria            = false;
            UseStaticStimNovelty          = true;
            StaticStimExpandedComparisons = true;
            UseBrainOutputOnly            = false;
        }
        else if (!strcmp(argv[1], "MCNOVELTY_STATICSTIM_EC"))
        {
            UseNoveltySearch              = true;
            UseMinimalCriteria            = true;
            UseStaticStimNovelty          = true;
            StaticStimExpandedComparisons = true;
            UseBrainOutputOnly            = false;
        }
        else if (!strcmp(argv[1], "RANDOM"))
        {
            UseNoveltySearch              = false;
            UseMinimalCriteria            = false;
            UseStaticStimNovelty          = false;
            StaticStimExpandedComparisons = false;
            UseBrainOutputOnly            = false;
            UseRandomEvolution            = true;
        }
        else
        {
            cout<<"Error parsing arguments."<<endl;
            printProgramUsage();
            exit(1);
        }

        dataFileName = argv[2];
        recordData   = true;
        break;

    default:
        printProgramUsage();
        exit(1);
    }


    // Print out the selections
    if (recordData)
    {
        cout<<"Starting with data recording, recording to file: "<<dataFileName + dataFileExtension<<endl;
    }
    else
    {
        cout<<"Starting without data recording"<<endl;
    }

    if (UseNoveltySearch)
    {
        if (UseMinimalCriteria)
        {
            cout<<"Using Minimal Criteria Novelty Search for genome scoring."<<endl;
        }
        else
        {
            cout<<"Using Novelty Search for genome scoring."<<endl;
        }

        if (UseStaticStimNovelty)
        {
            cout<<"Using static brain stimulation for behavior recording"<<endl;

            if (StaticStimExpandedComparisons)
            {
                cout<<"Using expanded team comparisons for static stim mode."<<endl;
            }
        }
        else
        {
            cout<<"Using dynamic match data for behavior recording"<<endl;

            if (UseBrainOutputOnly)
            {
                cout<<"Using Brain outputs only for behavior recording"<<endl;
            }
            else
            {
                cout<<"Using both brain inputs and outputs for behavior recording"<<endl;
            }
        }
    }
    else
    {
        if (UseRandomEvolution)
        {
            cout<<"Using Random Search, all genomes receiving random fitness scores."<<endl;
        }
        else
        {
            cout<<"Using Objective Fitness for genome scoring."<<endl;
        }
    }
}







int main(int argc, char *argv[])
{
    cout<<"Competitive Coevolutionary Experiment starting..."<<endl;

    static struct timespec sleepTime;
    sleepTime.tv_sec  = 0;
    sleepTime.tv_nsec = nanoSleepAmount;


    // Used to look for arguments at the command line
    parseArguments(argc, argv);


    // Setup the semaphore
    initPosixSemaphore();



    // QT stuff
    qtConsoleAppPtr = new QCoreApplication(argc, argv);
    netServer       = new NetworkServer(param_networkPort);




    // Catch signal for clean shutdowns
    signal(SIGINT, signalHandler);


    // This thread will run
    // the actual AI simulation. Primary
    // loop will be controlled by QT
    // for the network code
    pthread_t jobManagementThread;
    pthread_t executionThreads[param_numExecutionThreads];



    // Spawn off a separate thread to run the job manager
    pthread_create(&jobManagementThread, NULL, runJobManagerThread, NULL);

    // Spawn of the execution threads to crunch numbers
    // on evaluations
    for (int i = 0; i < param_numExecutionThreads; i++)
    {
        THREAD_READY = false;

        pthread_create(&executionThreads[i], NULL, runExecutionThread, (void *)&i);

        while (!THREAD_READY)
	{
            // Wait for the new thread to grab
            // it's index number from the void* iterator pointer
            // before moving onto the next thread. This
            // make sure every execution thread has
            // the correct "index" number
            nanosleep(&sleepTime, NULL);
	}
    }

    // This will let the job management thread
    // know that all of the execution threads are spawned
    // and ready.
    EXECUTION_THREADS_ONLINE = true;


    // Give up main process to QT
    qtConsoleAppPtr->exec();

    netServer->close();


    cout<<"qtConsoleApp exited..."<<endl;
    delete netServer;
    delete qtConsoleAppPtr;


    // If we're recording data, we need to close
    // the datafile
    if (recordData)
    {
        closeDataRecordFile();
    }



    // We're done, cleanup time
    // Cleanup semaphores...
    closeSemaphore();


    // Clean up vectors...
    redTeamRobots.clear();
    blueTeamRobots.clear();
    redTeamBestRobots.clear();
    blueTeamBestRobots.clear();


    cout<<"Exiting experimental system..."<<endl;
    return 0;
}

