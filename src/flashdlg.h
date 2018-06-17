/************************************************************************//**
 * \file
 *
 * \brief Flash manager dialog class implementation.
 *
 * \defgroup flashdlg flashdlg
 * \{
 * \brief Flash manager dialog class implementation.
 *
 * Uses a dialog with a QTabWidget. Each tab is implemented in a separate
 * class.
 *
 * \author doragasu
 * \date   2017
 ****************************************************************************/

#ifndef _FLASHDLG_H_
#define _FLASHDLG_H_

#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtNetwork/QtNetwork>

/************************************************************************//**
 * Main dialog class
 ****************************************************************************/
class FlashDialog : public QDialog {
	Q_OBJECT
public:
	/// Tabs class handler
	QTabWidget *tabs;
	/// Status label
	QLabel *statusLab;
	/// Pointer to progress bar from parent
	QProgressBar *progBar;
	/// Pointer to exit button
	QPushButton *btnQuit;
    /// Connected socket
    QTcpSocket *socket;

    uint8_t id[4];

    uint8_t fwVer[2];

    uint32_t bootAddr;

	/********************************************************************//**
	 * Default constructor.
	 ************************************************************************/
	FlashDialog(QTcpSocket *socket);

private:
	/********************************************************************//**
	 * Initialize the dialog with the tabs.
	 ************************************************************************/
	void InitUI(void);

	FlashDialog(void);
};

/************************************************************************//**
 * Widget class handling the INFO tab
 ****************************************************************************/
class FlashInfoTab : public QWidget {
	Q_OBJECT
public:
	/********************************************************************//**
	 * Constructor
	 *
	 * \param[in] pointer to the owner of this class
	 ************************************************************************/
	FlashInfoTab(FlashDialog *dlg);

private:
	/// Parent dialog
	FlashDialog *dlg;

	/********************************************************************//**
	 * Initialize the tab interface
	 ************************************************************************/
	void InitUI(void);

	/********************************************************************//**
	 * Do not allow using default constructor
	 ************************************************************************/
	FlashInfoTab(void);
};

/************************************************************************//**
 * Widget class handling the ERASE tab
 ****************************************************************************/
class FlashEraseTab : public QWidget {
	Q_OBJECT
public:
	/********************************************************************//**
	 * Constructor
	 *
	 * \param[in] pointer to the owner of this class
	 ************************************************************************/
	FlashEraseTab(FlashDialog *dlg);

public slots:
	/********************************************************************//**
	 * Erases flash as specified in dialog data.
	 ************************************************************************/
	void Erase(void);

private:
	/// Parent dialog
	FlashDialog *dlg;
	/// Start of the erase range
	QLineEdit *startLe;
	/// Length of the erase range
	QLineEdit *lengthLe;
	/// Widget holding the frame range controls
	QWidget *rangeFrame;

	/********************************************************************//**
	 * Initialize the tab interface
	 ************************************************************************/
	void InitUI(void);

	/********************************************************************//**
	 * Do not allow using default constructor
	 ************************************************************************/
	FlashEraseTab(void);
};

/************************************************************************//**
 * Widget class handling the WRITE tab
 ****************************************************************************/
class FlashWriteTab : public QWidget {
	Q_OBJECT
public:
	/********************************************************************//**
	 * Constructor
	 *
	 * \param[in] pointer to the owner of this class
	 ************************************************************************/
	FlashWriteTab(FlashDialog *dlg);

public slots:
	/********************************************************************//**
	 * Programs a file to the flash chip, depending on dialog input.
	 ************************************************************************/
	void Flash(void);

	/********************************************************************//**
	 * Opens the file dialog for the user to select the file to program.
	 ************************************************************************/
	void ShowFileDialog(void);

    void Boot(void);

private:
	/// Parent dialog
	FlashDialog *dlg;
	/// File to flash
	QLineEdit *fileLe;
	/// Auto-erase checkbox
	QCheckBox *autoCb;
	/// Auto-boot checkbox
	QCheckBox *autoBootCb;

	/********************************************************************//**
	 * Initialize the tab interface
	 ************************************************************************/
	void InitUI(void);

	/********************************************************************//**
	 * Do not allow using default constructor
	 ************************************************************************/
	FlashWriteTab(void);
};

#endif /*_FLASHDLG_H_*/

/** \} */

