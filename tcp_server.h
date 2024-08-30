#pragma once

#include <QTcpServer>

class TcpServer : public QTcpServer
{
	Q_OBJECT

public:
	TcpServer(int port);
	~TcpServer();
protected:
	//客户端有新的连接时 调用
	void incomingConnection(qintptr socket_descriptor);
signals:
	void SignalTcpMsgComes(QByteArray&);
public:
	//监听
	bool run();
private slots:
	//处理数据
	void SocketDataProcessing(QByteArray& send_data, int descriptor);
	//断开连接处理
	void SocketDisconnected(int descriptor);
private:
	//端口号
	int port_;
	//链表
	QList<QTcpSocket*> tcp_socket_connectlist_;
};
