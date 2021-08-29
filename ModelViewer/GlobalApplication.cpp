#include "GlobalApplication.h"

GlobalApplication::GlobalApplication(int& argc, char** argv) : QApplication(argc, argv)
{
	
}

void GlobalApplication::SetCamera(std::shared_ptr<Camera> camera)
{
	this->camera = camera;
}

void GlobalApplication::SetRenderer(std::shared_ptr<Renderer> renderer)
{
	this->renderer = renderer;
}

void GlobalApplication::setWindowInstance(QWidget* widget)
{
	this->widget = widget;
}


bool GlobalApplication::notify(QObject* obj, QEvent* e)
{
	const QMetaObject* objMeta = obj->metaObject();
    QString clName = objMeta->className();

    if(e->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);

        if(keyEvent->key() == Qt::Key_Alt)
        {
	        GetCursorPos(&curPos);
            setOverrideCursor(Qt::BlankCursor);
            rotate = true;
        }
        else if(keyEvent->key() == Qt::Key_Shift)
        {
	        GetCursorPos(&curPos);
            setOverrideCursor(Qt::BlankCursor);
            transform = true;
        }
        else if(keyEvent->key() == Qt::Key_R)
        {
            std::cout << "Press Q\n";
	        renderer->SwitchModel();
            return true;
        }
        else if(keyEvent->key() == Qt::Key_G)
        {
	        std::cout << "Stop Render!" << std::endl;
            renderer->flag ^= 1; 
            return true;
        }
        else if(keyEvent->key() == Qt::Key_V)
        {
	        printf("%f  %f  %f\n", camera->theta, camera->phi, camera->radius);
        }
        else if(keyEvent->key() == Qt::Key_J) renderer->primitiveType = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        else if(keyEvent->key() == Qt::Key_K) renderer->primitiveType = D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
        else if(keyEvent->key() == Qt::Key_L) renderer->primitiveType = D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
        else if(keyEvent->key() == Qt::Key_Escape)
        {
	        quit();
            return true;
        }
    }
	else if(e->type() == QEvent::KeyRelease)
    {
	    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);

        if(keyEvent->key() == Qt::Key_Alt) {
        	setOverrideCursor(Qt::ArrowCursor);
            rotate = false;
        }
        else if(keyEvent->key() == Qt::Key_Shift)
        {
	        setOverrideCursor(Qt::ArrowCursor);
            transform = false;
        }
    }
    else if(e->type() == QEvent::MouseMove && (transform || rotate))
    {
	    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);

        POINT newPos;
    	GetCursorPos(&newPos);

        float tmp = 0.05;

        if(rotate)
			renderer->Rotate(newPos.x - curPos.x, newPos.y - curPos.y);
        else 
            renderer->Transform((newPos.x - curPos.x) * tmp, (newPos.y - curPos.y) * tmp);
        SetCursorPos(curPos.x, curPos.y);
    }
    else if(e->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent * mouseEvent = static_cast<QMouseEvent*>(e);
        if(mouseEvent->button() == Qt::MidButton)
        {
	        GetCursorPos(&curPos);
    		setOverrideCursor(Qt::BlankCursor);
    		rotate = true;
        }
    }
    else if(e->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent * mouseEvent = static_cast<QMouseEvent*>(e);
        if(mouseEvent->button() == Qt::MidButton)
        {
	        setOverrideCursor(Qt::ArrowCursor);
            rotate = false;
        }
    }
    else if(e->type() == QEvent::Wheel)
    {
	    QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(e);

        camera->Scale(wheelEvent->delta());
    }
    else if(e->type() == QEvent::Move)
    {
        if(renderer != nullptr)
			renderer->lastMove = clock();
    }

    return QApplication::notify(obj,e);
}



