/* Copyright 2015 QReal Research Group, Dmitry Mordvinov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#pragma once

#include <QtWidgets/QFrame>

class QGraphicsItem;

namespace graphicsUtils {

class AbstractScene;

/// A base class for popup window that configures some item on 2D model scene.
/// Each time when user selets the item of template type this popup will appear to specify some
/// properties of the item.
class ItemPopup : public QFrame
{
	Q_OBJECT

public:
	explicit ItemPopup(AbstractScene &scene, QWidget *parent = 0);
	~ItemPopup();

private slots:
	/// Implementation must return should this window be shown for the given item or not.
	/// This can be prettily done with templates, but QObject can not be used with them.
	/// Default implementation returns true for graphicsUtils::AbstractItem instances.
	virtual bool suits(QGraphicsItem *item);

	/// Called when this window is shown for some item.
	/// Default implementation calls attachTo({item}).
	virtual void attachTo(QGraphicsItem *item);

	/// Called when this window is shown for some group of items.
	/// Default implementation does nothing.
	virtual void attachTo(const QList<QGraphicsItem *> &items);

	/// Called when user canceled his selection and before the window is hidden.
	/// Default implementation does nothing.
	virtual void detach();

	void onMousePressedScene();
	void onMouseReleasedScene();

private:
	QPointF leftmostTopmost(const QList<QGraphicsItem *> &items);

	AbstractScene &mScene;
};

}
