#include "../include/qt_extended.hh"
#include "../include/connect.h"

char info_text[MAXLINE];

void QMain::initialize()
{
    // Text
    userNamePrompt.setText("Username:");
    passWordPrompt.setText("Password:");
    userName.setPlaceholderText("username");
    passWord.setPlaceholderText("password");
    passWord.setEchoMode(QLineEdit::Password);
    submit.setText("Connect");
    storeUname.setText("Save Username");
    submit_dis.setText("Disconnect");
    combo.setText("Package\nBalance");
    combo.setMinimumHeight(40);
    flow.setText("Data");
    flow.setMinimumHeight(40);
    device.setText("Devices\nOnline");
    device.setMinimumHeight(40);
    info_display_scroll.setWidget(&info_display);
    info_display_scroll.setWidgetResizable(true);
    info_display_scroll.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    info_display.setWordWrap(true);
    //info_display.setMinimumSize(100,250);
    info_display.setAlignment(Qt::AlignTop);
    info_display.setIndent(-30);
    get_confirmed = 0;

    // Part of Layout
    username_title.addWidget(&userNamePrompt, 0, Qt::AlignLeft);
    username_title.addWidget(&storeUname, 0, Qt::AlignRight);
    info_area.addWidget(&combo, 0, 0);
    info_area.addWidget(&flow, 0, 0);
    info_area.addWidget(&device, 0, 0);

    // Connect
    QObject::connect(this, &QMain::display_success, this, &QMain::redraw_online);
    QObject::connect(this, &QMain::display_success, this, &QMain::st_keep_alive);
    QObject::connect(this, &QMain::logoff_success, this, &QMain::redraw_offline);
    QObject::connect(this, &QMain::info_success, this, &QMain::show_info);
    QObject::connect(this, &QMain::login_fail, this, &QMain::set_usable_field);
    QObject::connect(&userName, &QLineEdit::returnPressed, &submit, &QPushButton::click);
    QObject::connect(&passWord, &QLineEdit::returnPressed, &submit, &QPushButton::click);
    QObject::connect(&submit, &QPushButton::clicked, this, &QMain::login_thread);
    QObject::connect(&submit_dis, &QPushButton::clicked, this, &QMain::logout_thread);
    QObject::connect(&userName, &QLineEdit::textChanged, this, &QMain::get_username);
    QObject::connect(&passWord, &QLineEdit::textChanged, this, &QMain::get_password);
    QObject::connect(&combo, &QPushButton::clicked, this, &QMain::get_info_1_thread);
    QObject::connect(&flow, &QPushButton::clicked, this, &QMain::get_info_2_thread);
    QObject::connect(&device, &QPushButton::clicked, this, &QMain::get_info_3_thread);

    // Exit
    QObject::connect(this, &QMain::close_all_window, &QApplication::quit);
}

void QMain::set_off_layout()
{
    main_layout = new QVBoxLayout;
    main_layout->addLayout(&username_title);
    main_layout->addWidget(&userName,0,Qt::AlignTop);
    main_layout->addWidget(&passWordPrompt,0,Qt::AlignTop);
    main_layout->addWidget(&passWord,0,Qt::AlignTop);
    main_layout->addWidget(&submit,1,Qt::AlignTop);
    userName.setReadOnly(false);
    passWord.setReadOnly(false);
    userNamePrompt.setVisible(1);
    storeUname.setVisible(1);
    userName.setVisible(1);
    passWordPrompt.setVisible(1);
    passWord.setVisible(1);
    submit.setVisible(1);
    setLayout(main_layout);
    submit_dis.clearFocus();
    submit.setFocus();
}

void QMain::set_on_layout()
{
    main_layout = new QVBoxLayout;
    main_layout->addWidget(&rem_flow,0,Qt::AlignTop);
    //main_layout->addWidget(&info_display_scroll,100,Qt::AlignTop);
    main_layout->addWidget(&info_display_scroll,0,0);
    main_layout->addWidget(&submit_dis,0,Qt::AlignBottom);
    main_layout->addLayout(&info_area);
    getflow_thread();
    rem_flow.setVisible(1);
    info_display_scroll.setVisible(1);
    info_display.setVisible(1);
    submit_dis.setVisible(1);
    combo.setVisible(1);
    flow.setVisible(1);
    device.setVisible(1);
    setLayout(main_layout);
    submit.clearFocus();
    submit_dis.setFocus();
}

void QMain::del_off_layout()
    {
        main_layout->removeItem(&username_title);
        main_layout->removeWidget(&userName);
        main_layout->removeWidget(&passWordPrompt);
        main_layout->removeWidget(&passWord);
        main_layout->removeWidget(&submit);
        if (!remuname)
        {
            userName.setText("");
            get_username(userName.text());
        }
        passWord.setText("");
        get_password(passWord.text());
        userNamePrompt.setVisible(0);
        storeUname.setVisible(0);
        userName.setVisible(0);
        passWordPrompt.setVisible(0);
        passWord.setVisible(0);
        submit.setVisible(0);
        delete main_layout;
    }

void QMain::del_on_layout()
    {
        main_layout->removeWidget(&rem_flow);
        main_layout->removeWidget(&info_display_scroll);
        main_layout->removeWidget(&submit_dis);
        main_layout->removeItem(&info_area);
        rem_flow.setVisible(0);
        info_display_scroll.setVisible(0);
        info_display.setVisible(0);
        info_display.setText("");
        submit_dis.setVisible(0);
        combo.setVisible(0);
        flow.setVisible(0);
        device.setVisible(0);
        delete main_layout;
    }

void QMain::closeEvent(QCloseEvent *event)
{
    emit close_all_window();
}

void QMain::login_thread()
{
    set_remu(storeUname.isChecked());
    userName.setReadOnly(true);
    passWord.setReadOnly(true);
    pthread_t tid;
    pthread_create(&tid, NULL, login, (void*)this);
}

void QMain::logout_thread()
{
    pthread_t tid;
    pthread_create(&tid, NULL, logout, (void*)this);
}

void QMain::getflow_thread()
{
    pthread_t tid;
    pthread_create(&tid, NULL, getflow, (void*)this);
}

void QMain::get_info_1_thread()
{
    pthread_t tid;
    pthread_create(&tid, NULL, get_info_1, (void*)this);
}

void QMain::get_info_2_thread()
{
    pthread_t tid;
    pthread_create(&tid, NULL, get_info_2, (void*)this);
}

void QMain::get_info_3_thread()
{
    pthread_t tid;
    pthread_create(&tid, NULL, get_info_3, (void*)this);
}

void QMain::show_info()
{
    //info_display.setText(QString("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; text-indent:20px;\">").append(info_str).append("</p>"));
    info_display.setText(info_text);
}

void QMain::st_keep_alive()
{
    pthread_t tid;
    pthread_create(&tid, NULL, keep_alive, (void*)this);
}
