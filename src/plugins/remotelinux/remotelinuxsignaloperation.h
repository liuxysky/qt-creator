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

#ifndef REMOTELINUXSIGNALOPERATION_H
#define REMOTELINUXSIGNALOPERATION_H

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <ssh/sshconnection.h>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace RemoteLinux {

class REMOTELINUX_EXPORT RemoteLinuxSignalOperation
        : public ProjectExplorer::DeviceProcessSignalOperation
{
    Q_OBJECT
public:
    virtual ~RemoteLinuxSignalOperation();

    void killProcess(qint64 pid);
    void killProcess(const QString &filePath);
    void interruptProcess(qint64 pid);
    void interruptProcess(const QString &filePath);

protected:
    RemoteLinuxSignalOperation(const QSsh::SshConnectionParameters &sshParameters);

private slots:
    void runnerProcessFinished();
    void runnerConnectionError();

private:
    virtual QString killProcessByNameCommandLine(const QString &filePath) const;
    virtual QString interruptProcessByNameCommandLine(const QString &filePath) const;
    void run(const QString &command);
    void finish();

    const QSsh::SshConnectionParameters m_sshParameters;
    QSsh::SshRemoteProcessRunner *m_runner;

    friend class LinuxDevice;
};

}

#endif // REMOTELINUXSIGNALOPERATION_H
