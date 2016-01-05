# Basic GUI for UCAS ePortal

**UCAS_supplicnat**是为Unix和类Unix系统写的客户端程序。

现在许多*C++*和*Qt*方法的错误处理还没有做完，慢速网络连接下的测试也不完整，特别是发生超时时的清理。导致程序不稳定的部分因素还没有全改好，尽管大部分情况下已经比较稳定。

此外，还有部分代码没有整理，UI现在也不好看。

-------------------

## 前置需求

- *QT5*
- *libiconv*（用于将服务器返回的GBK编码的中文转为UTF-8编码）。据我所知，在大多数Linux发行版和Mac OS中都集成了这个库，其他一些系统，如FreeBSD可能要单独安装。

## 安装

执行`qmake`(也许是 `qmake-qt5`)然后`make`。

## 使用

目前只是基本上实现了我预想当中基本的功能，登入、登出、获取一些基本的帐户信息。当程序初始化时，会检查是否已经登入网络；目前当打开本程序的一个实例之后在另外的地方登入时，不会自动检查登入状态。

当勾选`Save Username`复选框时，在执行登入时程序会尝试在`$HOME`目录下修改`.ucas_uname`文件，如果失败会尝试在当前目录下修改`.ucas_uname`文件。读取是按照同样的顺序，优先读取`$HOME`目录下保存的文件。

当本程序认为用户在线时会每隔30秒访问[icloud](http://www.icloud.com)，以减少掉线，如果访问失败，会报`Connection lost`并退回登入界面。

过长的用户名/密码没有经过完整的测试。

## 联系作者

如果有任何建议或问题，请联系[作者](mailto:gzstzsj@gmail.com)，十分感谢。

## 其他

从*UNP*上抄了一端控制连接超时时间的代码。
