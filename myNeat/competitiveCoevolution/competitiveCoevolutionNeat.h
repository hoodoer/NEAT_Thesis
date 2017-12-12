#ifndef COMPETITIVECOEVOLUTIONNEAT_H
#define COMPETITIVECOEVOLUTIONNEAT_H

#include <QtCore/QCoreApplication>
#include <queue>
#include "robotAgent.h"
#include "gameworld.h"


// Various type definitions
enum hostTeamType
{
    BLUETEAM,
    REDTEAM
};

enum evalSide
{
    LEFT,
    RIGHT
};


enum renderType
{
    renderOff,
    normal,
    best
};



enum timeType
{
    realtime,
    accelerated
};


enum evaluationType
{
    standardEvaluation,
    playoffTournament,
    generationChampTournament,
    dominanceTournament,
    ncdCalculations,
    archiveBehaviorMatches,
    stopEvals
};


// structure used to track
// performance through all
// the evaluations for an agent
struct agentEvaluationData
{
    unsigned int hostRobotIndex;
    hostTeamType hostTeam;
    unsigned int score;
};


// Structure used to report
// scores of generation champ
// evaluations
struct generationChampEvalData
{
    int redChampScore;
    int blueChampScore;
};



// Structure used to report
// scores of dominance
// tournament evaluations
struct dominanceTournamentEvalData
{
    int dominantStrategyScore;
    int genChampScore;
};



// How the job manager pushes
// evaluation jobs to the execution
// threads. The execution thread
// will run both left start,
// and right start variants
// of the job
struct evaluationJobData
{
    hostTeamType hostTeam;
    int          hostRobotIndex;
};


// How the job manager pushes
// generation champ jobs to
// the execution threads.
// The execution thread will
// run both the left and right start
// variants of a job.
struct generationChampJobData
{
    int            blueTeamChampIndex;
    int            redTeamChampIndex;
    foodLayoutType foodLayout;
    int            firstExtraFoodPosIndex;
    int            secondExtraFoodPosIndex;
};



// How the job manager pushes
// dominance tournament jobs
// to the execution threads.
// The exe threads will
// run both the left and right
// start variants of the job
struct dominanceTournamentJobData
{
    int            dominanceStrategyIndex;
    hostTeamType   genChampTeam;
    int            genChampIndex;
    foodLayoutType foodLayout;
    int            firstExtraFoodPosIndex;
    int            secondExtraFoodPosIndex;
};




// Structure to reference a
// dominant strategy. Any genome
// that has become a dominant strategy will
// by definition be a generation champ (at some point),
// and thus will be in the hall of fame for a particlar
// team. Therefore, we can reference a dominant strategy
// by team, and hall of fame index.
struct dominantStrategyReferenceData
{
    hostTeamType dominantStratTeam;
    int          stratHofIndex;
};




// Used in the playoffMemberReferenceData
// struct below, useful for accumulating
// playoff results
struct playoffMemberEnemyData
{
    int enemyGenomeId;
    int redScore;
    int blueScore;
};



// Structure used to reference
// team members who are tied for
// generation champ. This is used
// to schedule playoffs to determine
// who the "real" generation champ
// is
// The vector of these structs is
// also used to accumulate the
// playoff tournament scores
struct playoffMemberReferenceData
{
    int robotIndex;
    int robotGenomeId;
    int memberReferenceIndex;

    int numWins;
    int numLosses;
    int numTies;
    int globalScore;

    hostTeamType robotTeam;

    vector <playoffMemberEnemyData> enemyDataVector;
};




// How the job manager pushes
// playoff tournament jobs
// to the execution threads.
// The exe threads will
// run both the left and right
// start variants of the job
struct playoffJobData
{
    int            blueTeamContestantRobotIndex;
    int            blueTeamContestantGenomeId;
    int            blueTeamContestantDataIndex;

    int            redTeamContestantRobotIndex;
    int            redTeamContestantGenomeId;
    int            redTeamContestantDataIndex;

    foodLayoutType foodLayout;
    int            firstExtraFoodPosIndex;
    int            secondExtraFoodPosIndex;
};



// Structure used to report
// scores of playoff
// tournaments evaluations
struct playoffEvalData
{
    int blueTeamContestantRobotIndex;
    int blueTeamContestantGenomeId;
    int blueTeamContestantDataIndex;

    int redTeamContestantRobotIndex;
    int redTeamContestantGenomeId;
    int redTeamContestantDataIndex;

    int redTeamContestantScore;
    int blueTeamContestantScore;
};



// Structure used to
// request NCD calculations
// for getting a novelty score
struct ncdJobData
{
    hostTeamType team;
    int          robotIndex;
};



// Structure used to report
// results of NCD calculations
// for novelty score
struct ncdEvalData
{
    hostTeamType team;
    int          robotIndex;
    double       ncdScore;
};



// Structure used to
// store novelty score
// data about team
// members
struct ncdScoreReferenceData
{
    int    robotIndex;
    double ncdScore;

    bool operator<(const ncdScoreReferenceData& a) const
    {
        return ncdScore < a.ncdScore;
    }
};





// Structure used to request
// archive behavior refresh matches
struct archiveBehaviorRefreshJobData
{
    hostTeamType team;
    int          index;
};



// We're not actually storing fitness results
// from these jobs. Only the behavior data
// is being stored, so that it can be
// serialized. This is just to indicate
// to the job manager thread that
// a job is complete
struct archiveBehaviorRefreshResultsData
{
    hostTeamType team;
    int          index;
    bool         done;
};




// Structure used for
// saving data about
// robot behavior. Used
// for NCD (Normalized Compression Distance)
// comparisons to
// drive novelty search
struct brainBehaviorData
{
    brainInputData  inputData;
    brainOutputData outputData;
};




// Structure used for
// reporting robot behavior
// data from standard
// evaluations during novelty search
struct agentBehaviorData
{
    unsigned int      robotIndex;
    hostTeamType      robotTeam;
    evalSide          robotStartSide;
    unsigned int      enemyIndex;
    unsigned int      enemyGenomeID;

    brainBehaviorData behaviorSnapshot;
};


// structure used for
// reporting robot behavior
// data during static
// stimulation novelty search
struct agentStaticStimBehaviorData
{
    unsigned int             robotIndex;
    hostTeamType             robotTeam;
    int                      genomeID;

    vector <brainOutputData> brainOutputVector;
};



// Structure used for
// reporting robot behavior
// data from standard
// evals when brain output
// only mode is turned on
struct agentOutputBehaviorData
{
    unsigned int      robotIndex;
    hostTeamType      robotTeam;
    evalSide          robotStartSide;
    unsigned int      enemyIndex;
    unsigned int      enemyGenomeID;

    brainOutputData   outputData;
};


// Structure used for
// reporting robot behavior
// data from behavior archive
// updated evaluations against
// new enemies
struct archiveAgentBehaviorData
{
    unsigned int      robotIndex;
    hostTeamType      robotTeam;
    unsigned int      genomeID;
    brainBehaviorData behaviorSnapshot;
};





// This area externs variables in competitiveCoevolutionNeatMain.cpp
// that need to be available to the network server code
extern bool              PAUSE;
extern bool              DONE;
extern bool              UseStaticStimNovelty;
extern bool              StaticStimExpandedComparisons;
extern bool              UseBrainOutputOnly;
extern timeType          timeMode;
extern renderType        renderMode;
extern unsigned int      generationCounter;
extern QCoreApplication *qtConsoleAppPtr;

extern vector < vector<RobotAgent*> > redTeamHallOfFameRobots;
extern vector < vector<RobotAgent*> > blueTeamHallOfFameRobots;


#endif // COMPETITIVECOEVOLUTIONNEAT_H
