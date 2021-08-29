#pragma once

#include <QWidget>
#include "Renderer.h"
#include "GlobalApplication.h"
#define D3DFVF_SCUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

class GDXWidget : public QWidget
{
	Q_OBJECT
public:
	GDXWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);

	virtual QPaintEngine* paintEngine() const { return nullptr; }

	void InitD3D(GlobalApplication* app);

	std::shared_ptr<Renderer> GetRenderer();

protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

private:
	std::shared_ptr<Renderer> renderer;
};