#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include "flashdlg.h"
#include "con_dlg.h"

#include "flash_man.h"
#include "util.h"
#include "version.h"


FlashInfoTab::FlashInfoTab(FlashDialog *dlg) {
	this->dlg = dlg;
	InitUI();
}

void FlashInfoTab::InitUI(void) {
	QLabel *mdmaVerCaption = new QLabel("Programmer Version:");
	QLabel *mdmaVer = new QLabel(QString::asprintf("%d.%d", VERSION_MAJOR,
				VERSION_MINOR));
	mdmaVer->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *progVerCapion = new QLabel("Bootloader version:");
	progVer = new QLabel("N/A");
	progVer->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *manIdCaption = new QLabel("Flash manufacturer ID:");
	manId = new QLabel("N/A");
	manId->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *devIdCaption = new QLabel("Flash device IDs:");
	devId = new QLabel("N/A");
	devId->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QLabel *about = new QLabel("Megadrive WiFi Programmer, "
			"by doragasu, 2018");

	// Connect signals and slots
	connect(dlg->tabs, SIGNAL(currentChanged(int)), this,
			SLOT(TabChange(int)));

	// Set layout
	QHBoxLayout *aboutLayout = new QHBoxLayout;
	aboutLayout->addWidget(about);
	aboutLayout->addStretch(1);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(mdmaVerCaption);
	mainLayout->addWidget(mdmaVer);
	mainLayout->addWidget(progVerCapion);
	mainLayout->addWidget(progVer);
	mainLayout->addWidget(manIdCaption);
	mainLayout->addWidget(manId);
	mainLayout->addWidget(devIdCaption);
	mainLayout->addWidget(devId);
	mainLayout->addLayout(aboutLayout);
	mainLayout->setAlignment(Qt::AlignTop);

	setLayout(mainLayout);
}

void FlashInfoTab::TabChange(int index) {
	uint16_t err;
	uint8_t id[4];
    uint8_t ver[2];

	// If tab is the info tab, update fields
	if (index != 2) return;
	
	FlashMan fm(dlg->socket);
	err = fm.IdsGet(id);
	if (err) QMessageBox::warning(this, "Error", "Could not get IDs!");
	else {
		manId->setText(QString::asprintf("%02X", id[0]));
		devId->setText(QString::asprintf("%02X:%02X:%02X", id[1],
				id[2], id[3]));
	}
    err = fm.BootloaderVersionGet(ver);
	if (err) QMessageBox::warning(this, "Error", "Could not get IDs!");
    else {
        progVer->setText(QString::asprintf("%d.%d", ver[0], ver[1]));
    }

}

FlashEraseTab::FlashEraseTab(FlashDialog *dlg) {
	this->dlg = dlg;

	InitUI();
}

void FlashEraseTab::InitUI(void) {
	// Create widgets
	QLabel *rangeLb = new QLabel("Range to erase (bytes):");
	QLabel *startLb = new QLabel("Start: ");
	startLe = new QLineEdit("0x000000");
	QLabel *lengthLb = new QLabel("Length: ");
	lengthLe = new QLineEdit(QString::asprintf("0x%06X", FM_CHIP_LENGTH));
	QPushButton *eraseBtn = new QPushButton("Erase!");
	
	// Connect signals and slots
	connect(eraseBtn, SIGNAL(clicked()), this, SLOT(Erase()));

	// Set layout
	QHBoxLayout *rangeLayout = new QHBoxLayout;
	rangeLayout->addWidget(startLb);
	rangeLayout->addWidget(startLe);
	rangeLayout->addWidget(lengthLb);
	rangeLayout->addWidget(lengthLe);

	QVBoxLayout *rangeVLayout = new QVBoxLayout;
	rangeVLayout->addWidget(rangeLb);
	rangeVLayout->addLayout(rangeLayout);
	rangeFrame = new QWidget;
	rangeFrame->setLayout(rangeVLayout);

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(eraseBtn);
	statLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(rangeFrame);
	mainLayout->addStretch(1);
	mainLayout->addLayout(statLayout);

	setLayout(mainLayout);
}

void FlashEraseTab::Erase(void) {
	int start, len;
	int status;
	bool ok;
	FlashMan fm(dlg->socket);

	dlg->tabs->setEnabled(false);
	dlg->btnQuit->setVisible(false);

	start = startLe->text().toInt(&ok, 0);
	if (ok) len = lengthLe->text().toInt(&ok, 0);
	if (!ok || ((start + len) > FM_CHIP_LENGTH)) {
		QMessageBox::warning(this, "MDMA", "Invalid erase range!");
		return;
	}
	dlg->statusLab->setText("Erasing...");
	dlg->repaint();
	// Partial erase, with word based range
	status = fm.RangeErase(start, len);

	if (status) QMessageBox::warning(this, "Error", "Erase failed!");

	dlg->tabs->setEnabled(true);
	dlg->btnQuit->setVisible(true);
	dlg->statusLab->setText("Done!");
}


FlashWriteTab::FlashWriteTab(FlashDialog *dlg) {
	this->dlg = dlg;
	InitUI();
}

void FlashWriteTab::Boot(void) {
	FlashMan fm(dlg->socket);

    if (fm.Boot()) {
        QMessageBox::warning(this, "ERROR", "Boot failed!");
        return;
    }
    exit(0);
}

void FlashWriteTab::InitUI(void) {
	// Create widgets
	QLabel *romLab = new QLabel("Write to cart from ROM:");	
	fileLe = new QLineEdit;
	QPushButton *fOpenBtn = new QPushButton("...");
	fOpenBtn->setFixedWidth(30);
	autoCb = new QCheckBox("Auto-erase");
	autoCb->setCheckState(Qt::Checked);
	autoBootCb = new QCheckBox("Auto-boot and close after flash");
	autoBootCb->setCheckState(Qt::Checked);
	QPushButton *bootBtn = new QPushButton("Boot and close!");
	QPushButton *flashBtn = new QPushButton("Flash!");

	// Connect signals to slots
	connect(fOpenBtn, SIGNAL(clicked()), this, SLOT(ShowFileDialog()));
	connect(flashBtn, SIGNAL(clicked()), this, SLOT(Flash()));
	connect(bootBtn, SIGNAL(clicked()), this, SLOT(Boot()));

	// Configure layout
	QHBoxLayout *fileLayout = new QHBoxLayout;
	fileLayout->addWidget(fileLe);
	fileLayout->addWidget(fOpenBtn);

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(bootBtn);
	statLayout->addWidget(flashBtn);
	statLayout->setAlignment(Qt::AlignRight);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(romLab);
	mainLayout->addLayout(fileLayout);
	mainLayout->addWidget(autoCb);
	mainLayout->addWidget(autoBootCb);
	mainLayout->addStretch(1);
	mainLayout->addLayout(statLayout);

	setLayout(mainLayout);
}

void FlashWriteTab::ShowFileDialog(void) {
	QString fileName;

	fileName = QFileDialog::getOpenFileName(this, tr("Open ROM file"),
			NULL, tr("ROM Files (*.bin);;All files (*)"));
	if (!fileName.isEmpty()) fileLe->setText(fileName);
}

void FlashWriteTab::Flash(void) {
	uint8_t *wrBuf = NULL;
	uint32_t start = 0;
	uint32_t len = 0;
	bool autoErase;

	if (fileLe->text().isEmpty()) return;

    // Read file
	FlashMan fm(dlg->socket);
    wrBuf = fm.AllocFile(fileLe->text().toStdString().c_str(), &len);
    if (!wrBuf) {
        QMessageBox::warning(this, "Flash error", "Reading file failed!");
        return ;
    }


	dlg->tabs->setDisabled(true);
	dlg->btnQuit->setVisible(false);
	dlg->progBar->setVisible(true);
	// Create Flash Manager and connect signals to UI control slots
	connect(&fm, &FlashMan::RangeChanged, dlg->progBar,
			&QProgressBar::setRange);
	connect(&fm, &FlashMan::ValueChanged, dlg->progBar,
			&QProgressBar::setValue);
	connect(&fm, &FlashMan::StatusChanged, dlg->statusLab, &QLabel::setText);

	autoErase = autoCb->isChecked();
	// Start programming
	if (fm.Program(wrBuf, autoErase, start, len)) {
        fm.BufFree(wrBuf);
		dlg->progBar->setVisible(false);
		dlg->btnQuit->setVisible(true);
		dlg->tabs->setDisabled(false);
		dlg->statusLab->setText("Done!");
		disconnect(this, 0, 0, 0);
		return;
	}

    fm.BufFree(wrBuf);
	dlg->progBar->setVisible(false);
	dlg->btnQuit->setVisible(true);
	dlg->tabs->setDisabled(false);
	disconnect(this, 0, 0, 0);

    if (autoBootCb->isChecked()) {
        Boot();
    }
}

FlashDialog::FlashDialog(QTcpSocket *socket) {
    this->socket = socket;
	InitUI();
}

void FlashDialog::InitUI(void) {
	tabs = new QTabWidget;
	tabs->addTab(new FlashWriteTab(this), tr("WRITE"));
	tabs->addTab(new FlashEraseTab(this), tr("ERASE"));
	tabs->addTab(new FlashInfoTab(this),  tr("INFO"));

	statusLab = new QLabel("Ready!");
	statusLab->setFixedWidth(80);
	progBar = new QProgressBar;
	progBar->setVisible(false);
	btnQuit = new QPushButton("Exit");
	btnQuit->setDefault(true);

	connect(btnQuit, SIGNAL(clicked()), this, SLOT(close()));

	QHBoxLayout *statLayout = new QHBoxLayout;
	statLayout->addWidget(statusLab);
	statLayout->addWidget(progBar);
	statLayout->addWidget(btnQuit);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(tabs);
	mainLayout->addLayout(statLayout);

	setLayout(mainLayout);

	resize(350, 300);
	setWindowTitle("Megadrive WiFi Programmer");
}

