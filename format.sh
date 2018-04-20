set -o nounset
set -o pipefail

thisfile="${BASH_SOURCE[0]}"
thisdir="$( cd "$( dirname "${thisfile}" )" && pwd )"

yapf -ir -vv --no-local-style ${thisdir}/

astyle --style=kr --indent-modifiers --indent-switches --pad-oper --add-braces --preserve-date --recursive --suffix=none ${thisdir}/"*.h, *.cc"
