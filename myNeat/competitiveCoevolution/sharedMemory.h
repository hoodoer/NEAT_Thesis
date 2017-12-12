#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <semaphore.h>
#include "competitiveCoevolutionNeat.h"

extern evaluationType evaluationMode;
extern bool           sharedMemoryReady;


void initPosixSemaphore();
void closeSemaphore();


bool exeThreadGetStandardJobToDo(evaluationJobData &assignedJob);
bool exeThreadGetPlayoffJobToDo(playoffJobData &assignedJob);
bool exeThreadGetGenChampJobToDo(generationChampJobData &assignedJob);
bool exeThreadGetDominanceTournyJobsToDo(dominanceTournamentJobData &assignedJob);
bool exeThreadGetNcdCalcJobToDo(ncdJobData &assignedJob);
bool exeThreadGetArchBehaviorJobToDo(archiveBehaviorRefreshJobData &assignedJob);
void exeThreadGetDominantStrategyRefData(int index, dominantStrategyReferenceData &refData);
void exeThreadReportJobResults(agentEvaluationData newJobResults);
void exeThreadReportPlayoffResults(playoffEvalData newJobResults);
void exeThreadReportGenChampResults(generationChampEvalData newJobResults);
void exeThreadReportDominanceTournyResults(dominanceTournamentEvalData newJobResults);
void exeThreadReportNcdCalculations(ncdEvalData newNcdResults);
void exeThreadReportArchBehaviorMatch(archiveBehaviorRefreshResultsData newResults);
int  exeThreadGetHallOfFamerIndex(int listIndex);

int  jobManagerGetResultsQueueSize();
int  jobManagerGetPlayoffResultsQueueSize();
int  jobManagerGetGenChampResultsQueueSize();
int  jobManagerGetDominanceTournyResultsQueueSize();
int  jobManagerGetNcdCalcResultsQueueSize();
int  jobManagerGetArchBehaviorResultsQueueSize();
void jobManagerClearResultsQueue();
void jobManagerClearGenChampResultsQueue();
void jobManagerClearDominanceTournyResultsQueue();
void jobManagerClearArchBehaviorResultsQueue();
void jobManagerClearAllQueues();
void jobManagerAddJobToQueue(evaluationJobData newEvalJob);
void jobManagerAddGenerationChampJobToQueue(generationChampJobData newGenChampEvalJob);
void jobManagerAddPlayoffJobToQueue(playoffJobData newPlayoffJob);
void jobManagerAddNcdCalcJobToQueue(ncdJobData newNcdCalcJob);
void jobManagerAddArchBehaviorJobToQueue(archiveBehaviorRefreshJobData newRefreshJob);
bool jobManagerGetEvalResults(agentEvaluationData &jobResults);
bool jobManagerGetPlayoffResults(playoffEvalData &jobResults);
bool jobManagerGetGenChampResults(generationChampEvalData &jobResults);
bool jobManagerGetDominanceTournyResults(dominanceTournamentEvalData &jobResults);
bool jobManagerGetNcdCalcResults(ncdEvalData &jobResults);
void jobManagerAddDominantStrategy(dominantStrategyReferenceData stratData);
void jobMangerGetLatestDominantStrategyRefData(dominantStrategyReferenceData &stratData);
void jobManagerAddDominantTournyJobToQueue(dominanceTournamentJobData newDominanceEvalJob);
int  jobManagerGetNumberOfDominantStrategies();
bool jobManagerCheckDominantStratsForGenome(hostTeamType genomeTeam, int genomeID);
void jobManagerPickHallOfFameRobotsForEval();



#endif // SHAREDMEMORY_H
