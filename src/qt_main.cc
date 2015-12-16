#include "../include/qt_extended.hh"

extern int gfflag;

int main(int argc, char *argv[])
{
    int q_state;
    QApplication qt_main(argc, argv);
    gfflag = 1;

    // main_window layout
    QMain main_window;
    main_window.setWindowTitle("UCAS Supplicant");
    main_window.setMinimumSize(260,260);

    main_window.initialize();
    main_window.set_off_layout();

    q_state = main_window.check_state();
    /*
    else 
        QDebug("Problem Connecting to Server!");
        */

    // supplementary window layout
    QWidget supp_window;
    supp_window.setWindowTitle("Message");
    supp_window.setMinimumSize(100,100);

    QLabel *message_text = new QLabel(&supp_window);
    QPushButton *message_exit = new QPushButton("Close", &supp_window);

    QVBoxLayout *layout_supp = new QVBoxLayout;
    layout_supp->addWidget(message_text,0,Qt::AlignCenter);
    layout_supp->addWidget(message_exit,0,Qt::AlignCenter);

    supp_window.setLayout(layout_supp);

    // on click of connect
    QObject::connect(&main_window, &QMain::display_message, message_text, &QLabel::setText);
    QObject::connect(&main_window, &QMain::display_message, &supp_window, &QWidget::show);
    QObject::connect(message_exit, &QPushButton::clicked, &supp_window, &QWidget::hide);
    //QObject::connect(&supp_window, &QSuppWindow::supp_closed, &main_window, &QMain::destroy_message);
    main_window.show();

    if (q_state == 1)
        main_window.send_success();

    return qt_main.exec();
}
