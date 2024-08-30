#pragma once

#include <QTcpSocket>

class TcpSocket : public QTcpSocket
{
	Q_OBJECT


public:
	TcpSocket();
	~TcpSocket();
	void run();
signals:
	//从客户端收到数据后，发射信号，告诉server有信号要处理
	void SignalGetDataFromClient(QByteArray&, int);
	void SignalClientDisconnect(int);	//告诉server 有客户端断开连接

private slots:
	//处理ready_read客户端数据
	void OnRecviveData();
	//客户端断开连接
	void OnClientDisconnect();


private:
	//描述符:用于唯一标识
	int descriptor_;
};
