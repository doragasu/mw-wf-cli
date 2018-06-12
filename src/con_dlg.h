#ifndef _CON_DLG_H_
#define _CON_DLG_H_

#include <QtWidgets/QDialog>

/************************************************************************//**
 * Main dialog class
 ****************************************************************************/
class ConDialog : public QDialog {
	Q_OBJECT
public:
	/********************************************************************//**
	 * Default constructor.
	 ************************************************************************/
	ConDialog(void);

private:
	/********************************************************************//**
	 * Initialize the dialog
	 ************************************************************************/
	void InitUI(void);
};

#endif /*_CON_DLG_H_*/
