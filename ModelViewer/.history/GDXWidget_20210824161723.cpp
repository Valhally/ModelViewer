#include "GDXWidget.h"
#include "strsafe.h"
#include <QResizeEvent>
#include <QTimer>

GDXWidget::GDXWidget(QWidget* parent, Qt::WindowFlags f) :
	QWidget(parent, f)
{
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void GDXWidget::paintEvent(QPaintEvent* event)
{
	Render();
}

void GDXWidget::resizeEvent(QResizeEvent* event)
{
	renderer->lastResize = clock();
	renderer->newSize = {geometry().width(), geometry().height()};
}

void GDXWidget::InitD3D(GlobalApplication* app)
{
	renderer = std::make_shared<Renderer>((HWND)winId(), size().width(), size().height());
	app->SetRenderer(renderer);
	app->SetCamera(renderer->GetCamera());

	QTimer* pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout()), this, SLOT(repaint()));
	pTimer->start(8); //Լ60FPS
}

void GDXWidget::Render()
{
	renderer->Draw();
}

std::shared_ptr<Renderer> GDXWidget::GetRenderer()
{
	return renderer;
}

void GDXWidget::SwitchModel()
{
	renderer->SwitchModel();
}
void GDXWidget::SwitchPoint()
{
	renderer->SwitchPoint();
}
void GDXWidget::SwitchLine()
{
	renderer->SwitchLine();
}
void GDXWidget::SwitchFace()
{
	renderer->SwitchFace();
}
void GDXWidget::SwitchSolid()
{
	renderer->SwitchSolid();
}

void GDXWidget::SaveModel()
{
	renderer->SaveModel();
}

void GDXWidget::SwitchUp()
{
	renderer->SwitchUp();
}
void GDXWidget::SwitchDown()
{
	renderer->SwitchDown();
}
void GDXWidget::SwitchLeft()
{
	renderer->SwitchLeft();
}
void GDXWidget::SwitchRight()
{
	renderer->SwitchRight();
}
void GDXWidget::SwitchFront()
{
	renderer->SwitchFront();
}
void GDXWidget::SwitchBack()
{
	renderer->SwitchBack();
}

void GDXWidget::ChangeLight(int intensity)
{
	renderer->ChangeLightIntensity(intensity);
}



