/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt tool chooser of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>

#ifdef Q_OS_WIN
#  include <process.h>
#  define getpid _getpid
#else
#  include <unistd.h>
#endif

#ifdef Q_OS_WIN
#  define LIST_SEP ";"
#else
#  define LIST_SEP ":"
#endif

#define VERIFY_NORMAL_EXIT(proc) \
    QVERIFY(proc); \
    QCOMPARE(proc->readAllStandardError().constData(), ""); \
    QCOMPARE(proc->exitCode(), 0)

class tst_ToolChooser : public QObject
{
    Q_OBJECT

public:
    QProcessEnvironment testModeEnvironment;
    QString toolPath;
    QString pathsWithDefault;

    tst_ToolChooser();
    inline QProcess *execute(const QStringList &arguments)
    { return execute(arguments, testModeEnvironment); }
    inline QProcess *execute(const QStringList &arguments, const QProcessEnvironment &env)
    { return execute(toolPath, arguments, env); }
    QProcess *execute(const QString &program, const QStringList &arguments, const QProcessEnvironment &env);

    QString tempFileName() const;

private Q_SLOTS:
    void list();
    void argv0_data();
    void argv0();
    void selectTool_data();
    void selectTool();
    void selectQt_data();
    void selectQt();
    void defaultQt_data();
    void defaultQt();
    void passArgs_data();
    void passArgs();
};

tst_ToolChooser::tst_ToolChooser()
    : testModeEnvironment(QProcessEnvironment::systemEnvironment())
{
    QString testData = QFINDTESTDATA("testdata");
    pathsWithDefault = testData + "/config1" LIST_SEP +
            testData + "/config2";
    testModeEnvironment.insert("XDG_CONFIG_HOME", "/dev/null");
    testModeEnvironment.insert("XDG_CONFIG_DIRS", pathsWithDefault);
    testModeEnvironment.insert("QT_SELECT", "4.8");

    pathsWithDefault.prepend(testData + "/default" LIST_SEP);

#ifdef Q_OS_WIN
    toolPath = QCoreApplication::applicationDirPath() + "/../../../src/qtchooser/qtchooser-test.exe";
#else
    toolPath = QCoreApplication::applicationDirPath() + "/../../../src/qtchooser/qtchooser-test";
#endif

    QVERIFY(QFile::exists(toolPath));
}

QProcess *tst_ToolChooser::execute(const QString &program, const QStringList &arguments, const QProcessEnvironment &env)
{
    QProcess *proc = new QProcess;
    proc->setProcessEnvironment(env);
    proc->start(program, arguments, QIODevice::ReadOnly | QIODevice::Text);
    if (!proc->waitForFinished()) {
        QTest::qFail("Executing '" + program.toLocal8Bit()
                     + "' failed: " + proc->errorString().toLocal8Bit(),
                     __FILE__, __LINE__);
        delete proc;
        return 0;
    }
    return proc;
}

QString tst_ToolChooser::tempFileName() const
{
    static int seq = 0;
    return QDir::currentPath() + "/tool" + QString::number(seq++) + '-' + QString::number(getpid()) + ".exe";
}

void tst_ToolChooser::list()
{
    QScopedPointer<QProcess> proc(execute(QStringList() << "-list-versions"));
    VERIFY_NORMAL_EXIT(proc);

    QStringList foundVersions;
    while (!proc->atEnd()) {
        QByteArray line = proc->readLine().trimmed();
        QVERIFY(!line.isEmpty());
        QVERIFY(!foundVersions.contains(line));
        foundVersions << line;
    }

    QVERIFY(foundVersions.contains("4.8"));
    QVERIFY(foundVersions.contains("5"));
}

void tst_ToolChooser::argv0_data()
{
    QTest::addColumn<bool>("symlink");
    QTest::newRow("copy") << false;

#ifdef Q_OS_UNIX
    QTest::newRow("symlink") << true;
#endif
}

void tst_ToolChooser::argv0()
{
    // check that the tool tries to execute something with the sane base name
    // as its argv[0]
    QFETCH(bool, symlink);

    // We can't use QTemporaryFile here because we need to
    // fully close the file in order to run the executable
    QString tmpName = tempFileName();
    QFile source(toolPath);
    if (!symlink) {
        QFile tmp(tmpName);
        QVERIFY(tmp.open(QIODevice::ReadWrite));
        QVERIFY(source.open(QIODevice::ReadOnly));
        tmp.write(source.readAll());
        tmp.setPermissions(QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);
        source.close();
    } else {
#ifndef Q_OS_UNIX
        qFatal("Impossible, cannot happen, you've broken the test!!");
#else
        // even though QTemporaryFile has the file opened, we'll overwrite it with a symlink
        // on Unix, we're allowed to do that
        QVERIFY(source.link(tmpName));
#endif
    }

    QScopedPointer<QProcess> proc(execute(tmpName, QStringList(), testModeEnvironment));
    VERIFY_NORMAL_EXIT(proc);

    QByteArray procstdout = proc->readAllStandardOutput().trimmed();
    QVERIFY2(procstdout.endsWith(QFileInfo(tmpName).fileName().toLocal8Bit()), procstdout);

    QFile::remove(tmpName);
}

void tst_ToolChooser::selectTool_data()
{
    QTest::addColumn<bool>("useEnv");
    QTest::newRow("cmdline") << false;
    QTest::newRow("env") << true;
}

void tst_ToolChooser::selectTool()
{
    QFETCH(bool, useEnv);
    QProcessEnvironment env = testModeEnvironment;
    QStringList args;

    if (useEnv)
        env.insert("QTCHOOSER_RUNTOOL", "testtool");
    else
        args << "-run-tool=testtool";

    QScopedPointer<QProcess> proc(execute(args, env));
    VERIFY_NORMAL_EXIT(proc);

    QByteArray procstdout = proc->readAllStandardOutput().trimmed();
    QVERIFY2(procstdout.endsWith("testtool"), procstdout);
}

void tst_ToolChooser::selectQt_data()
{
    QTest::addColumn<bool>("useEnv");
    QTest::addColumn<QString>("select");
    QTest::addColumn<QString>("expected");

    QTest::newRow("cmdline-4.8") << false << "4.8" << "correct-4.8";
    QTest::newRow("env-4.8") << true << "4.8" << "correct-4.8";
    QTest::newRow("cmdline-5") << false << "5" << "qt5";
    QTest::newRow("env-5") << true << "5" << "qt5";

    QTest::newRow("cmdline-invalid") << false << "invalid" << QString();
    QTest::newRow("env-invalid") << true << "invalid" << QString();
}

void tst_ToolChooser::selectQt()
{
    QFETCH(bool, useEnv);
    QFETCH(QString, select);
    QFETCH(QString, expected);

    QProcessEnvironment env = testModeEnvironment;
    QStringList args;

    env.remove("QT_SELECT");
    if (useEnv)
        env.insert("QT_SELECT", select);
    else
        args << "-qt=" + select;
    args << "-print-env";

    QScopedPointer<QProcess> proc(execute(args, env));
    QVERIFY(proc);
    if (expected.isEmpty()) {
        // it is supposed to fail
        QCOMPARE(proc->readAllStandardOutput().constData(), "");
        QVERIFY(proc->exitCode() != 0);
        QVERIFY(!proc->readAllStandardError().isEmpty());
    } else {
        QCOMPARE(proc->readAllStandardError().constData(), "");
        QCOMPARE(proc->exitCode(), 0);

        // The first line is QT_SELECT= again
        QByteArray line = proc->readLine().trimmed();
        QVERIFY2(line.startsWith("QT_SELECT="), line);

        // The second line is QTTOOLDIR=
        line = proc->readLine().trimmed();
        QVERIFY2(line.startsWith("QTTOOLDIR="), line);
        QVERIFY2(line.contains(expected.toLatin1()), line);
        QVERIFY2(line.endsWith("tooldir\""), line);

        // The third line is QTLIBDIR=
        line = proc->readLine().trimmed();
        QVERIFY2(line.startsWith("QTLIBDIR="), line);
        QVERIFY2(line.contains(expected.toLatin1()), line);
        QVERIFY2(line.endsWith("libdir\""), line);
    }
}

void tst_ToolChooser::defaultQt_data()
{
    QTest::addColumn<bool>("withDefault");
    QTest::addColumn<QString>("expected");

    QTest::newRow("no-default") << false << QString();
    QTest::newRow("with-default") << true << "default-qt";
}

void tst_ToolChooser::defaultQt()
{
    QFETCH(bool, withDefault);
    QFETCH(QString, expected);

    QProcessEnvironment env = testModeEnvironment;
    env.remove("QT_SELECT");
    if (withDefault)
        env.insert("XDG_CONFIG_DIRS", pathsWithDefault);

    QScopedPointer<QProcess> proc(execute(QStringList() << "-print-qmake", env));
    QVERIFY(proc);
    if (withDefault) {
        QCOMPARE(proc->readAllStandardError().constData(), "");
        QCOMPARE(proc->exitCode(), 0);

        QByteArray procstdout = proc->readAllStandardOutput().trimmed();
        QVERIFY2(procstdout.contains(expected.toLatin1()), procstdout);
    } else {
        // no default, the tool fails
        QVERIFY(proc->exitCode() != 0);
        QVERIFY(!proc->readAllStandardError().isEmpty());
        QByteArray procstdout = proc->readAllStandardOutput().trimmed();
        QVERIFY2(procstdout.isEmpty(), procstdout.constData());
    }
}

void tst_ToolChooser::passArgs_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("empty") << QStringList() << QStringList();
    QTest::newRow("drop1") << (QStringList() << "-qt5") << QStringList();
    QTest::newRow("drop2") << (QStringList() << "-qt5" << "-qt5") << QStringList();

    QTest::newRow("dash-dash") << (QStringList() << "--") << QStringList();
    QTest::newRow("dash-dash-qt5") << (QStringList() << "--" << "-qt5") << (QStringList() << "-qt5");
    QTest::newRow("dash-dash-dash-dash") << (QStringList() << "--" << "--") << (QStringList() << "--");

    QTest::newRow("unknown-opt") << (QStringList() << "-query") << (QStringList() << "-query");
    QTest::newRow("unknown-opt-qt5") << (QStringList() << "-query" << "-qt5") << (QStringList() << "-query" << "-qt5");
    QTest::newRow("non-opt") << (QStringList() << ".") << (QStringList() << ".");
    QTest::newRow("non-opt-qt5") << (QStringList() << "." << "-qt5") << (QStringList() << "." << "-qt5");
}

void tst_ToolChooser::passArgs()
{
    QFETCH(QStringList, args);
    QFETCH(QStringList, expected);

    QScopedPointer<QProcess> proc(execute(args));
    VERIFY_NORMAL_EXIT(proc);

    // skip the first line of procstdout, as it contains the tool name
    proc->readLine();

    QByteArray procstdout = proc->readAll().trimmed();
    QCOMPARE(QString::fromLocal8Bit(procstdout), expected.join('\n'));
}

QTEST_MAIN(tst_ToolChooser)

#include "tst_qtchooser.moc"
