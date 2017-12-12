#include "sharedMemory.h"



/**********************************************************************
             Shared Memory specific functions and variables
                The various threads communicate
                through shared memory, so we need
                semaphores for controlling access
*********************************************************************/
// The queue of jobs the job manager
// cooks up. This jobs are sent to
// the execution threads
// For standard generation evaluations
queue <evaluationJobData>   globalEvalJobQueue;
queue <agentEvaluationData> evaluationResultsQueue;


// This is the index's of which hall-of-famers
// that the execution threads should use for
// a particular generation. This is randomly
// picked by the jobManagerThread, and read
// by the execution threads, so that each
// agent fights the same hall-of-famers
// for an entire generation. Dr. Ken Stanley
// recommended this to reduce the noisy
// evaluation significantly.
int hallOfFameIndexes[param_numHallOfFamers];



// The queues for the job manager
// to cook up playoff tournmanet
// jobs for the execution threads
queue <playoffJobData>  globalPlayoffJobQueue;
queue <playoffEvalData> playoffTournamentResultsQueue;




// The queues for the job manager
// to cook up generation champ tournamet
// jobs for the execution threads
queue <generationChampJobData>  globalGenerationChampJobQueue;
queue <generationChampEvalData> generationChampResultsQueue;


// The queues for the job manager
// to cook up dominance tournament jobs
// for the execution threads
queue <dominanceTournamentJobData>  globalDominanceTournyJobQueue;
queue <dominanceTournamentEvalData> dominanceTournyResultsQueue;



// The queues for the job manager to cook up
// NCD calculations for the execution threads
// to run, and for execution threads
// reporting back the calculated NCD scores
queue <ncdJobData>  globalNcdJobQueue;
queue <ncdEvalData> ncdCalcResultsQueue;




// The queues for the job manager to cook up
// archived behavior match refresh jobs
// for the execution threads to run, and
// for the execution threads to report
// back to the job manager that jobs
// are complete
queue <archiveBehaviorRefreshJobData>     globalArchBehaviorRefreshJobQueue;
queue <archiveBehaviorRefreshResultsData> archiveBehaviorRefreshResultsQueue;





// Vector for holding robot agent reference
// data for dominant strategies. The execution
// threads will use this reference to use the
// correct robot agent from hall of fame
// vectors above.
vector <dominantStrategyReferenceData> dominantStrategiesVector;



// Current evaluation mode
evaluationType evaluationMode = standardEvaluation;


// Semaphore stuffs
sem_t processSemaphore;
bool  sharedMemoryReady = false;



// Lock the shared memory
inline void shmSemLock()
{
    if (sem_wait(&processSemaphore) != 0)
    {
        cout<<"Error locking semaphore"<<endl;
        DONE = true;
        exit(1);
    }
}


// Unlock the shared memory
inline void shmSemUnlock()
{
    if (sem_post(&processSemaphore) != 0)
    {
        cout<<"Error unlocking semaphore"<<endl;
        DONE = true;
        exit(1);
    }
}




// Initialize the semaphore to
// allow for save share memory
// access by the various threads
// This should only be called
// by the main func
void initPosixSemaphore()
{
    if (sem_init(&processSemaphore, 0, 1) != 0)
    {
        cout<<"Init'ing unamed semaphore failed!"<<endl;
        DONE = true;
        exit(1);
    }

    sharedMemoryReady = true;
}



// Called by main func to destroy the
// unamed semaphore
void closeSemaphore()
{
    sem_destroy(&processSemaphore);
}



// Called by execution thread to
// retrieve a standard eval job for it's job queue
// Returns false if the job queue is empty,
// and there are no more jobs to do
bool exeThreadGetStandardJobToDo(evaluationJobData &assignedJob)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!globalEvalJobQueue.empty())
        {
            returnValue = true;
            assignedJob = globalEvalJobQueue.front();
            globalEvalJobQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}





// Called by execution thread to
// retrieve playoff evaluation jobs for
// it's local job queue.
// Returns false if the job queue is empty,
// and there are no more jobs to do
bool exeThreadGetPlayoffJobToDo(playoffJobData &assignedJob)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!globalPlayoffJobQueue.empty())
        {
            returnValue = true;
            assignedJob = globalPlayoffJobQueue.front();
            globalPlayoffJobQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}





// Called by the execution thread to
// retrieve a generation champ eval job
// for it's job queue
// Returns false if the job queue is empty,
// and there are no more jobs to do
bool exeThreadGetGenChampJobToDo(generationChampJobData &assignedJob)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!globalGenerationChampJobQueue.empty())
        {
            returnValue = true;
            assignedJob = globalGenerationChampJobQueue.front();
            globalGenerationChampJobQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}



// Called by the execution thread to
// retrieve dominance tournament evaluation
// jobs for it's job queue.
// Returns false if the job queue is empty,
// and there are no more jobs to do
bool exeThreadGetDominanceTournyJobsToDo(dominanceTournamentJobData &assignedJob)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!globalDominanceTournyJobQueue.empty())
        {
            returnValue = true;
            assignedJob = globalDominanceTournyJobQueue.front();
            globalDominanceTournyJobQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}




// Called by the execution threads to
// retrieve NCD calculation jobs for it's
// local job queue. Returns false if
// the global job queue is empty,
// and there are no more jobs to do
bool exeThreadGetNcdCalcJobToDo(ncdJobData &assignedJob)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!globalNcdJobQueue.empty())
        {
            returnValue = true;
            assignedJob = globalNcdJobQueue.front();
            globalNcdJobQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}



// Called by the execution threads to
// retrieve archived behavior refresh
// match jobs for it's local job queue.
// Returns false if the global job
// queue is empty and there are
// no more jobs to do
bool exeThreadGetArchBehaviorJobToDo(archiveBehaviorRefreshJobData &assignedJob)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!globalArchBehaviorRefreshJobQueue.empty())
        {
            returnValue = true;
            assignedJob = globalArchBehaviorRefreshJobQueue.front();
            globalArchBehaviorRefreshJobQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}





// Called by the execution thread to
// retrieve dominance tournament strategy
// reference data.
void exeThreadGetDominantStrategyRefData(int index, dominantStrategyReferenceData &refData)
{
    shmSemLock();
    {
        if (dominantStrategiesVector.empty())
        {
            cout<<"Error in exeThreadGetDominantStrategyRefData, vector is empty!"<<endl;
            exit(1);
        }

        refData = dominantStrategiesVector.at(index);
    }
    shmSemUnlock();
}




// Allow the execution thread to add
// standard evaluation results to the results
// queue for the job manager to contend
// with
void exeThreadReportJobResults(agentEvaluationData newJobResults)
{
    shmSemLock();
    {
        evaluationResultsQueue.push(newJobResults);
    }
    shmSemUnlock();
}




// Allow execution threads to add playoff tournament
// job results to the results queue for the job
// manager to contend with
void exeThreadReportPlayoffResults(playoffEvalData newJobResults)
{
    shmSemLock();
    {
        playoffTournamentResultsQueue.push(newJobResults);
    }
    shmSemUnlock();
}



// Allow execution threads to add generation champ
// tourney results to the results queue for the job
// manager to contend with
void exeThreadReportGenChampResults(generationChampEvalData newJobResults)
{
    shmSemLock();
    {
        generationChampResultsQueue.push(newJobResults);
    }
    shmSemUnlock();
}







// Allows the execution threads to add dominance tourny
// results to the results queue for the job manager
// to contend with
void exeThreadReportDominanceTournyResults(dominanceTournamentEvalData newJobResults)
{
    shmSemLock();
    {
        dominanceTournyResultsQueue.push(newJobResults);
    }
    shmSemUnlock();
}





// Allows the execution threads to add ncd
// calculation results to the results queue for
// the job manager to contend with
void exeThreadReportNcdCalculations(ncdEvalData newNcdResults)
{
    shmSemLock();
    {
        ncdCalcResultsQueue.push(newNcdResults);
    }
    shmSemUnlock();
}



// Allows the execution threads to report
// archived behavior refresh matches as complete
void exeThreadReportArchBehaviorMatch(archiveBehaviorRefreshResultsData newResults)
{
    shmSemLock();
    {
        archiveBehaviorRefreshResultsQueue.push(newResults);
    }
    shmSemUnlock();
}




// Allows the execution threads to get the robot index's
// to use for hall of famers during standard evaluations
int exeThreadGetHallOfFamerIndex(int listIndex)
{
    int returnIndex;

    shmSemLock();
    {
        returnIndex = hallOfFameIndexes[listIndex];
    }
    shmSemUnlock();

    return returnIndex;
}





// Allows the job manager to get the size
// of the results queue
int jobManagerGetResultsQueueSize()
{
    int size;

    shmSemLock();
    {
        size = evaluationResultsQueue.size();
    }
    shmSemUnlock();

    return size;
}





// Allows the job manager to get
// the size of the playoff tournament
// results queue
int jobManagerGetPlayoffResultsQueueSize()
{
    int size;

    shmSemLock();
    {
        size = playoffTournamentResultsQueue.size();
    }
    shmSemUnlock();

    return size;
}




// Allows the job manager to get the size
// of the generation champ tourny results
// queue
int jobManagerGetGenChampResultsQueueSize()
{
    int size;

    shmSemLock();
    {
        size = generationChampResultsQueue.size();
    }
    shmSemUnlock();

    return size;
}



// Allows the job manager to get the size
// of the dominance tournament results
// queue
int jobManagerGetDominanceTournyResultsQueueSize()
{
    int size;

    shmSemLock();
    {
        size = dominanceTournyResultsQueue.size();
    }
    shmSemUnlock();

    return size;
}





// Allows the job manager to get the size
// of the NCD calculations results queue
int jobManagerGetNcdCalcResultsQueueSize()
{
    int size;

    shmSemLock();
    {
        size = ncdCalcResultsQueue.size();
    }
    shmSemUnlock();

    return size;
}





// Allows the job manager to get the size
// of the archive behavior refresh job
// results queue
int jobManagerGetArchBehaviorResultsQueueSize()
{
    int size;

    shmSemLock();
    {
        size = archiveBehaviorRefreshResultsQueue.size();
    }
    shmSemUnlock();

    return size;
}



// Shortcut to clear all results
// in the standard eval results queue
void jobManagerClearResultsQueue()
{
    shmSemLock();
    {
        while (!evaluationResultsQueue.empty())
        {
            evaluationResultsQueue.pop();
        }
    }
    shmSemUnlock();
}



// Shortcut to clear all results
// in the generation champ results queue
void jobManagerClearGenChampResultsQueue()
{
    shmSemLock();
    {
        while (!generationChampResultsQueue.empty())
        {
            generationChampResultsQueue.pop();
        }
    }
    shmSemUnlock();
}



// Shortcut to clear all results in the
// dominance tournament results queue
void jobManagerClearDominanceTournyResultsQueue()
{
    shmSemLock();
    {
        while (!dominanceTournyResultsQueue.empty())
        {
            dominanceTournyResultsQueue.pop();
        }
    }
    shmSemUnlock();
}



// Shortcut to clear all the results in
// the archived behaior refresh match
// results queue.
void jobManagerClearArchBehaviorResultsQueue()
{
    shmSemLock();
    {
        while (!archiveBehaviorRefreshResultsQueue.empty())
        {
            archiveBehaviorRefreshResultsQueue.pop();
        }
    }
    shmSemUnlock();
}





// Shortcut to make sure all queues are empty,
// useful for the job manager to make sure
// things are empty at the end of a generation
void jobManagerClearAllQueues()
{
    shmSemLock();
    {
        while (!globalEvalJobQueue.empty())
        {
            globalEvalJobQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!evaluationResultsQueue.empty())
        {
            evaluationResultsQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!globalPlayoffJobQueue.empty())
        {
            globalPlayoffJobQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!playoffTournamentResultsQueue.empty())
        {
            playoffTournamentResultsQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!globalGenerationChampJobQueue.empty())
        {
            globalGenerationChampJobQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!generationChampResultsQueue.empty())
        {
            generationChampResultsQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!globalDominanceTournyJobQueue.empty())
        {
            globalDominanceTournyJobQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!dominanceTournyResultsQueue.empty())
        {
            dominanceTournyResultsQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!globalNcdJobQueue.empty())
        {
            globalNcdJobQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!ncdCalcResultsQueue.empty())
        {
            ncdCalcResultsQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!globalArchBehaviorRefreshJobQueue.empty())
        {
            globalArchBehaviorRefreshJobQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }

        while (!archiveBehaviorRefreshResultsQueue.empty())
        {
            archiveBehaviorRefreshResultsQueue.pop();
            cout<<"Warning! Non-empty queues being cleared!"<<endl;
        }
    }
    shmSemUnlock();
}





// Allows the job manager to add another
// requested evaluation to the back of
// the job queue
void jobManagerAddJobToQueue(evaluationJobData newEvalJob)
{
    shmSemLock();
    {
        globalEvalJobQueue.push(newEvalJob);
    }
    shmSemUnlock();
}


// Allows the job manager to add another
// requested match to the queue of
// generation champ tournament jobs
void jobManagerAddGenerationChampJobToQueue(generationChampJobData newGenChampEvalJob)
{
    shmSemLock();
    {
        globalGenerationChampJobQueue.push(newGenChampEvalJob);
    }
    shmSemUnlock();
}



// Allows the job manager to add another
// requested match to the queue of
// playoff tournment jobs
void jobManagerAddPlayoffJobToQueue(playoffJobData newPlayoffJob)
{
    shmSemLock();
    {
        globalPlayoffJobQueue.push(newPlayoffJob);
    }
    shmSemUnlock();
}



// Allows the job manager to add
// another requested NCD calculation
// job to the queue
void jobManagerAddNcdCalcJobToQueue(ncdJobData newNcdCalcJob)
{
    shmSemLock();
    {
        globalNcdJobQueue.push(newNcdCalcJob);
    }
    shmSemUnlock();
}





// Allows the job manager to add
// another requested archived behavior
// refresh match to the queue of
// jobs for execution threads to crunch
void jobManagerAddArchBehaviorJobToQueue(archiveBehaviorRefreshJobData newRefreshJob)
{
    shmSemLock();
    {
        globalArchBehaviorRefreshJobQueue.push(newRefreshJob);
    }
    shmSemUnlock();
}





// Called by the job manager thread to
// retrieve job results.
// Returns false if the results queue is empty,
// and true if there are more results to crunch
bool jobManagerGetEvalResults(agentEvaluationData &jobResults)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!evaluationResultsQueue.empty())
        {
            returnValue = true;
            jobResults  = evaluationResultsQueue.front();
            evaluationResultsQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}






// Called by the job manager thread to
// retrieve playoff tournament eval results.
// Returns false if the results queue is
// empty, and true if there are more
// results to crunch
bool jobManagerGetPlayoffResults(playoffEvalData &jobResults)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!playoffTournamentResultsQueue.empty())
        {
            returnValue = true;
            jobResults = playoffTournamentResultsQueue.front();
            playoffTournamentResultsQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}






// Called by the job manager thread to
// retrieve generation champ eval results.
// Returns false if the results queue is empty,
// and true if there are more results to crunch
bool jobManagerGetGenChampResults(generationChampEvalData &jobResults)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!generationChampResultsQueue.empty())
        {
            returnValue = true;
            jobResults  = generationChampResultsQueue.front();
            generationChampResultsQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}



// Called by the job manager thread to
// retrieve dominance tournament evaluation results.
// Returns false if the results queue is empty,
// and true if there are results to crunch
bool jobManagerGetDominanceTournyResults(dominanceTournamentEvalData &jobResults)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!dominanceTournyResultsQueue.empty())
        {
            returnValue = true;
            jobResults  = dominanceTournyResultsQueue.front();
            dominanceTournyResultsQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}


// Called by job manager thread to
// retrieve NCD calculation eval results.
// Returns false if the results queue is empty,
// and true if there are results to crunch
bool jobManagerGetNcdCalcResults(ncdEvalData &jobResults)
{
    bool returnValue = false;

    shmSemLock();
    {
        if (!ncdCalcResultsQueue.empty())
        {
            returnValue = true;
            jobResults = ncdCalcResultsQueue.front();
            ncdCalcResultsQueue.pop();
        }
    }
    shmSemUnlock();

    return returnValue;
}




// Called by the job manager to add in a new dominant strategy that
// has been found by way of the dominance tournamenet
void jobManagerAddDominantStrategy(dominantStrategyReferenceData stratData)
{
    shmSemLock();
    {
        dominantStrategiesVector.push_back(stratData);
    }
    shmSemUnlock();
}



// Allow the job manager to get referencing data for the latest (best) dominant strategy
void jobMangerGetLatestDominantStrategyRefData(dominantStrategyReferenceData &stratData)
{
    shmSemLock();
    {
        stratData = dominantStrategiesVector.back();
    }
    shmSemUnlock();
}


// Allows the job manager to add another requested match to the
// queue of dominanace tournament jobs
void jobManagerAddDominantTournyJobToQueue(dominanceTournamentJobData newDominanceEvalJob)
{
    shmSemLock();
    {
        globalDominanceTournyJobQueue.push(newDominanceEvalJob);
    }
    shmSemUnlock();
}







// Allows the job manager to get the number of dominant strategies
int jobManagerGetNumberOfDominantStrategies()
{
    int returnSize;

    shmSemLock();
    {
        returnSize = dominantStrategiesVector.size();
    }
    shmSemUnlock();

    return returnSize;
}





// Allows the Job Manager thread to check if a generation champ
// is already one of the dominant strategy. Returns true if
// the genome is already one of the dominant strategies, and false
// if it isn't.
bool jobManagerCheckDominantStratsForGenome(hostTeamType genomeTeam, int genomeID)
{
    bool returnValue = false;
    dominantStrategyReferenceData stratRefData;
    int dominantStratGenomeID;

    shmSemLock();
    {
        for (unsigned int i = 0; i < dominantStrategiesVector.size(); i++)
        {
            stratRefData = dominantStrategiesVector.at(i);

            switch (stratRefData.dominantStratTeam)
            {
            case REDTEAM:
                dominantStratGenomeID = redTeamHallOfFameRobots[0][stratRefData.stratHofIndex]->getBrainGenomeID();
                break;

            case BLUETEAM:
                dominantStratGenomeID = blueTeamHallOfFameRobots[0][stratRefData.stratHofIndex]->getBrainGenomeID();
                break;

            default:
                cout<<"Error in jobManagerCheckDominantStratsForGenome, invalid stratTeam."<<endl;
                DONE = true;
                exit(1);
            }

            if ((stratRefData.dominantStratTeam == genomeTeam) &&
                    (dominantStratGenomeID == genomeID))
            {
                cout<<"!!!! Generation Champ is already in the dominant strategies!"<<endl;
                returnValue = true;
            }
        }
    }
    shmSemUnlock();

    return returnValue;
}





// Helper function for jobManagerPickHallOfFameRobotsForEval
inline bool isHofIndexAlreadyListed(int robotIndex)
{
    for (int i = 0; i < param_numHallOfFamers; i++)
    {
        if (hallOfFameIndexes[i] == robotIndex)
        {
            return true;
        }
    }

    return false;
}




// Randomly selects the hall-of-famers for the
// execution threads to use for standard evaluations.
// Called from the setupEvaluationJobs function
void jobManagerPickHallOfFameRobotsForEval()
{
    shmSemLock();
    {
        int pickedIndex;

        // Set all of the index's to -1 for easier testing
        for (int i = 0; i < param_numHallOfFamers; i++)
        {
            hallOfFameIndexes[i] = -1;
        }

        for (int i = 0; i < param_numHallOfFamers; i++)
        {
            // If we don't have enough hall of famers,
            // repeats have to be allowed
            if (generationCounter-1 < param_numHallOfFamers)
            {
                // At start of generation 2, we have 1 HOF'er (index 0)...
                if (generationCounter > 2)
                {
                    pickedIndex = RandInt(0, generationCounter-2);
                }
                else
                {
                    pickedIndex = 0;
                }

                hallOfFameIndexes[i] = pickedIndex;
            }
            else // We have enough HOF'ers to disallow repeats
            {
                pickedIndex = RandInt(0, generationCounter-2);

                // Is this index already listed?
                while (isHofIndexAlreadyListed(pickedIndex))
                {
                    // it is already listed, pick another
                    pickedIndex = RandInt(0, generationCounter-2);
                }

                hallOfFameIndexes[i] = pickedIndex;
            }
        }
    }
    shmSemUnlock();
}

