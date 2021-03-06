/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef ICODESTYLEPREFERENCESFACTORY_H
#define ICODESTYLEPREFERENCESFACTORY_H

#include "texteditor_global.h"

#include <QObject>

namespace Core { class Id; }
namespace TextEditor {

class ICodeStylePreferences;
class CodeStylePool;
class Indenter;
class ISnippetProvider;

class TEXTEDITOR_EXPORT ICodeStylePreferencesFactory : public QObject
{
    Q_OBJECT
public:
    explicit ICodeStylePreferencesFactory(QObject *parent = 0);

    virtual Core::Id languageId() = 0;
    virtual QString displayName() = 0;
    virtual ICodeStylePreferences *createCodeStyle() const = 0;
    virtual QWidget *createEditor(ICodeStylePreferences *preferences, QWidget *parent) const = 0;
    virtual TextEditor::Indenter *createIndenter() const = 0;
    virtual ISnippetProvider *snippetProvider() const = 0;
    virtual QString previewText() const = 0;
};

} // namespace TextEditor

#endif // ICODESTYLEPREFERENCESFACTORY_H
