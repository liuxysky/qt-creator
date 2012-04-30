/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "opendocumentsfilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QFileInfo>
#include <QDir>

using namespace Core;
using namespace Locator;
using namespace Locator::Internal;

OpenDocumentsFilter::OpenDocumentsFilter(EditorManager *editorManager) :
    m_editorManager(editorManager)
{
    connect(m_editorManager, SIGNAL(editorOpened(Core::IEditor*)),
            this, SLOT(refreshInternally()));
    connect(m_editorManager, SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(refreshInternally()));
    setShortcutString(QString(QLatin1Char('o')));
    setIncludedByDefault(true);
}

QList<FilterEntry> OpenDocumentsFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    QList<FilterEntry> value;
    const QChar asterisk = QLatin1Char('*');
    QString pattern = QString(asterisk);
    pattern += entry;
    pattern += asterisk;
    QRegExp regexp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return value;
    foreach (const OpenEditorsModel::Entry &editorEntry, m_editors) {
        if (future.isCanceled())
            break;
        QString fileName = editorEntry.fileName();
        QString displayName = editorEntry.displayName();
        if (regexp.exactMatch(displayName)) {
            if (!fileName.isEmpty()) {
                QFileInfo fi(fileName);
                FilterEntry fiEntry(this, fi.fileName(), fileName);
                fiEntry.extraInfo = QDir::toNativeSeparators(fi.path());
                fiEntry.resolveFileIcon = true;
                value.append(fiEntry);
            }
        }
    }
    return value;
}

void OpenDocumentsFilter::refreshInternally()
{
    m_editors.clear();
    foreach (IEditor *editor, m_editorManager->openedEditors()) {
        OpenEditorsModel::Entry entry;
        // don't work on IEditor directly, since that will be useless with split windows
        entry.m_displayName = editor->displayName();
        entry.m_fileName = editor->document()->fileName();
        m_editors.append(entry);
    }
    m_editors += m_editorManager->openedEditorsModel()->restoredEditors();
}

void OpenDocumentsFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, "refreshInternally", Qt::BlockingQueuedConnection);
}

void OpenDocumentsFilter::accept(FilterEntry selection) const
{
    m_editorManager->openEditor(selection.internalData.toString(), Core::Id(), Core::EditorManager::ModeSwitch);
}
