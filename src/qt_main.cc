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

    // supplementary window layout
    QSuppWindow supp_window;
    supp_window.initialize();
    QErrorWindow error_window;
    error_window.initialize();

    // on click of connect
    QObject::connect(&main_window, &QMain::display_message, &supp_window, &QSuppWindow::showup);
    QObject::connect(&main_window, &QMain::display_message, &supp_window, &QSuppWindow::show);
    QObject::connect(&main_window, &QMain::show_error, &error_window, &QErrorWindow::showup);
    QObject::connect(&main_window, &QMain::show_error, &error_window, &QErrorWindow::show);

    main_window.initialize();
    main_window.set_off_layout();

    q_state = main_window.check_state();

    main_window.show();
    if (q_state == 1)
        main_window.send_success();

    return qt_main.exec();
}
