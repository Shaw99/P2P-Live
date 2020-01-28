#pragma once
#include <QMainWindow>
#include<string>
#include<iostream>
extern "C" {
#include<libavdevice/avdevice.h>
#include<libavformat/avformat.h>
#include<libavutil/avutil.h>
#include<libavutil/time.h>
}
using namespace std;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = 0);
    //输入流 输出流 错误退出
    int InInit(char* in_url,AVFormatContext *&in_avfc);
    int OutInit(char* out_url,AVFormatContext *&out_avfc);
    int ErrorExit(int errorNum);

    ~MainWindow();

  private:
    Ui::MainWindow *ui;
};
