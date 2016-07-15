# -*- mode: sh -*-
## Copyright (C) 2016 The Qt Company Ltd.
## Contact: http://www.qt-project.org/legal
##
## This file is part of the qtchooser module of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:BSD$
## You may use this file under the terms of the BSD license as follows:
##
## "Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are
## met:
##   * Redistributions of source code must retain the above copyright
##     notice, this list of conditions and the following disclaimer.
##   * Redistributions in binary form must reproduce the above copyright
##     notice, this list of conditions and the following disclaimer in
##     the documentation and/or other materials provided with the
##     distribution.
##   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
##     of its contributors may be used to endorse or promote products derived
##     from this software without specific prior written permission.
##
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
## OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
## LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
##
## $QT_END_LICENSE$
##


function _qt_env_removefrom
    test (count $argv) -eq 2; or return
    set -l INDEX (eval contains -i -- $argv[2] $$argv[1])
    if test $INDEX
        set -e $argv[1][$INDEX]
    end
end

function _qt_select
    # Get or set the Qt version
    if test 0 -eq (count $argv)
        if test -z $QT_SELECT
            echo "Not using Qt."
        else
            echo "Using Qt version: $QT_SELECT"
        end
    else
        # Set the working Qt version
        set -e QT_SELECT
        set -l QTTOOLDIR
        set -l QTCHOOSER_PRINT_ENV qtchooser -qt=$argv[1] -print-env
        set -l QT_ENV (test x$argv[1] = xnone; or eval $QTCHOOSER_PRINT_ENV)

        # Remove old
        _qt_env_removefrom LD_LIBRARY_PATH $QTLIBDIR
        _qt_env_removefrom PKG_CONFIG_PATH $QTLIBDIR/pkgconfig
        _qt_env_removefrom CMAKE_PREFIX_PATH $QTDIR

        if test "x$argv[1]" != "xnone"
            for QT_ENV_VAR in $QT_ENV
                eval set -gx (echo $QT_ENV_VAR | sed -e "s,=, ,")
            end

            # Add new
            set -gx LD_LIBRARY_PATH $LD_LIBRARY_PATH $QTLIBDIR
            set -gx PKG_CONFIG_PATH $PKG_CONFIG_PATH $QTLIBDIR/pkgconfig

            echo "Using Qt version: $argv[1]"

            # try to get the QTDIR from qmake now
            set -gx QTDIR (qmake -query QT_INSTALL_PREFIX)

            set -gx CMAKE_PREFIX_PATH $CMAKE_PREFIX_PATH $QTDIR

            # is this an uninstalled Qt build dir?
            if test -f $QTDIR/Makefile
                set -gx QTSRCDIR (dirname (awk '/Project:/{print $NF}' $QTDIR/Makefile))
            else
                set -e QTSRCDIR
            end
        else
            set -e QTLIBDIR
            set -e QTSRCDIR
            set -e QTDIR
            set -e QT_SELECT

            if qtchooser -print-env >/dev/null 2>&1
                echo "Using default Qt version."
            else
                echo "Not using Qt."
            end
        end
    end
end

function qt
    _qt_select $argv
end

complete -x -c qt -a '(qtchooser -list-versions)' -d "Select a Qt version"

function qcd
    if test -z $QTDIR
        echo "No Qt version selected."
        return 1
    end

    if test (count $argv) -ge 1
        cd $QTDIR/$argv[1]
    else
        # switch between src and bld dir (not changing sub dir)
        if string match -q "$QTDIR*" (pwd)
            set -l subdir (string replace "$QTDIR" "" (pwd))
            cd $QTSRCDIR/$subdir
        else if string match -q "$QTSRCDIR*" (pwd)
            set -l subdir (string replace "$QTSRCDIR" "" (pwd))
            cd $QTDIR/$subdir
        else
            cd $QTDIR
        end
    end
end

function __qt_qcd_paths
    set -l token (commandline -ct)
    cd $QTDIR
    for d in $token*/
        echo $d
    end
end

complete -x -c qcd -a "(__qt_qcd_paths)"
