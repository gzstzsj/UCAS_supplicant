# Basic GUI for UCAS ePortal

这只是一个内测版本，有很多错误处理没有做完，因而有时不稳定，有很多错误消息向标准输出（如果你在前台运行）输出，还有大量untidy的代码没有整理，UI现在也不好看。
只是基本上实现了我预想当中基本的功能，登入、登出、获取一些基本的帐户信息。
目前当打开本程序的一个实例之后在另外的地方登入时，不会自动检查登入状态。
当本程序认为用户在线时会每隔30秒访问[icloud](http://www.icloud.com)，以减少掉线，并据此检查是否已掉线。
如果有任何建议或问题，请联系[作者](mailto:gzstzsj@gmail.com)。

-------------------

## Prerequisite

需要QT5和libiconv（用于将服务器返回的GBK编码的中文转为UTF-8编码），后者好像在Linux和Mac OS中都集成了，其他一些系统譬如FreeBSD好像要单独安装。

## Installation

Run `qmake` (possibly `qmake-qt5`) then `make`.
