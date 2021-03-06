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

#ifndef QMLPROFILERRUNCONTROL_H
#define QMLPROFILERRUNCONTROL_H

#include "qmlprofilerstatemanager.h"

#include <analyzerbase/analyzerruncontrol.h>
#include <utils/outputformat.h>

namespace QmlProfiler {

namespace Internal { class QmlProfilerTool; }

class QmlProfilerRunControl : public Analyzer::AnalyzerRunControl
{
    Q_OBJECT

public:
    QmlProfilerRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
                          Internal::QmlProfilerTool *tool);
    ~QmlProfilerRunControl() override;

    void registerProfilerStateManager( QmlProfilerStateManager *profilerState );

    void notifyRemoteSetupDone(quint16 port) override;
    StopResult stop() override;

signals:
    void processRunning(quint16 port);

public slots:
    bool startEngine() override;
    void stopEngine() override;
    void cancelProcess();
    void notifyRemoteFinished() override;
    void logApplicationMessage(const QString &msg, Utils::OutputFormat format) override;

private slots:
    void wrongSetupMessageBox(const QString &errorMessage);
    void wrongSetupMessageBoxFinished(int);
    void processIsRunning(quint16 port);
    void profilerStateChanged();

private:
    class QmlProfilerRunControlPrivate;
    QmlProfilerRunControlPrivate *d;
};

} // namespace QmlProfiler

#endif // QMLPROFILERRUNCONTROL_H
