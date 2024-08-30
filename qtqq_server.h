#pragma once

#include <QtWidgets/QDialog>
#include "ui_qtqqserver.h"
#include <qudpsocket.h>
#include <QSqlQueryModel>
#include <QTimer>
#include "tcp_server.h"
class QtqqServer : public QDialog
{
    Q_OBJECT

public:
    QtqqServer(QWidget *parent = nullptr);
    ~QtqqServer();
    
private:
    void InitComboBoxData();  //初始化组合框数据
    void InitTcpSocket();   //初始化TCP
    void InitUdpSocket();   //初始化UDP
    bool ConnectDatabase();

    int GetComDepID();          //公司部门ID
    void SetDepNameMap();
    void SetStatusMap();
    void SetOnlineMap();
    void UpdateTableData(int dep_id = 0,int employ_id = 0);
   
private slots:
    void OnUdpBroadMsg(QByteArray& bt_data);
    void OnRefresh();
    void on_query_department_btn_clicked();     //点击信息与槽函数自动连接
    void on_queryid_btn_clicked();              //根据员工QQ号筛选
    void on_logout_btn_clicked();               //注销员工QQ号
    void on_select_picture_btn_clicked();       //选择图片员工的寸照
    void on_add_btn_clicked();                  //新增员工
private:
    Ui::QtqqServerClass ui;
    QTimer* timer_; //定时刷新数据
    int compony_depid_;                   //公司群QQ号
    int dep_id_;                           //部门QQ号
    int employee_id_;                      //员工QQ号

    QString pic_path_;                    //头像路径
    QMap<QString, QString> status_map_;   //状态
    QMap<QString, QString> dep_name_map_; //部门名称
    QMap<QString, QString> online_map_;   //在线
    TcpServer* tcp_server_;              //tcp服务器 
    QUdpSocket* udp_sender_;            //tcp服务器 
    //查询所有员工信息模型
    QSqlQueryModel queryinfo_model_;

};

