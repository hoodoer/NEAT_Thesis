#ifndef NETWORKSERVERTHREAD_H
#define NETWORKSERVERTHREAD_H

#include <QThread>
#include <QtNetwork>

#include "competitiveCoevolutionNeat.h"



// Network message types
enum messageId
{
    ACK_MSG                   = 0,
    PAUSE_MSG                 = 1,
    EXIT_MSG                  = 2,
    SWITCH_RENDER_THREAD_MSG  = 3,
    RENDER_SPEED_MSG          = 4,
    RENDER_DATA_MSG           = 5,
    GENERATION_DATA_MSG       = 6,
    HELLO_MSG                 = 7,
    GET_PAUSE_STATUS_MSG      = 8,
    RETURN_PAUSE_STATUS_MSG   = 9,
    GET_RENDER_SPEED_MSG      = 10,
    GET_RENDER_THREAD_MSG     = 11,
    RETURN_RENDER_SPEED_MSG   = 12,
    RETURN_RENDER_THREAD_MSG  = 13
};



class NetworkServerThread : public QThread
{
    Q_OBJECT

public:
    NetworkServerThread(int newSocketDescriptor, QObject *parent);
    ~NetworkServerThread();

    void run();


signals:
    void error(QTcpSocket::SocketError socketError);
    void getPauseStatus();
    void togglePause();
    void exitProgram();
    void getTimeMode();
    void toggleTimeMode();


public slots:
    void sendPauseStatus(bool pauseStatus);
    void sendTimeMode(timeType timeMode);
    void updateGenCount(unsigned int generationCounter,
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


private slots:
    void socketDisconnected();
    void receiveMessage();
    void threadStarted();



private:
    int         socketDescriptor;
    QTcpSocket *socket;
    quint16     blockSize;

    void sendMessage(messageId messageType, bool boolValue, int intValue,
                     int intValue2, int intValue3, int intValue4,
                     int intValue5, int intValue6, int intValue7,
                     int intValue8, int intValue9, int intValue10,
                     int intValue11);
};

#endif // NETWORKSERVERTHREAD_H
