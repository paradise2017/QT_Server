#include "tcp_socket.h"

TcpSocket::TcpSocket()
{}

TcpSocket::~TcpSocket()
{}

void TcpSocket::run() {
	descriptor_ = this->socketDescriptor();
	connect(this, SIGNAL(readyRead()), this, SLOT(OnRecviveData()));
	connect(this, SIGNAL(disconnected()), this, SLOT(OnClientDisconnect()));
}

void TcpSocket::OnClientDisconnect()
{
	emit SignalClientDisconnect(descriptor_);
}

void TcpSocket::OnRecviveData() {
	QByteArray buffer = this->readAll();
	if (!buffer.isEmpty()) {
		QString str_data = QString::fromLocal8Bit(buffer);
		//发射接受到的客户端信号
		emit SignalGetDataFromClient(buffer, descriptor_);
	}
}
