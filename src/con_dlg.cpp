#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QMessageBox>

#include "con_dlg.h"

ConDialog::ConDialog(QTcpSocket *socket) {
    this->socket = socket;
	InitUI();
}

void ConDialog::Connect(void) {
    int nPort;
    nPort = atoi(port->text().toStdString().c_str());
    if ((nPort <= 0) || (nPort > UINT16_MAX)) {
        
		QMessageBox::warning(this, "Connection error", "Invalid port number");
        return;
    }
    ConnectingDialog cDlg(socket, addr->text().toStdString().c_str(), nPort);
    if (!cDlg.exec()) {
		QMessageBox::warning(this, "Connection error", "Connection failed");
        return;
    }

    accept();
}

void ConDialog::InitUI(void) {
	QLabel *addrLabel = new QLabel("Address and port:");
    addr = new QLineEdit("192.168.1.60");
    QLabel *separatorLabel = new QLabel(":");
    port = new QLineEdit("1989");
    port->setFixedWidth(50);

    QPushButton *connBtn = new QPushButton("CONNECT");
    QPushButton *exitBtn = new QPushButton("EXIT");

	connect(exitBtn, SIGNAL(clicked()), this, SLOT(reject()));
	connect(connBtn, SIGNAL(clicked()), this, SLOT(Connect()));

	QHBoxLayout *addrLayout = new QHBoxLayout;
    addrLayout->addWidget(addr);
    addrLayout->addWidget(separatorLabel);
    addrLayout->addWidget(port);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(connBtn);
    btnLayout->addWidget(exitBtn);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(addrLabel);
	mainLayout->addLayout(addrLayout);
	mainLayout->addLayout(btnLayout);

	setLayout(mainLayout);

	setWindowTitle("Mega WiFi connection");
    this->layout()->setSizeConstraint( QLayout::SetFixedSize );
}

ConnectingDialog::ConnectingDialog(QTcpSocket *s, const char *addr,
        uint16_t port) {
    sck = s;
    connect(sck, SIGNAL(connected()), this, SLOT(accept()));
    connect(sck, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(reject()));
    connect(sck, SIGNAL(disconnected()), this, SLOT(reject()));

    InitUI();

    sck->connectToHost(addr, port);
}

void ConnectingDialog::InitUI(void) {
    QLabel *conLabel = new QLabel("Connecting...");
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(conLabel);

	setLayout(mainLayout);

	setWindowTitle("Mega WiFi connection");
    this->layout()->setSizeConstraint( QLayout::SetFixedSize );
}

