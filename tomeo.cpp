/*
 *
 *    ______
 *   /_  __/___  ____ ___  ___  ____
 *    / / / __ \/ __ `__ \/ _ \/ __ \
 *   / / / /_/ / / / / / /  __/ /_/ /
 *  /_/  \____/_/ /_/ /_/\___/\____/
 *              video for sports enthusiasts...
 *
 *  2811 cw3 : twak
 */

#include <iostream>
#include <QApplication>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QMediaPlaylist>
#include <string>
#include <vector>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>
#include <QtCore/QFileInfo>
#include <QtWidgets/QFileIconProvider>
#include <QDesktopServices>
#include <QImageReader>
#include <QMessageBox>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include "the_player.h"
#include "the_button.h"
#include "media_buttons.h"
#include "timestamp.h"
#include "scrubber.h"
#include <QSlider>
#include <QLabel>
#include <QDebug>

using namespace std;

// read in videos and thumbnails to this directory
vector<TheButtonInfo> getInfoIn (string loc) {

    vector<TheButtonInfo> out =  vector<TheButtonInfo>();
    QDir dir(QString::fromStdString(loc) );
    QDirIterator it(dir);
    unsigned long i = 0;
    while (it.hasNext()) { // for all files

        QString f = it.next();

            if (f.contains("."))

#if defined(_WIN32)
            if (f.contains(".wmv"))  { // windows
#else
            if (f.contains(".mp4") || f.contains("MOV"))  { // mac/linux
#endif

            QString thumb = f.left( f .length() - 4) +".png";
            if (QFile(thumb).exists()) { // if a png thumbnail exists
                QImageReader *imageReader = new QImageReader(thumb);
                    QImage sprite = imageReader->read(); // read the thumbnail
                    if (!sprite.isNull()) {
                        QIcon* ico = new QIcon(QPixmap::fromImage(sprite)); // voodoo to create an icon for the button
                        QUrl* url = new QUrl(QUrl::fromLocalFile( f )); // convert the file location to a generic url
                        out . push_back(TheButtonInfo( url , ico, i) ); // add to the output list
                        i++;
                    }
                    else
                        qDebug() << "warning: skipping video because I couldn't process thumbnail " << thumb << Qt::endl;
            }
            else
                qDebug() << "warning: skipping video because I couldn't find thumbnail " << thumb << Qt::endl;
        }
    }

    return out;
}


int main(int argc, char *argv[]) {

    // let's just check that Qt is operational first
    qDebug() << "Qt version: " << QT_VERSION_STR << Qt::endl;

    // create the Qt Application
    QApplication app(argc, argv);

    // collect all the videos in the folder
    vector<TheButtonInfo> videos;

    if (argc == 2)
        videos = getInfoIn( string(argv[1]) );

    if (videos.size() == 0) {

        const int result = QMessageBox::question(
                    NULL,
                    QString("Tomeo"),
                    QString("no videos found! download, unzip, and add command line argument to \"quoted\" file location. Download videos from Tom's OneDrive?"),
                    QMessageBox::Yes |
                    QMessageBox::No );

        switch( result )
        {
        case QMessageBox::Yes:
          QDesktopServices::openUrl(QUrl("https://leeds365-my.sharepoint.com/:u:/g/personal/scstke_leeds_ac_uk/EcGntcL-K3JOiaZF4T_uaA4BHn6USbq2E55kF_BTfdpPag?e=n1qfuN"));
          break;
        default:
            break;
        }
        exit(-1);
    }

    // the widget that will show the video
    QVideoWidget *videoWidget = new QVideoWidget;

    // the QMediaPlayer which controls the playback
    ThePlayer *player = new ThePlayer();
    player->setVideoOutput(videoWidget);

    // a row of buttons
    QWidget *buttonWidget = new QWidget();
    // a list of the buttons
    vector<TheButton*> buttons;
    // the buttons are arranged horizontally
    QHBoxLayout *layout = new QHBoxLayout();
    buttonWidget->setLayout(layout);

    Media_Buttons *back = new Media_Buttons(buttonWidget);
    back->connect(back, SIGNAL(released()),player, SLOT(prevButtons()));
    back->setIcon(QIcon(":/stop.png")); //placeholder icon
    layout->addWidget(back);

    // create the four buttons
    for ( int i = 0; i < 4; i++ ) {
        TheButton *button = new TheButton(buttonWidget);
        button->connect(button, SIGNAL(jumpTo(TheButtonInfo* )), player, SLOT (jumpTo(TheButtonInfo* ))); // when clicked, tell the player to play.
        buttons.push_back(button);
        layout->addWidget(button,1);
        button->init(&videos.at(i));
    }
    //create next button
    Media_Buttons *next = new Media_Buttons(buttonWidget);
    next->connect(next, SIGNAL(released()),player, SLOT(nextButtons()));
    next->setIcon(QIcon(":/stop.png")); //placeholder icon
    layout->addWidget(next);

    //create buttons
    QWidget *playbackWidget = new QWidget();
    QHBoxLayout *layout2 = new QHBoxLayout();
    //Play pause button
    Media_Buttons *pp = new Media_Buttons(playbackWidget);
    pp->connect(pp, SIGNAL(released()), pp, SLOT(playClicked()));
    pp->connect(pp, SIGNAL(play()), player, SLOT(play()));
    pp->connect(pp, SIGNAL(pause()), player, SLOT(pause()));
    pp->setIcon(QIcon(":/pause.png"));
    layout2->addWidget(pp);
    //Stop button
    Media_Buttons *stop = new Media_Buttons(playbackWidget);
    stop->connect(stop, SIGNAL(released()),player, SLOT(stop()));
    stop->setIcon(QIcon(":/stop.png"));
    layout2->addWidget(stop);

    //Mute button
    Media_Buttons *mute = new Media_Buttons(playbackWidget);
    mute->connect(mute, SIGNAL(released()), mute, SLOT(muteClicked()));
    mute->connect(mute, SIGNAL(setMuted(bool)), player, SLOT(setMuted(bool)));
    mute->setIcon(QIcon(":/mute.png"));
    layout2->addWidget(mute);

    //Volume slider
    QSlider *volume = new QSlider(Qt::Horizontal);
    volume->setRange(0, 100);
    volume->connect(volume, SIGNAL(valueChanged(int)), player, SLOT(setVolume(int)));
    volume->setValue(50);
    layout2->addWidget(volume,2);
    //Timestamp label
    Timestamp *timestamp = new Timestamp();
    player->connect(player, SIGNAL(positionChanged(qint64)), timestamp, SLOT(positionChanged(qint64)));
    player->connect(player, SIGNAL(durationChanged(qint64)), timestamp, SLOT(durationChanged(qint64)));
    layout2->addWidget(timestamp);
    //Video scrubber
    Scrubber *scrubber = new Scrubber(player);
    player->connect(player, SIGNAL(positionChanged(qint64)), scrubber, SLOT(positionChanged(qint64)));
    player->connect(player, SIGNAL(durationChanged(qint64)), scrubber, SLOT(durationChanged(qint64)));
    scrubber->connect(scrubber, SIGNAL(valueChanged(int)), scrubber, SLOT(scrubberChanged(int)));
    scrubber->connect(scrubber, SIGNAL(sliderPressed()), scrubber, SLOT(scrubberPress()));
    scrubber->connect(scrubber, SIGNAL(sliderReleased()), scrubber, SLOT(scrubberUnpress()));
    scrubber->connect(scrubber, SIGNAL(scrubberPos(qint64)), player, SLOT(setPosition(qint64)));
    layout2->addWidget(scrubber,25);

    playbackWidget->setLayout(layout2);
    // tell the player what buttons and videos are available
    player->setContent(&buttons, & videos);

    // create the main window and layout
    QWidget window;
    QVBoxLayout *top = new QVBoxLayout();
    window.setLayout(top);
    window.setWindowTitle("Tomeo");
    window.setMinimumSize(800, 680);


    // add the video and the buttons to the top level widget
    top->addWidget(videoWidget);
    top->addWidget(playbackWidget);
    top->addWidget(buttonWidget);
    app.setWindowIcon(QIcon(":/logo.png"));
    // showtime!
    window.show();

    // wait for the app to terminate
    return app.exec();
}
