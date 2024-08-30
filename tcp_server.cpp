#include "tcp_server.h"
#include "qtcpsocket.h"
#include "tcp_socket.h"
TcpServer::TcpServer(int port)
	:port_(port)
{}

TcpServer::~TcpServer()
{}

void TcpServer::incomingConnection(qintptr socket_descriptor) {
	qDebug() << QString::fromLocal8Bit("新的连接：") << socket_descriptor << endl;
	TcpSocket* tcp_socket = new TcpSocket();

	tcp_socket->setSocketDescriptor(socket_descriptor);
	tcp_socket->run();
	//收到客户端数据后， 服务器进行处理
	connect(tcp_socket, SIGNAL(SignalGetDataFromClient(QByteArray&, int)), this, SLOT(SocketDataProcessing(QByteArray&, int)));
	//收到客户端断开连接，server进行处理	
	connect(tcp_socket, SIGNAL(SignalClientDisconnect(int)), this, SLOT(SocketDisconnected(int)));
	//socket 添加到链表中
	tcp_socket_connectlist_.append(tcp_socket);
}

bool TcpServer::run() {
	//服务器监听
	if (this->listen(QHostAddress::AnyIPv4, port_)) {
		qDebug() << QString::fromLocal8Bit("服务端监听端口%1 成功").arg(port_);
		return true;
	}
	else {
		qDebug() << QString::fromLocal8Bit("服务端监听端口%1 失败").arg(port_);
		return false;
	}
}

void TcpServer::SocketDisconnected(int descriptor) {
	for (int i = 0; i < tcp_socket_connectlist_.count(); ++i) {
		QTcpSocket* item = tcp_socket_connectlist_.at(i);
		int item_descriptor = item->socketDescriptor();
		//查找断开连接的socket
		if (item_descriptor == descriptor || item_descriptor == -1) {
			tcp_socket_connectlist_.removeAt(i);//断开的socket 从链表中移除
			item->deleteLater(); //回收资源
			qDebug() << QString::fromLocal8Bit("TcpSocket断开连接：") << descriptor << endl;
			return;
		}
	}
}

void TcpServer::SocketDataProcessing(QByteArray& send_data, int descriptor) {
	for (int i = 0; i < tcp_socket_connectlist_.count(); i++)
	{
		QTcpSocket* item = tcp_socket_connectlist_.at(i);
		if (item->socketDescriptor() == descriptor) {
			qDebug() << QString::fromLocal8Bit("来自IP:") << item->peerAddress().toString() << QString::fromLocal8Bit("发来的数据：") << QString(send_data);
			emit SignalTcpMsgComes(send_data);
		}
	}
}
