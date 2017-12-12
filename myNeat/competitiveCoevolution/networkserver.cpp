#include <QtNetwork>
#include <iostream>

using namespace std;

#include "networkserver.h"
#include "networkserverthread.h"


NetworkServer::NetworkServer(int portNumber) : QTcpServer(0)
{
    thread = NULL;
    cout<<"Server listening for client connections on port: "<<portNumber<<endl;
    listen(QHostAddress::Any, portNumber);
}

NetworkServer::~NetworkServer()
{
}



void NetworkServer::incomingConnection(int socketDescriptor)
{
    cout<<"Incoming network connection!"<<endl;

    thread = new NetworkServerThread(socketDescriptor, this);

    connect(thread, SIGNAL(finished()),
            thread, SLOT(deleteLater()));

    connect(thread, SIGNAL(started()),
            thread, SLOT(threadStarted()));

    connect(thread, SIGNAL(getPauseStatus()),
            this, SLOT(getPauseStatus()));

    connect(this, SIGNAL(sendPauseStatus(bool)),
            thread, SLOT(sendPauseStatus(bool)));

    connect(thread, SIGNAL(togglePause()),
            this, SLOT(togglePause()));

    connect(this, SIGNAL(updateGenCount(uint,int,int,int,int,int,int,int,int,int,int)),
            thread, SLOT(updateGenCount(uint,int,int,int,int,int,int,int,int,int,int)));

    connect(thread, SIGNAL(getTimeMode()),
            this, SLOT(getTimeMode()));

    connect(this, SIGNAL(sendTimeMode(timeType)),
            thread, SLOT(sendTimeMode(timeType)));


    connect(thread, SIGNAL(exitProgram()),
            this, SLOT(exitProgram()));

    connect(thread, SIGNAL(toggleTimeMode()),
            this, SLOT(toggleTimeMode()));

    thread->start();
}





void NetworkServer::getPauseStatus()
{
    emit sendPauseStatus(PAUSE);
}





void NetworkServer::updateGenerationCount(unsigned int generationCounter,
                                          int blueTeamChampIndex,
                                          int blueTeamChampNumNeurons,
                                          int blueTeamChampFitnessScore,
                                          int blueTeamBestFitnessEver,
                                          int blueTeamNumSpecies,
                                          int redTeamChampIndex,
                                          int redTeamChampNumNeurons,
                                          int redTeamChampFitnessScore,
                                          int redTeamBestFitnessEver,
                                          int redTeamNumSpecies)
{
    emit updateGenCount(generationCounter, blueTeamChampIndex, blueTeamChampNumNeurons,
                        blueTeamChampFitnessScore,blueTeamBestFitnessEver, blueTeamNumSpecies,
                        redTeamChampIndex,redTeamChampNumNeurons, redTeamChampFitnessScore,
                        redTeamBestFitnessEver, redTeamNumSpecies);
}






void NetworkServer::togglePause()
{
    if (PAUSE)
    {
        cout<<"Client requested unpausing system..."<<endl;
        PAUSE = false;
    }
    else
    {
        cout<<"Client requested pausing system..."<<endl;
        PAUSE = true;
    }

    emit sendPauseStatus(PAUSE);
}



void NetworkServer::exitProgram()
{
    cout<<"Client requested system shutdown..."<<endl;
    DONE = true;
    qtConsoleAppPtr->exit();
}


void NetworkServer::getTimeMode()
{
    emit sendTimeMode(timeMode);
}


void NetworkServer::toggleTimeMode()
{
    if (timeMode == realtime)
    {
        timeMode = accelerated;
    }
    else
    {
        timeMode = realtime;
    }

    emit sendTimeMode(timeMode);
}



