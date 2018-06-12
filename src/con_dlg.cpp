#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtWidgets/QPushButton>

#include "con_dlg.h"

/********************************************************************//**
 * Default constructor.
 ************************************************************************/
ConDialog::ConDialog(void) {
	InitUI();
}

/********************************************************************//**
 * Initialize the dialog with the tabs.
 ************************************************************************/
void ConDialog::InitUI(void) {
	QLabel *addrLabel = new QLabel("Address and port:");
    QLineEdit *addr = new QLineEdit("192.168.1.60");
    QLabel *separatorLabel = new QLabel(":");
    QLineEdit *port = new QLineEdit("1989");
    port->setFixedWidth(50);

    QPushButton *connBtn = new QPushButton("CONNECT");
    QPushButton *exitBtn = new QPushButton("EXIT");

	connect(exitBtn, SIGNAL(clicked()), this, SLOT(close()));

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


