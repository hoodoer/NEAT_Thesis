#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QTcpSocket>
#include <QGLWidget>
#include <QGraphicsView>
#include <QGraphicsScene>


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


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_connectButton_clicked();
    void displayError(QAbstractSocket::SocketError socketError);
    void connectedSlot();
    void disconnectedSlot();

    void receiveMessage();

    void on_disconnectButton_clicked();

    void on_pushButton_clicked();

    void on_pausePushButton_clicked();

    void on_exitSimButton_clicked();

    void on_timeModeComboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QGraphicsView  *graphicsView;
    QGraphicsScene *scene;


    QTcpSocket     *tcpSocket;
    quint16         blockSize;

    bool            connected;
    bool            systemPaused;

    int             generationCounter;

    int             blueChampIndex;
    int             blueChampNumNeurons;
    int             blueChampFitnessScore;
    int             blueBestFitnessEver;
    int             blueTeamNumSpecies;

    int             redChampIndex;
    int             redChampNumNeurons;
    int             redChampFitnessScore;
    int             redBestFitnessEver;
    int             redTeamNumSpecies;


    void sendMessage(messageId messageType);
};

#endif // MAINWINDOW_H
