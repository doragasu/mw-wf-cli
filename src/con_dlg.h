#ifndef _CON_DLG_H_
#define _CON_DLG_H_

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtNetwork/QtNetwork>

/************************************************************************//**
 * Main dialog class
 ****************************************************************************/
class ConDialog : public QDialog {
	Q_OBJECT
public:
    // Address
    QLineEdit *addr;
    // Port
    QLineEdit *port;
	/********************************************************************//**
	 * Default constructor.
	 ************************************************************************/
	ConDialog(QTcpSocket *socket);
    
public slots:
    void Connect(void);


private:
    ConDialog(void);
	/********************************************************************//**
	 * Initialize the dialog
	 ************************************************************************/
	void InitUI(void);

    QTcpSocket *socket;
};

class ConnectingDialog : public QDialog {
    Q_OBJECT

public:
        /// The socket reference
        QTcpSocket *sck;

        ConnectingDialog(QTcpSocket *s, const char *addr, uint16_t port);

private:
        ConnectingDialog(void);
        void InitUI(void);
};

#endif /*_CON_DLG_H_*/

