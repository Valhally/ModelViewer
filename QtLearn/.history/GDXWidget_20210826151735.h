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

public slots:
	void Render();
	void SwitchModel();
	void SaveModel();
	void SwitchPoint();
	void SwitchLine();
	void SwitchFace();
	void SwitchSolid();
	void SwitchUp();
	void SwitchDown();
	void SwitchLeft();
	void SwitchRight();
	void SwitchFront();
	void SwitchBack();
	void ChangeLight(int);

protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

private:
	std::shared_ptr<Renderer> renderer;
};