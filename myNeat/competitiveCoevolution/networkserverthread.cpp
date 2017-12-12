#include "networkserverthread.h"

#include <QtNetwork>
#include <iostream>

using namespace std;


NetworkServerThread::NetworkServerThread(int newSocketDescriptor, QObject *parent) : QThread(parent)
{
    socketDescriptor = newSocketDescriptor;
    blockSize        = 0;
}


NetworkServerThread::~NetworkServerThread()
{
    if (socket->isOpen())
    {
        socket->close();
    }
}



void NetworkServerThread::run()
{
    exec();
}



void NetworkServerThread::socketDisconnected()
{
    cout<<"Network connection closed"<<endl;
    quit();
}



void NetworkServerThread::threadStarted()
{
    socket = new QTcpSocket(this);

    connect(socket, SIGNAL(disconnected()),
            this, SLOT(socketDisconnected()));

    connect(socket, SIGNAL(readyRead()),
            this, SLOT(receiveMessage()));

    if (!socket->setSocketDescriptor(socketDescriptor))
    {
        emit error(socket->error());
        cout<<"Socket error: "<<socket->errorString().toStdString()<<endl;
        return;
    }
}



void NetworkServerThread::receiveMessage()
{
    QDataStream in(socket);
//    in.setVersion(QDataStream::Qt_4_7);
    in.setVersion(QDataStream::Qt_4_0);


    while (!in.atEnd())
    {
        if (blockSize == 0)
        {
            if (socket->bytesAvailable() < (int)sizeof(quint16))
            {
                return;
            }

            in >> blockSize;
        }

        if (socket->bytesAvailable() < blockSize)
        {
            return;
        }

        messageId messageType;
        quint8    messageNum;

        in >> messageNum;
        messageType = (messageId)messageNum;
        blockSize   = 0;

        switch (messageType)
        {
        case ACK_MSG:
            cout<<"Got an ACK message"<<endl;
            break;

        case PAUSE_MSG:
            emit togglePause();
            break;

        case EXIT_MSG:
            emit exitProgram();
            break;

        case SWITCH_RENDER_THREAD_MSG:
            cout<<"Got a SWITCH_RENDER_THREAD message"<<endl;
            break;

        case RENDER_SPEED_MSG:
            emit toggleTimeMode();
            break;

        case HELLO_MSG:
            cout<<"Got a hello message!"<<endl;
            break;

        case GET_PAUSE_STATUS_MSG:
            emit getPauseStatus();
            break;

        default:
            cout<<"In network receiveMessage, invalid message ID"<<endl;
            break;
        }
    }
}




void NetworkServerThread::sendPauseStatus(bool pauseStatus)
{
    sendMessage(RETURN_PAUSE_STATUS_MSG, pauseStatus, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}



void NetworkServerThread::updateGenCount(unsigned int generationCounter,
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
    sendMessage(GENERATION_DATA_MSG, FALSE, (int)generationCounter,
                blueTeamChampIndex,
                blueTeamChampNumNeurons,
                blueTeamChampFitnessScore,
                blueTeamBestFitnessEver,
                blueTeamNumSpecies,
                redTeamChampIndex,
                redTeamChampNumNeurons,
                redTeamChampFitnessScore,
                redTeamBestFitnessEver,
                redTeamNumSpecies);
}





void NetworkServerThread::sendTimeMode(timeType timeMode)
{
    sendMessage(RETURN_RENDER_SPEED_MSG, FALSE, (int)timeMode,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}









/********************************************************
     Send network message functions
********************************************************/
void NetworkServerThread::sendMessage(messageId messageType, bool boolValue, int intValue,
                                      int intValue2, int intValue3, int intValue4,
                                      int intValue5, int intValue6, int intValue7,
                                      int intValue8, int intValue9, int intValue10,
                                      int intValue11)
{
    QByteArray messageArray;
    QDataStream out(&messageArray, QIODevice::WriteOnly);
//    out.setVersion(QDataStream::Qt_4_7);
    out.setVersion(QDataStream::Qt_4_0);

    out << (quint16)0; // save space for message size data
    out << (quint8)messageType;

    // Pack in various extra parameters
    switch(messageType)
    {
    case ACK_MSG:
        cout<<"Sending an ACK message"<<endl;
        break;

    case EXIT_MSG:
        cout<<"Sending an EXIT message"<<endl;
        break;

    case RENDER_DATA_MSG:
        cout<<"Sending a RENDER_DATA message"<<endl;
        break;

    case GENERATION_DATA_MSG:
        out << (quint16)intValue;
        out << (quint16)intValue2;
        out << (quint16)intValue3;
        out << (quint16)intValue4;
        out << (quint16)intValue5;
        out << (quint16)intValue6;
        out << (quint16)intValue7;
        out << (quint16)intValue8;
        out << (quint16)intValue9;
        out << (quint16)intValue10;
        out << (quint16)intValue11;
        break;

    case HELLO_MSG:
        cout<<"Sending a hello message!"<<endl;
        break;


    case RETURN_PAUSE_STATUS_MSG:
        out << boolValue;
        break;

    case RETURN_RENDER_SPEED_MSG:
        out << (quint16)intValue;
        break;

    default:
        cout<<"In network sendMessage, invalid message ID"<<endl;
        break;
    }

    out.device()->seek(0); // Go back to beginning and write the message size
    out << (quint16)(messageArray.size() - sizeof(quint16));

    socket->write(messageArray);
}





