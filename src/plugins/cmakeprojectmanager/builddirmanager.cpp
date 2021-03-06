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

#include "builddirmanager.h"
#include "cmakekitinformation.h"
#include "cmakeparser.h"
#include "cmakeprojectmanager.h"
#include "cmaketool.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

namespace CMakeProjectManager {
namespace Internal {

static QStringList toArguments(const CMakeConfig &config) {
    return Utils::transform(config, [](const CMakeConfigItem &i) -> QString {
                                 QString a = QString::fromLatin1("-D");
                                 a.append(QString::fromUtf8(i.key));
                                 switch (i.type) {
                                 case CMakeConfigItem::FILEPATH:
                                     a.append(QLatin1String(":FILEPATH="));
                                     break;
                                 case CMakeConfigItem::PATH:
                                     a.append(QLatin1String(":PATH="));
                                     break;
                                 case CMakeConfigItem::BOOL:
                                     a.append(QLatin1String(":BOOL="));
                                     break;
                                 case CMakeConfigItem::STRING:
                                     a.append(QLatin1String(":STRING="));
                                     break;
                                 case CMakeConfigItem::INTERNAL:
                                     a.append(QLatin1String(":INTERNAL="));
                                     break;
                                 }
                                 a.append(QString::fromUtf8(i.value));

                                 return a;
                             });
}

// --------------------------------------------------------------------
// BuildDirManager:
// --------------------------------------------------------------------

BuildDirManager::BuildDirManager(const Utils::FileName &sourceDir, const ProjectExplorer::Kit *k,
                                 const CMakeConfig &inputConfig, const Utils::Environment &env,
                                 const Utils::FileName &buildDir) :
    m_sourceDir(sourceDir),
    m_buildDir(buildDir),
    m_kit(k),
    m_environment(env),
    m_inputConfig(inputConfig),
    m_watcher(new QFileSystemWatcher(this))
{
    QTC_CHECK(!sourceDir.isEmpty());
    m_projectName = m_sourceDir.fileName();
    if (m_buildDir.isEmpty()) {
        m_tempDir = new QTemporaryDir(QLatin1String("cmake-tmp-XXXXXX"));
        m_buildDir = Utils::FileName::fromString(m_tempDir->path());
    }
    QTC_CHECK(!m_buildDir.isEmpty());
    QTC_CHECK(k);

    m_reparseTimer.setSingleShot(true);
    m_reparseTimer.setInterval(500);
    connect(&m_reparseTimer, &QTimer::timeout, this, &BuildDirManager::forceReparse);

    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this]() {
        if (!isBusy())
            m_reparseTimer.start();
    });

    QTimer::singleShot(0, this, &BuildDirManager::parse);
}

BuildDirManager::~BuildDirManager()
{
    delete m_tempDir;
}

bool BuildDirManager::isBusy() const
{
    if (m_cmakeProcess)
        return m_cmakeProcess->state() != QProcess::NotRunning;
    return false;
}

void BuildDirManager::forceReparse()
{
    if (isBusy()) {
        m_cmakeProcess->disconnect();
        m_cmakeProcess->deleteLater();
        m_cmakeProcess = nullptr;
    }

    CMakeTool *tool = CMakeKitInformation::cmakeTool(m_kit);
    const QString generator = CMakeGeneratorKitInformation::generator(m_kit);

    QTC_ASSERT(tool, return);
    QTC_ASSERT(!generator.isEmpty(), return);

    startCMake(tool, generator, m_inputConfig);
}

void BuildDirManager::setInputConfiguration(const CMakeConfig &config)
{
    m_inputConfig = config;
    forceReparse();
}

void BuildDirManager::parse()
{
    CMakeTool *tool = CMakeKitInformation::cmakeTool(m_kit);
    const QString generator = CMakeGeneratorKitInformation::generator(m_kit);

    QTC_ASSERT(tool, return);
    QTC_ASSERT(!generator.isEmpty(), return);

    // Pop up a dialog asking the user to rerun cmake
    QString cbpFile = CMakeManager::findCbpFile(QDir(m_buildDir.toString()));
    QFileInfo cbpFileFi(cbpFile);

    if (!cbpFileFi.exists()) {
        // Initial create:
        startCMake(tool, generator, m_inputConfig);
        return;
    }

    const bool mustUpdate
            = Utils::anyOf(m_watchedFiles, [&cbpFileFi](const Utils::FileName &f) {
                  return f.toFileInfo().lastModified() > cbpFileFi.lastModified();
              });
    if (mustUpdate) {
        startCMake(tool, generator, CMakeConfig());
    } else {
        extractData();
        emit dataAvailable();
    }
}

bool BuildDirManager::isProjectFile(const Utils::FileName &fileName) const
{
    return m_watchedFiles.contains(fileName);
}

QString BuildDirManager::projectName() const
{
    return m_projectName;
}

QList<CMakeBuildTarget> BuildDirManager::buildTargets() const
{
    return m_buildTargets;
}

QList<ProjectExplorer::FileNode *> BuildDirManager::files() const
{
    return m_files;
}

void BuildDirManager::clearFiles()
{
    m_files.clear();
}

CMakeConfig BuildDirManager::configuration() const
{
    return parseConfiguration();
}

void BuildDirManager::extractData()
{
    const Utils::FileName topCMake
            = Utils::FileName::fromString(m_sourceDir.toString() + QLatin1String("/CMakeLists.txt"));

    m_projectName = m_sourceDir.fileName();
    m_buildTargets.clear();
    m_watchedFiles.clear();
    m_files.clear();
    m_files.append(new ProjectExplorer::FileNode(topCMake, ProjectExplorer::ProjectFileType, false));
    m_watchedFiles.insert(topCMake);

    m_watcher->removePaths(m_watcher->files());

    // Find cbp file
    QString cbpFile = CMakeManager::findCbpFile(m_buildDir.toString());
    if (cbpFile.isEmpty())
        return;

    m_watcher->addPath(cbpFile);

    // setFolderName
    CMakeCbpParser cbpparser;
    // Parsing
    if (!cbpparser.parseCbpFile(m_kit, cbpFile, m_sourceDir.toString()))
        return;

    m_projectName = cbpparser.projectName();

    m_files = cbpparser.fileList();
    QSet<Utils::FileName> projectFiles;
    if (cbpparser.hasCMakeFiles()) {
        m_files.append(cbpparser.cmakeFileList());
        foreach (const ProjectExplorer::FileNode *node, cbpparser.cmakeFileList())
            projectFiles.insert(node->filePath());
    } else {
        m_files.append(new ProjectExplorer::FileNode(topCMake, ProjectExplorer::ProjectFileType, false));
        projectFiles.insert(topCMake);
    }

    m_watchedFiles = projectFiles;
    const QStringList toWatch
            = Utils::transform(m_watchedFiles.toList(), [](const Utils::FileName &fn) { return fn.toString(); });
    m_watcher->addPaths(toWatch);

    m_buildTargets = cbpparser.buildTargets();
}

void BuildDirManager::startCMake(CMakeTool *tool, const QString &generator,
                                 const CMakeConfig &config)
{
    QTC_ASSERT(tool && tool->isValid(), return);
    QTC_ASSERT(!m_cmakeProcess, return);
    QTC_ASSERT(!m_parser, return);
    QTC_ASSERT(!m_future, return);

    // Make sure m_buildDir exists:
    const QString buildDirStr = m_buildDir.toString();
    QDir bDir = QDir(buildDirStr);
    bDir.mkpath(buildDirStr);

    m_parser = new CMakeParser;
    QDir source = QDir(m_sourceDir.toString());
    connect(m_parser, &ProjectExplorer::IOutputParser::addTask, m_parser,
            [source](const ProjectExplorer::Task &task) {
                if (task.file.isEmpty() || task.file.toFileInfo().isAbsolute()) {
                    ProjectExplorer::TaskHub::addTask(task);
                } else {
                    ProjectExplorer::Task t = task;
                    t.file = Utils::FileName::fromString(source.absoluteFilePath(task.file.toString()));
                    ProjectExplorer::TaskHub::addTask(t);
                }
            });

    // Always use the sourceDir: If we are triggered because the build directory is getting deleted
    // then we are racing against CMakeCache.txt also getting deleted.
    const QString srcDir = m_sourceDir.toString();

    m_cmakeProcess = new Utils::QtcProcess(this);
    m_cmakeProcess->setWorkingDirectory(buildDirStr);
    m_cmakeProcess->setEnvironment(m_environment);

    connect(m_cmakeProcess, &QProcess::readyReadStandardOutput,
            this, &BuildDirManager::processCMakeOutput);
    connect(m_cmakeProcess, &QProcess::readyReadStandardError,
            this, &BuildDirManager::processCMakeError);
    connect(m_cmakeProcess, static_cast<void(QProcess::*)(int,  QProcess::ExitStatus)>(&QProcess::finished),
            this, &BuildDirManager::cmakeFinished);

    QString args;
    Utils::QtcProcess::addArg(&args, srcDir);
    if (!generator.isEmpty())
        Utils::QtcProcess::addArg(&args, QString::fromLatin1("-G%1").arg(generator));
    Utils::QtcProcess::addArgs(&args, toArguments(config));

    ProjectExplorer::TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    Core::MessageManager::write(tr("Running '%1 %2' in %3.")
                                .arg(tool->cmakeExecutable().toUserOutput())
                                .arg(args)
                                .arg(m_buildDir.toUserOutput()));

    m_future = new QFutureInterface<void>();
    m_future->setProgressRange(0, 1);
    Core::ProgressManager::addTask(m_future->future(),
                                   tr("Configuring \"%1\"").arg(projectName()),
                                   "CMake.Configure");

    m_cmakeProcess->setCommand(tool->cmakeExecutable().toString(), args);
    m_cmakeProcess->start();
    emit parsingStarted();
}

void BuildDirManager::cmakeFinished(int code, QProcess::ExitStatus status)
{
    QTC_ASSERT(m_cmakeProcess, return);

    // process rest of the output:
    processCMakeOutput();
    processCMakeError();

    m_parser->flush();
    delete m_parser;
    m_parser = nullptr;

    m_cmakeProcess->deleteLater();
    m_cmakeProcess = nullptr;

    extractData(); // try even if cmake failed...

    QString msg;
    if (status != QProcess::NormalExit)
        msg = tr("*** cmake process crashed!");
    else if (code != 0)
        msg = tr("*** cmake process exited with exit code %1.").arg(code);

    if (!msg.isEmpty()) {
        Core::MessageManager::write(msg);
        ProjectExplorer::TaskHub::addTask(ProjectExplorer::Task::Error, msg,
                                          ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
        m_future->reportCanceled();
    } else {
        m_future->setProgressValue(1);
    }

    m_future->reportFinished();
    delete m_future;
    m_future = 0;

    emit dataAvailable();
}

static QString lineSplit(const QString &rest, const QByteArray &array, std::function<void(const QString &)> f)
{
    QString tmp = rest + Utils::SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(array));
    int start = 0;
    int end = tmp.indexOf(QLatin1Char('\n'), start);
    while (end >= 0) {
        f(tmp.mid(start, end - start));
        start = end + 1;
        end = tmp.indexOf(QLatin1Char('\n'), start);
    }
    return tmp.mid(start);
}

void BuildDirManager::processCMakeOutput()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardOutput(), [this](const QString &s) { Core::MessageManager::write(s); });
}

void BuildDirManager::processCMakeError()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardError(), [this](const QString &s) {
        m_parser->stdError(s);
        Core::MessageManager::write(s);
    });
}

static QByteArray trimCMakeCacheLine(const QByteArray &in) {
    int start = 0;
    while (start < in.count() && (in.at(start) == ' ' || in.at(start) == '\t'))
        ++start;

    return in.mid(start, in.count() - start - 1);
}

static QByteArrayList splitCMakeCacheLine(const QByteArray &line) {
    const int colonPos = line.indexOf(':');
    if (colonPos < 0)
        return QByteArrayList();

    const int equalPos = line.indexOf('=', colonPos + 1);
    if (equalPos < colonPos)
        return QByteArrayList();

    return QByteArrayList() << line.mid(0, colonPos)
                            << line.mid(colonPos + 1, equalPos - colonPos - 1)
                            << line.mid(equalPos + 1);
}

static CMakeConfigItem::Type fromByteArray(const QByteArray &type) {
    if (type == "BOOL")
        return CMakeConfigItem::BOOL;
    if (type == "STRING")
        return CMakeConfigItem::STRING;
    if (type == "FILEPATH")
        return CMakeConfigItem::FILEPATH;
    if (type == "PATH")
        return CMakeConfigItem::PATH;
    QTC_CHECK(type == "INTERNAL" || type == "STATIC");

    return CMakeConfigItem::INTERNAL;
}

CMakeConfig BuildDirManager::parseConfiguration() const
{
    CMakeConfig result;
    const QString cacheFile = QDir(m_buildDir.toString()).absoluteFilePath(QLatin1String("CMakeCache.txt"));
    QFile cache(cacheFile);
    if (!cache.open(QIODevice::ReadOnly | QIODevice::Text))
        return CMakeConfig();

    QSet<QByteArray> advancedSet;
    QByteArray documentation;
    while (!cache.atEnd()) {
        const QByteArray line = trimCMakeCacheLine(cache.readLine());

        if (line.isEmpty() || line.startsWith('#'))
            continue;

        if (line.startsWith("//")) {
            documentation = line.mid(2);
            continue;
        }

        const QByteArrayList pieces = splitCMakeCacheLine(line);
        if (pieces.isEmpty())
            continue;

        QTC_ASSERT(pieces.count() == 3, continue);
        const QByteArray key = pieces.at(0);
        const QByteArray type = pieces.at(1);
        const QByteArray value = pieces.at(2);

        if (key.endsWith("-ADVANCED") && value == "1") {
            advancedSet.insert(key.left(key.count() - 9 /* "-ADVANCED" */));
        } else {
            CMakeConfigItem::Type t = fromByteArray(type);
            if (t != CMakeConfigItem::INTERNAL)
                result << CMakeConfigItem(key, t, documentation, value);

            // Sanity checks:
            if (key == "CMAKE_HOME_DIRECTORY") {
                const Utils::FileName actualSourceDir = Utils::FileName::fromUserInput(QString::fromUtf8(value));
                if (actualSourceDir != m_sourceDir)
                    emit errorOccured(tr("Build directory contains a build of the wrong project (%1).")
                                      .arg(actualSourceDir.toUserOutput()));
            }
        }
    }

    // Set advanced flags:
    for (int i = 0; i < result.count(); ++i) {
        CMakeConfigItem &item = result[i];
        item.isAdvanced = advancedSet.contains(item.key);
    }

    Utils::sort(result, CMakeConfigItem::sortOperator());

    return result;
}

} // namespace Internal
} // namespace CMakeProjectManager
