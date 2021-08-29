#include <QtWidgets/QApplication>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QBoxLayout>
#include "QFile"
#include "QRadioButton"
#include "QMenuBar"
#include "QAction"
#include "QScreen"
#include "QStatusBar"
#include "QFont"

#include "GDXWidget.h"
#include "GlobalApplication.h"

#define chinese(x) QString::fromLocal8Bit(x)

QWidget* BuildWindow(GlobalApplication&);

int main(int argc, char *argv[])
{
    GlobalApplication app(argc, argv);

	QWidget* window = BuildWindow(app);
	window->show();

    return app.exec();
}

QString getSytleSheet()
{
	QFile file("styles/styles.qss");
	file.open(QIODevice::ReadOnly);
	QString str = file.readAll();
	file.close();
	return str;
}

QWidget* BuildWindow(GlobalApplication& app)
{
	GDXWidget* rendererWindow = new GDXWidget();
	rendererWindow->setWindowTitle("ModelViewer");
	rendererWindow->setBaseSize(1400, 800);
	rendererWindow->InitD3D(&app);

	QMenuBar* menubar = new QMenuBar;
	QMenu* fileMenu = new QMenu(chinese("�ļ�"));
	QMenu* displayMenu = new QMenu(chinese("��ʾģʽ"));
	QMenu* viewMenu = new QMenu(chinese("�ӽ�"));
	QStatusBar* statusBar = new QStatusBar;

	QAction* loadAction  = new QAction(chinese("����ģ��"));
	QAction* saveAction  = new QAction(chinese("����ģ��"));

	QAction* pointAction = new QAction(chinese("����ӻ�"));
	QAction* lineAction  = new QAction(chinese("�߿��ӻ�"));
	QAction* faceAction  = new QAction(chinese("����ӻ�"));
	QAction* solidAction = new QAction(chinese("ƽ����ɫ"));

	QAction* upAction    = new QAction(chinese("����"));
	QAction* downAction  = new QAction(chinese("����"));
	QAction* leftAction  = new QAction(chinese("����"));
	QAction* rightAction = new QAction(chinese("����"));
	QAction* frontAction = new QAction(chinese("ǰ��"));
	QAction* backAction  = new QAction(chinese("����"));

	fileMenu->addAction(loadAction);
	fileMenu->addAction(saveAction);

	displayMenu->addAction(pointAction);
	displayMenu->addAction(lineAction);
	displayMenu->addAction(faceAction);
	displayMenu->addAction(solidAction);

	viewMenu->addAction(upAction);
	viewMenu->addAction(downAction);
	viewMenu->addAction(leftAction);
	viewMenu->addAction(rightAction);
	viewMenu->addAction(frontAction);
	viewMenu->addAction(backAction);

	QObject::connect(loadAction,  SIGNAL(triggered()), rendererWindow, SLOT(SwitchModel()));
	QObject::connect(pointAction, SIGNAL(triggered()), rendererWindow, SLOT(SwitchPoint()));
	QObject::connect(lineAction,  SIGNAL(triggered()), rendererWindow, SLOT(SwitchLine()));
	QObject::connect(faceAction,  SIGNAL(triggered()), rendererWindow, SLOT(SwitchFace()));
	QObject::connect(solidAction, SIGNAL(triggered()), rendererWindow, SLOT(SwitchSolid()));
	QObject::connect(saveAction,  SIGNAL(triggered()), rendererWindow, SLOT(SaveModel()));
	QObject::connect(upAction,    SIGNAL(triggered()), rendererWindow, SLOT(SwitchUp()));
	QObject::connect(downAction,  SIGNAL(triggered()), rendererWindow, SLOT(SwitchDown()));
	QObject::connect(leftAction,  SIGNAL(triggered()), rendererWindow, SLOT(SwitchLeft()));
	QObject::connect(rightAction, SIGNAL(triggered()), rendererWindow, SLOT(SwitchRight()));
	QObject::connect(frontAction, SIGNAL(triggered()), rendererWindow, SLOT(SwitchFront()));
	QObject::connect(backAction,  SIGNAL(triggered()), rendererWindow, SLOT(SwitchBack()));

	menubar->addMenu(fileMenu);
	menubar->addMenu(displayMenu);
	menubar->addMenu(viewMenu);

	//statusBar->showMessage("Hello, World!");
	statusBar->setFixedHeight(25);

	QString styleSheet = getSytleSheet();

	menubar->setStyleSheet(styleSheet);
	fileMenu->setStyleSheet(styleSheet);
	viewMenu->setStyleSheet(styleSheet);
	displayMenu->setStyleSheet(styleSheet);
	statusBar->setStyleSheet(styleSheet);

	QSlider* slider = new QSlider;
	slider->setRange(0, 6000000);
	slider->setSliderPosition(2500000);
	slider->setFixedSize(160, 20);
	slider->setOrientation(Qt::Horizontal);
	statusBar->addWidget(new QLabel(chinese("��������: ")));
	statusBar->addWidget(slider);

	QObject::connect(slider, SIGNAL(valueChanged(int)), rendererWindow, SLOT(ChangeLight(int)));

	QLabel* label = new QLabel;
	statusBar->addPermanentWidget(label);
	statusBar->setContentsMargins(0, 0, 0, 0);
	rendererWindow->GetRenderer()->infoLabel = label;
	

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->setMenuBar(menubar);
	layout->addWidget(rendererWindow);
	layout->addWidget(statusBar);

	QWidget* window = new QWidget;
	window->setLayout(layout);

	QScreen* screen = QGuiApplication::primaryScreen();
	auto screenRect = screen->availableGeometry();

	int initWidth = 1400, initHeight = 800;

	QRect newRect = QRect(
		(screenRect.width() - initWidth) / 2, 
		(screenRect.height() - initHeight) / 2 - 20,
		initWidth,
		initHeight );
	window->setGeometry(newRect);
	window->setStyleSheet(styleSheet);
	return window;
}

