/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

/*
 * This tool is meant to wrap the other Qt tools by searching for different
 * instances of Qt on the system. It's mostly destined to be used on Unix
 * systems other than Mac OS X. For that reason, it uses POSIX APIs and does
 * not try to be compatible with the DOS or Win32 APIs. In particular, it uses
 * opendir(3) and readdir(3) as well as paths exclusively separated by slashes.
 */

/*
 * TODO:
 *  - rewrite with no Qt dependencies (C++ with Standard Libraries)
 *  - move to separate repository
 *  - default.conf searched for
 *  - no fallback if no default.conf found
 *  - no Windows support needed (can use POSIX)
 *  - qtbase configure option to say wrapper is coming (do create symlinks on install)
 */

#define _CRT_SECURE_NO_WARNINGS

#include <set>
#include <string>
#include <vector>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(__WIN32__)
#  include <process.h>
#  define execv _execv
#  define PATH_SEP "\\"
#else
#  include <sys/types.h>
#  include <dirent.h>
#  include <libgen.h>
#  include <unistd.h>
#  define PATH_SEP "/"
#endif

using namespace std;

static bool testMode = false;
static const char *argv0;
enum Mode {
    RunTool,
    ListVersions,
    PrintEnvironment
};

struct Sdk
{
    string name;
    string configFile;
    string toolsPath;
    string librariesPath;

    bool isValid() const { return !toolsPath.empty(); }
};

struct ToolWrapper
{
    int listVersions();
    int printEnvironment(const string &targetSdk);
    int runTool(const string &targetSdk, const string &targetTool, char **argv);

private:
    vector<string> searchPaths() const;

    typedef bool (ToolWrapper:: *VisitFunction)(const string &targetSdk, Sdk &item);
    Sdk iterateSdks(const string &targetSdk, VisitFunction visit);
    Sdk selectSdk(const string &targetSdk);

    bool printSdk(const string &, Sdk &sdk);
    bool matchSdk(const string &targetSdk, Sdk &sdk);
};

int ToolWrapper::listVersions()
{
    iterateSdks(string(), &ToolWrapper::printSdk);
    return 0;
}

int ToolWrapper::printEnvironment(const string &targetSdk)
{
    Sdk sdk = selectSdk(targetSdk);
    if (!sdk.isValid())
        return 1;

    // ### The paths and the name are not escaped, so hopefully no one will be
    // installing Qt on paths containing backslashes or quotes on Unix. Paths
    // with spaces are taken into account by surrounding the arguments with
    // quotes.

    printf("QT_SELECT=\"%s\"\n", sdk.name.c_str());
    printf("QTTOOLDIR=\"%s\"\n", sdk.toolsPath.c_str());
    printf("QTLIBDIR=\"%s\"\n", sdk.librariesPath.c_str());
    return 0;
}

int ToolWrapper::runTool(const string &targetSdk, const string &targetTool, char **argv)
{
    Sdk sdk = selectSdk(targetSdk);
    if (!sdk.isValid())
        return 1;

    string tool = sdk.toolsPath + PATH_SEP + targetTool;
    argv[0] = &tool[0];
    if (testMode) {
        while (*argv)
            printf("%s\n", *argv++);
        return 0;
    }

    execv(argv[0], argv);
    fprintf(stderr, "%s: could not exec '%s': %s\n",
            argv0, argv[0], strerror(errno));
    return 1;
}

static vector<string> stringSplit(const char *source)
{
#if defined(_WIN32) || defined(__WIN32__)
    char listSeparator = ';';
#else
    char listSeparator = ':';
#endif

    vector<string> result;
    if (!*source)
        return result;

    while (true) {
        const char *p = strchr(source, listSeparator);
        if (!p) {
            result.push_back(source);
            return result;
        }

        result.push_back(string(source, p - source));
        source = p + 1;
    }
    return result;
}

vector<string> ToolWrapper::searchPaths() const
{
    vector<string> paths;
    if (testMode) {
        return stringSplit(getenv("QTCHOOSER_PATHS"));
    } else {
        // search the XDG config location directories
        const char *globalDirs = getenv("XDG_CONFIG_DIRS");
        paths = stringSplit(!globalDirs || !*globalDirs ? "/etc/xdg" : globalDirs);

        string localDir;
        const char *localDirEnv = getenv("XDG_CONFIG_HOME");
        if (localDirEnv && *localDirEnv) {
            localDir = localDirEnv;
        } else {
            localDir = getenv("HOME"); // accept empty $HOME too
            localDir += "/.config";
        }
        paths.push_back(localDir);

        for (vector<string>::iterator it = paths.begin(); it != paths.end(); ++it)
            *it += "/qtchooser/";
    }
    return paths;
}

Sdk ToolWrapper::iterateSdks(const string &targetSdk, VisitFunction visit)
{
    vector<string> paths = searchPaths();
    set<string> seenNames;
    Sdk sdk;
    for (vector<string>::iterator it = paths.begin(); it != paths.end(); ++it) {
        const string &path = *it;

        // no ISO C++ or ISO C API for listing directories, so use POSIX
        DIR *dir = opendir(path.c_str());
        if (!dir)
            continue;  // no such dir or not a dir, doesn't matter

        while (struct dirent *d = readdir(dir)) {
#ifdef _DIRENT_HAVE_D_TYPE
            if (d->d_type == DT_DIR)
                continue;
#endif

            static const char wantedSuffix[] = ".conf";
            size_t fnamelen = strlen(d->d_name);
            if (fnamelen < sizeof(wantedSuffix))
                continue;
            if (memcmp(d->d_name + fnamelen + 1 - sizeof(wantedSuffix), wantedSuffix, sizeof wantedSuffix - 1) != 0)
                continue;

            if (seenNames.find(d->d_name) != seenNames.end())
                continue;

            seenNames.insert(d->d_name);
            sdk.name = d->d_name;
            sdk.name.resize(fnamelen + 1 - sizeof wantedSuffix);
            sdk.configFile = path + PATH_SEP + d->d_name;
            if ((this->*visit)(targetSdk, sdk))
                return sdk;
        }

        closedir(dir);
    }
    return Sdk();
}

Sdk ToolWrapper::selectSdk(const string &targetSdk)
{
    Sdk matchedSdk = iterateSdks(targetSdk, &ToolWrapper::matchSdk);
    if (!matchedSdk.isValid()) {
        fprintf(stderr, "%s: could not find a Qt installation of '%s'\n", argv0, targetSdk.c_str());
    }
    return matchedSdk;
}

bool ToolWrapper::printSdk(const string &, Sdk &sdk)
{
    printf("%s\n", sdk.name.c_str());
    return false; // continue
}

bool ToolWrapper::matchSdk(const string &targetSdk, Sdk &sdk)
{
    if (targetSdk == sdk.name || (targetSdk.empty() && sdk.name == "default")) {
        FILE *f = fopen(sdk.configFile.c_str(), "r");
        if (!f) {
            fprintf(stderr, "%s: could not open config file '%s': %s\n",
                    argv0, sdk.configFile.c_str(), strerror(errno));
            exit(1);
        }

        // read the first two lines.
        // 1) the first line contains the path to the Qt tools like qmake
        // 2) the second line contains the path to the Qt libraries
        // further lines are reserved for future enhancement
        char buf[PATH_MAX];
        if (!fgets(buf, PATH_MAX - 1, f)) {
            fclose(f);
            return false;
        }
        sdk.toolsPath = buf;
        sdk.toolsPath.erase(sdk.toolsPath.size() - 1); // drop newline

        if (!fgets(buf, PATH_MAX - 1, f)) {
            fclose(f);
            return false;
        }
        sdk.librariesPath = buf;
        sdk.librariesPath.erase(sdk.librariesPath.size() - 1); // drop newline

        fclose(f);
        return true;
    }

    return false;
}

static inline bool beginsWith(const char *haystack, const char *needle)
{
    return strncmp(haystack, needle, strlen(needle)) == 0;
}

int main(int argc, char **argv)
{
    // search the environment for defaults
    Mode operatingMode = RunTool;
    argv0 = basename(argv[0]);
    const char *targetSdk = getenv("QT_SELECT");
    const char *targetTool = getenv("QTCHOOSER_RUNTOOL");

#ifdef QTCHOOSER_TEST_MODE
    testMode = atoi(getenv("QTCHOOSER_TESTMODE")) > 0;
#endif

    // if the target tool wasn't set in the environment, use argv[0]
    if (!targetTool || !*targetTool)
        targetTool = argv0;

    // check the arguments to see if there's an override
    int optind = 1;
    for ( ; optind < argc; ++optind) {
        char *arg = argv[optind];
        if (*arg == '-') {
            ++arg;
            if (*arg == '-')
                ++arg;
            if (!*arg) {
                // -- argument, ends our processing
                // but skip this dash-dash argument
                ++optind;
                break;
            } else if (beginsWith(arg, "qt")) {
                // -qtX or -qt=X argument
                arg += 2;
                targetSdk = *arg == '=' ? arg + 1 : arg;
            } else if (beginsWith(arg, "run-tool=")) {
                // -run-tool= argument
                targetTool = arg + strlen("run-tool=");
                operatingMode = RunTool;
            } else if (strcmp(arg, "list-versions") == 0) {
                operatingMode = ListVersions;
            } else if (beginsWith(arg, "print-env")) {
                operatingMode = PrintEnvironment;
            } else {
                // not one of our arguments, must be for the target tool
                break;
            }
        } else {
            // not one of our arguments, must be for the target tool
            break;
        }
    }

    if (!targetSdk)
        targetSdk = "";

    ToolWrapper wrapper;

    // dispatch
    switch (operatingMode) {
    case RunTool:
        return wrapper.runTool(targetSdk,
                               targetTool,
                               argv + optind - 1);

    case PrintEnvironment:
        return wrapper.printEnvironment(targetSdk);

    case ListVersions:
        return wrapper.listVersions();
    }
}
