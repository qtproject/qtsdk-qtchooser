# -*- mode: sh -*-
## Copyright (C) 2013 Intel Corportation
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


source ${BASH_SOURCE%/*}/common.sh

function qt_env_removefrom()
{
    local entry
    local -a contents
    local IFS=' '
    export IFS

    # Split on ':'
    for entry in `IFS=: eval echo \\\$$1`; do
        test "$entry" = "$2" || contents=("${contents[@]}" $entry)
    done

    # Set it again
    IFS=:
    eval "$1=\"\${contents[*]}\""
}

# completion:
function _qt()
{
    COMPREPLY=()
    if [ ${#COMP_WORDS[@]} -eq 2 ] && [ $COMP_CWORD -eq 1 ]; then
        local cur opts
        cur="${COMP_WORDS[COMP_CWORD]}"
        opts=`qtchooser -list-versions`
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
    fi
}
complete -F _qt qt
