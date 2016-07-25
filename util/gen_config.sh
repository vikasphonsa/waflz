#!/bin/sh
cat > .test.h <<'EOM'
#include <gperftools/profiler.h>
EOM

rm -f _config.h
if gcc -Werror .test.h
then
    echo '#include <gperftools/profiler.h>' >> _config.h
    echo '#include <gperftools/heap-profiler.h>' >> _config.h
else
    echo '#include <google/profiler.h>' >> _config.h
    echo '#include <google/heap-profiler.h>' >> _config.h
fi
