/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLPRIVATEGATE_H
#define QMLPRIVATEGATE_H

#include "nodeinstanceglobal.h"

#include <QObject>
#include <QString>
#include <QQmlContext>

class QQmlContext;

namespace QmlDesigner {

namespace Internal {

class ObjectNodeInstance;
typedef QSharedPointer<ObjectNodeInstance> ObjectNodeInstancePointer;
typedef QWeakPointer<ObjectNodeInstance> ObjectNodeInstanceWeakPointer;

namespace QmlPrivateGate {

class ComponentCompleteDisabler
{
public:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 1, 0))
    ComponentCompleteDisabler();

    ~ComponentCompleteDisabler();
#else
    ComponentCompleteDisabler()
    {
    //nothing not available yet
    }
#endif
};

    void readPropertyValue(QObject *object, const QByteArray &propertyName, QQmlContext *qmlContext, bool *ok);
    void createNewDynamicProperty(const ObjectNodeInstancePointer &nodeInstance, const QString &name);
    void registerNodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance);
    QObject *createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context);
    QObject *createComponent(const QUrl &componentUrl, QQmlContext *context);
    void tweakObjects(QObject *object);
    bool isPropertyBlackListed(const QmlDesigner::PropertyName &propertyName);
    PropertyNameList propertyNameListForWritableProperties(QObject *object,
                                                           const PropertyName &baseName = PropertyName(),
                                                           QObjectList *inspectedObjects = 0);
    PropertyNameList allPropertyNames(QObject *object,
                                      const PropertyName &baseName = PropertyName(),
                                      QObjectList *inspectedObjects = 0);


} // namespace QmlPrivateGate
} // namespace Internal
} // namespace QmlDesigner

#endif // QMLPRIVATEGATE_H