#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>

using namespace std;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    blockSize    = 0;
    connected    = false;
    systemPaused = false;

    blueChampIndex        = 0;
    blueChampNumNeurons   = 0;
    blueChampFitnessScore = 0;
    blueBestFitnessEver   = 0;
    blueTeamNumSpecies    = 0;

    redChampIndex         = 0;
    redChampNumNeurons    = 0;
    redChampFitnessScore  = 0;
    redBestFitnessEver    = 0;
    redTeamNumSpecies     = 0;


    tcpSocket = new QTcpSocket(this);

    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));

    connect(tcpSocket, SIGNAL(connected()),
            this, SLOT(connectedSlot()));

    connect(tcpSocket, SIGNAL(disconnected()),
            this, SLOT(disconnectedSlot()));

    connect(tcpSocket, SIGNAL(readyRead()),
            this, SLOT(receiveMessage()));

    ui->setupUi(this);

    // Various UI setup thingies...
    ui->connectionStatus->setStyleSheet("background-color:red");
    ui->tabWidget->setCurrentIndex(0);

    ui->timeModeComboBox->setEditable(false);
    ui->timeModeComboBox->addItem("Accelerated Time");
    ui->timeModeComboBox->addItem("Realtime Mode");
    ui->timeModeComboBox->setCurrentIndex(0);

    // OpenGL rendering stuff
    graphicsView = new QGraphicsView(ui->renderWidget);
    graphicsView->setViewport(new QGLWidget);
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 700, 700);
    graphicsView->setScene(scene);
//    ui->renderWidget->setDisabled(true);
}




MainWindow::~MainWindow()
{
    delete graphicsView;
    delete scene;
    delete tcpSocket;
    delete ui;
}






void MainWindow::on_connectButton_clicked()
{
    QString hostname;
    QString portString;

    hostname   = ui->hostNameEdit->text();
    portString = ui->portEdit->text();

    cout<<"Trying to connect to host: "<<hostname.toStdString()<<", port: "<<portString.toStdString()<<endl;
    tcpSocket->connectToHost(hostname, portString.toInt());
}


void MainWindow::on_disconnectButton_clicked()
{
    tcpSocket->disconnectFromHost();
}





void MainWindow::on_pushButton_clicked()
{
    sendMessage(HELLO_MSG);
}





void MainWindow::connectedSlot()
{
    cout<<"Client connected to server"<<endl;
    ui->connectButton->setEnabled(false);
    ui->disconnectButton->setEnabled(true);
    ui->connectionStatus->setStyleSheet("background-color:lightgreen");
    ui->connectionStatus->setText("Connected");

    connected = true;


    // Get initial data values
    sendMessage(GET_PAUSE_STATUS_MSG);
    sendMessage(GET_RENDER_SPEED_MSG);
}




void MainWindow::disconnectedSlot()
{
    cout<<"Client disconnected from server"<<endl;
    ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false);
    ui->connectionStatus->setStyleSheet("background-color:red");
    ui->connectionStatus->setText("Not Connected");

    connected = false;
}



void MainWindow::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        break;

    case QAbstractSocket::HostNotFoundError:
        cout<<"Host not found"<<endl;
        break;

    case QAbstractSocket::ConnectionRefusedError:
        cout<<"Connection refused by peer."<<endl;
        break;

    default:
        cout<<"Connection error"<<endl;
    }
}






void MainWindow::receiveMessage()
{
    cout<<"Client received message from server..."<<endl;
    QDataStream in(tcpSocket);
    in.setVersion(QDataStream::Qt_4_7);

    while (!in.atEnd())
    {
        if (blockSize == 0)
        {
            if (tcpSocket->bytesAvailable() < (int)sizeof(quint16))
            {
                return;
            }

            in >> blockSize;
        }

        if (tcpSocket->bytesAvailable() < blockSize)
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

        case EXIT_MSG:
            cout<<"Got an EXIT message"<<endl;
            break;

        case RENDER_DATA_MSG:
            cout<<"Got a RENDER_DATA message"<<endl;
            break;

        case GENERATION_DATA_MSG:
            cout<<"Got a GENERATION_DATA message"<<endl;
            quint16 genCount;
            quint16 btChampIndex;
            quint16 btChampNumNeurons;
            quint16 btChampFitnessScore;
            quint16 btBestFitnessEver;
            quint16 btNumSpecies;
            quint16 rtChampIndex;
            quint16 rtChampNumNeurons;
            quint16 rtChampFitnessScore;
            quint16 rtBestFitnessEver;
            quint16 rtNumSpecies;

            in >> genCount;
            in >> btChampIndex;
            in >> btChampNumNeurons;
            in >> btChampFitnessScore;
            in >> btBestFitnessEver;
            in >> btNumSpecies;
            in >> rtChampIndex;
            in >> rtChampNumNeurons;
            in >> rtChampFitnessScore;
            in >> rtBestFitnessEver;
            in >> rtNumSpecies;

            generationCounter     = (unsigned int)genCount;
            blueChampIndex        = (int)btChampIndex;
            blueChampNumNeurons   = (int)btChampNumNeurons;
            blueChampFitnessScore = (int)btChampFitnessScore;
            blueBestFitnessEver   = (int)btBestFitnessEver;
            blueTeamNumSpecies    = (int)btNumSpecies;
            redChampIndex         = (int)rtChampIndex;
            redChampNumNeurons    = (int)rtChampNumNeurons;
            redChampFitnessScore  = (int)rtChampFitnessScore;
            redBestFitnessEver    = (int)rtBestFitnessEver;
            redTeamNumSpecies     = (int)rtNumSpecies;

            ui->genCounterLine->setText(QString::number(generationCounter));
            ui->blueChampIndexLine->setText(QString::number(blueChampIndex));
            ui->blueNumNeuronsLine->setText(QString::number(blueChampNumNeurons));
            ui->blueFitnessLine->setText(QString::number(blueChampFitnessScore));
            ui->blueBestFitnessLine->setText(QString::number(blueBestFitnessEver));
            ui->blueNumSpeciesLine->setText(QString::number(blueTeamNumSpecies));
            ui->redChampIndexLine->setText(QString::number(redChampIndex));
            ui->redNumNeuronsLine->setText(QString::number(redChampNumNeurons));
            ui->redFitnessLine->setText(QString::number(redChampFitnessScore));
            ui->redBestFitnessLine->setText(QString::number(redBestFitnessEver));
            ui->redNumSpeciesLine->setText(QString::number(redTeamNumSpecies));
            break;

        case HELLO_MSG:
            cout<<"Got a hello message!"<<endl;
            break;

        case RETURN_PAUSE_STATUS_MSG:
            cout<<"Got a return pause status message!"<<endl;
            in >> systemPaused;
            if (systemPaused)
            {
                cout<<"System is paused"<<endl;
                ui->pauseStatusLine->setText("Paused");
                ui->pausePushButton->setText("Unpause System");
            }
            else
            {
                cout<<"System is not paused"<<endl;
                ui->pauseStatusLine->setText("Not Paused");
                ui->pausePushButton->setText("Pause System");
            }
            break;

        case RETURN_RENDER_SPEED_MSG:
            timeType timeMode;
            quint16  timeModeInt;

            in >> timeModeInt;
            timeMode = (timeType)timeModeInt;

            if (timeMode == realtime)
            {
                ui->timeModeComboBox->setCurrentIndex(1);
            }
            else if (timeMode == accelerated)
            {
                ui->timeModeComboBox->setCurrentIndex(0);
            }
            break;

        default:
            cout<<"In network receiveMessage, invalid message ID"<<endl;
            break;
        }
    }
}






void MainWindow::sendMessage(messageId messageType)
{
    if (!connected)
    {
        cout<<"In MainWindow::sendMessage, trying to send message when not connected!"<<endl;
        return;
    }


    QByteArray messageArray;
    QDataStream out(&messageArray, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_7);

    out << (quint16)0; // save space for message size data
    out << (quint8)messageType;


    // Pack in various extra parameters
    switch (messageType)
    {
    case ACK_MSG:
        cout<<"Sending an ACK message"<<endl;
        break;

    case PAUSE_MSG:
        cout<<"Sending a PAUSE message"<<endl;
        break;

    case EXIT_MSG:
        cout<<"Sending an EXIT message"<<endl;
        break;

    case SWITCH_RENDER_THREAD_MSG:
        cout<<"Sending a SWITCH_RENDER_THREAD message"<<endl;
        break;

    case RENDER_SPEED_MSG:
        cout<<"Sending a RENDER_SPEED message"<<endl;
        break;

    case RENDER_DATA_MSG:
        cout<<"Sending a RENDER_DATA message"<<endl;
        break;

    case GENERATION_DATA_MSG:
        cout<<"Sending a GENERATION_DATA message"<<endl;
        break;

    case HELLO_MSG:
        cout<<"Sending a hello message!"<<endl;
        break;

    case GET_PAUSE_STATUS_MSG:
        cout<<"Sending a get pause status message!"<<endl;
        break;

    case RETURN_PAUSE_STATUS_MSG:
        cout<<"Sending a return pause status message!"<<endl;
        break;

    case GET_RENDER_SPEED_MSG:
        cout<<"Sending get render speed msg!"<<endl;
        break;

    case GET_RENDER_THREAD_MSG:
        cout<<"Sending get render thread msg!"<<endl;
        break;

    default:
        cout<<"In network sendMessage, invalid message ID"<<endl;
        break;
    }


    out.device()->seek(0); // Go back to beginning, and write the message size
    out << (quint16)(messageArray.size() - sizeof(quint16));

    tcpSocket->write(messageArray);
}




void MainWindow::on_pausePushButton_clicked()
{
    sendMessage(PAUSE_MSG);
}



void MainWindow::on_exitSimButton_clicked()
{
    sendMessage(EXIT_MSG);
}


void MainWindow::on_timeModeComboBox_currentIndexChanged(int index)
{
    sendMessage(RENDER_SPEED_MSG);
}
