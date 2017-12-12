#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "networkserverthread.h"
#include "competitiveCoevolutionNeat.h"
#include "parameters.h"





class NetworkServer : public QTcpServer
{
    Q_OBJECT

public:
    NetworkServer(int portNumber = param_networkPort);
    ~NetworkServer();

    void updateGenerationCount(unsigned int generationCounter,
                               int blueTeamChampIndex,
                               int blueTeamChampNumNeurons,
                               int blutTeamChampFitnessScore,
                               int blueTeamBestFitnessEver,
                               int blueTeamNumSpecies,
                               int redTeamChampIndex,
                               int redTeamChampNumNeurons,
                               int redTeamChampFitnessScore,
                               int redTeamBestFitnessEver,
                               int redTeamNumSpecies);

protected:
    void incomingConnection (int socketDescriptor);

private slots:
    void getPauseStatus();
    void togglePause();
    void exitProgram();
    void getTimeMode();
    void toggleTimeMode();

signals:
    void sendPauseStatus(bool pauseStatus);
    void sendTimeMode(timeType timeMode);
    void updateGenCount(unsigned int generationCount,
                        int blueTeamChampIndex,
                        int blueTeamChampNumNeurons,
                        int blueTeamChampFitnessScore,
                        int blueTeamBestFitnessEver,
                        int blueTeamNumSpecies,
                        int redTeamChampIndex,
                        int redTeamChampNumNeurons,
                        int redTeamChampFitnessScore,
                        int redTeamBestFitnessEver,
                        int redTeamNumSpecies);

private:
    NetworkServerThread *thread;
};

#endif // NETWORKSERVER_H
