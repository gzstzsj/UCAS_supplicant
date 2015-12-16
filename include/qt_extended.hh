#ifndef __QT_EXTENDED__H__
#define __QT_EXTENDED__H__
#include <QApplication>
#include <QObject>
#include <QWidget>
#include <QString>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QString>
#include <QScrollArea>
#include <QDebug>
#define len_username 20
#define len_password 1024

class QMain : public QWidget
{
    Q_OBJECT
public:
    QMain(){}

    static void *login(void *arg);
    void login_thread();
    static void *logout(void *arg);
    void logout_thread();

    void initialize();
    void getflow_thread();
    static void *getflow(void *arg);
    void st_keep_alive();
    static void *keep_alive(void *arg);
    //int check_state_thread();
    int check_state();
    void get_info_1_thread();
    void get_info_2_thread();
    void get_info_3_thread();
    static void *get_info_1(void *arg);
    static void *get_info_2(void *arg);
    static void *get_info_3(void *arg);
    void set_off_layout();
    void set_on_layout();
    void del_off_layout();
    void del_on_layout();
    void set_remu(int remu)
    {
        remuname = remu;
    }

    int get_confirmed;

    void send()
    {
        emit display_message(message_server);
    }

    void send_success()
    {
        emit display_success();
    }

    void send_logoff_success()
    {
        emit logoff_success();
    }

    void send_info()
    {
        emit info_success();
    }

    void send_fail()
    {
        emit login_fail();
    }

signals:
    void display_message(const QString & message);
    void display_success();
    void logoff_success();
    void close_all_window();
    void info_success();
    void login_fail();

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    int isOffline;
    int retcode;
    int remuname;
    QString message_server;
    QByteArray username;
    QByteArray password;

    /* layout when offline */
    QHBoxLayout username_title;
    QLabel userNamePrompt;
    QLabel passWordPrompt;
    QLineEdit userName;
    QLineEdit passWord;
    QPushButton submit;
    QCheckBox storeUname;

    /* layout when online */
    QHBoxLayout info_area;
    QPushButton submit_dis;
    QPushButton combo;
    QPushButton flow;
    QPushButton device;
    QScrollArea info_display_scroll;
    QLabel info_display;
    QLabel rem_flow;

    QVBoxLayout *main_layout;

    void show_info();

    void get_username(const QString & uname)
    {
        username = uname.toLatin1();
    }
    
    void get_password(const QString & pword)
    {
        password = pword.toLatin1();
    }

    void redraw_online()
    {
        del_off_layout();
        set_on_layout();
    }
        
    void redraw_offline()
    {
        del_on_layout();
        set_off_layout();
    }

    void set_usable_field()
    {
        userName.setReadOnly(false);
        passWord.setReadOnly(false);
    }

};

#endif
