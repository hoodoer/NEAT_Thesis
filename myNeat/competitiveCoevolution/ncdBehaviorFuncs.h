#ifndef NCDBEHAVIORFUNCS_H
#define NCDBEHAVIORFUNCS_H
#include "competitiveCoevolutionNeat.h"


// This code bridges between the competitive coevolution
// experiment written by Drew Kirkpatrick,
// and LZMA NCD code by Rudi Cilibrasi
// For NCD information, please see the website:
// http://www.complearn.org/
//
// It also uses another Zlib based
// NCD calculation algorithm, see zlibNcd.h
// for the class, which is a faster, but
// more limited implementation of NCD
// than the LMZA implemenation from complearn.org
//
// drew.kirkpatrick@gmail.com



void reportAgentBehavior(agentBehaviorData behaviorData);
void serializeAgentBehavior(hostTeamType goodGuyTeam, int robotIndex,
                            unsigned int enemyIndex, unsigned int enemyGenomeID,
                            evalSide startSide);

void reportAndSerializeAgentStaticStimBehavior(agentStaticStimBehaviorData behaviorData);

void reportArchivedAgentBehavior(archiveAgentBehaviorData behaviorData);
void serializeArchivedAgentBehavior(hostTeamType team, int robotIndex, int genomeID);

void setGenerationEnemySeries(hostTeamType goodGuyTeam,
                              vector<int> enemyGenomesIdents);
void clearBehaviorDataMemory();
unsigned int getArchiveSize(hostTeamType team);

double getInterteamNcdValue(hostTeamType team,
                            int robotIndex1,
                            int robotIndex2);
double calculateNcdScoreForRobot(hostTeamType robotTeam, int robotIndex);

void addRobotToArchive(hostTeamType team, int genomeID);


//void setupStaticStimulusVector();
vector <brainInputData> getStaticStimulusVector();

void testFunc();


#endif // NCDBEHAVIORFUNCS_H
