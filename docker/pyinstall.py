from __future__ import print_function

import argparse
import os, os.path
import stat
from sys import exit


def get_host_work_dir():
    hosthomedir = os.environ['HOME']
    hostworkdir = os.path.join(hosthomedir, 'work')
    #if not os.path.isdir(hostworkdir):
    #    hostworkdir = hosthomedir
    assert os.path.isdir(hostworkdir)
    return hostworkdir


def get_image(imgname, imgversion):
    if imgname is None or imgversion is None:
        imgdir = os.getcwd()
        if imgname is None:
            imgname = os.path.join(imgdir, 'name')
            if not os.path.isfile(imgname):
                imgname = os.path.join(os.path.dirname(imgdir), 'name')
                if not os.path.isfile(imgname):
                    exit()
            imgname = open(imgname).read().strip('\n')
            envname = os.path.basename(imgdir)
        else:
            envname = imgname
        if imgversion is None:
            imgversion = os.path.join(imgdir, 'version')
            if not os.path.isfile(imgversion):
                imgversion = os.path.join(os.path.dirname(imgdir), 'version')
                if not os.path.isfile(imgversion):
                    exit()
            imgversion = open(imgversion).read().strip('\n')
    else:
        envname = imgname
    return imgname, imgversion, envname


def maketext():
    if asroot:
        dockeruser = 'root'
        args = '-u root'
        dockerhomedir = '/root'
    else:
        if os.uname()[0] == 'Linux' and os.getuid() != 1000:
            dockeruser = os.getuid()
            args = """-u {hostuser}:docker \\
        -v /etc/group:/etc/group:ro \\
        -v /etc/passwd:/etc/passwd:ro""".format(hostuser=os.getuid())
        else:
            dockeruser = 'docker-user'
            args = "-u docker-user"

        dockerhomedir = '/home/docker-user'

    if options:
        args += ' ' + options

    dockerworkdir = os.path.join(dockerhomedir, os.path.basename(hostworkdir))
    pypaths = ':'.join([os.path.join(dockerworkdir, p) for p in pypath])
    if pypath_abs:
        pypaths += ':' + ':'.join(pypath_abs)

    print('installing', cmd, 'into', bindir)

    text = """\
#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

launchdir=$(pwd)
if [[ "${{launchdir}}" == "{hostworkdir}"* ]]; then
    workdir="{dockerworkdir}""${{launchdir#{hostworkdir}}}"
else
    workdir="{dockerworkdir}"
fi

ARGS="\\
    {args} \\
    -e USER={dockeruser} \\
    -e HOME={dockerhomedir} \\
    -v "{hostworkdir}":"{dockerworkdir}" \\
    -e CFGDIR="{dockerworkdir}/config" \\
    -e LOGDIR="{dockerworkdir}/log" \\
    -e DATADIR="{dockerworkdir}/data" \\
    -e TMPDIR="{dockerworkdir}/tmp" \\
    -e IMAGE_NAME={envname} \\
    -e IMAGE_VERSION={imgversion} \\
    -e PYTHONPATH={pypaths} \\
    --rm -it \\
    -w "${{workdir}}" \\
    -e TZ=America/Los_Angeles"

if (( $# > 0 )); then
    if [[ "$1" == "notebook" ]]; then
        ARGS="${{ARGS}} \\
    --expose=8888 \\
    -p 8888:8888"
        workdir="{dockerworkdir}"
        shift
        command="jupyter notebook --port=8888 --no-browser --ip=0.0.0.0 --NotebookApp.notebook_dir='${{workdir}}' --NotebookApp.token='' $@"
    else
        command="$@"
    fi
else
    command="{defaultcmd}"
fi

docker run ${{ARGS}} {imgname}:{imgversion} ${{command}}
""".format(
        hostworkdir=hostworkdir,
        dockerworkdir=dockerworkdir,
        args=args,
        dockeruser=dockeruser,
        dockerhomedir=dockerhomedir,
        envname=envname,
        imgversion=imgversion,
        pypaths=pypaths,
        imgname=imgname,
        defaultcmd=defaultcmd,
    )

    return text


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--cmd', required=True, help='name of the generated script')
    p.add_argument(
        '--dockercmd',
        help='default command to launch in the Docker container',
        default='/bin/bash',
    )
    p.add_argument(
        '--asroot',
        action='store_true',
    )
    p.add_argument(
        '--pypath',
        action='append',
        default=[],
        help=
        'add the specified directory to PYTHONPATH; the directory starts below `hostworkdir`',
    )
    p.add_argument(
        '--pypath_abs',
        action='append',
        default=[],
        help='add the specified absolute directory to PYTHONPATH',
    )
    p.add_argument(
        '--imgname',
        help='name of Docker image',
    )
    p.add_argument(
        '--imgversion',
        help='version of Docker image',
    )
    p.add_argument(
        '--options',
        help=
        'additional arguments, as a string, passed on to "docker run" verbatim',
        default='',
    )
    args = p.parse_args()

    imgname = args.imgname
    imgversion = args.imgversion
    imgname, imgversion, envname = get_image(imgname, imgversion)

    cmd = args.cmd
    defaultcmd = args.dockercmd
    pypath = args.pypath
    pypath_abs = args.pypath_abs
    asroot = args.asroot
    options = args.options

    hostworkdir = get_host_work_dir()
    bindir = os.path.join(hostworkdir, 'bin')
    if not os.path.isdir(bindir):
        os.mkdir(bindir)

    target = os.path.join(bindir, cmd)

    text = maketext()
    open(target, 'w').write(text)
    os.chmod(target, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP
             | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)
