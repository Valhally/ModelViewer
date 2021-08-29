#pragma once

#define QDBG qDebug()<<__FILE__<<__FUNCTION__<<__LINE__

#include <QApplication>
#include <QEvent>
#include "qevent.h"
#include "Renderer.h"
#include "QFileDialog"

class GlobalApplication : public QApplication
{
public:
	GlobalApplication(int& argc, char** argv);

	bool notify(QObject*, QEvent*) override;
	void setWindowInstance(QWidget* widget);
	void SetCamera(std::shared_ptr<Camera> camera);
	void SetRenderer(std::shared_ptr<Renderer> renderer);

private:
	QWidget* widget;

	std::shared_ptr<Renderer> renderer;
	std::shared_ptr<Camera> camera;

	bool transform = false;
	bool rotate = false;

	POINT curPos;
};