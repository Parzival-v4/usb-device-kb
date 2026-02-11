#!/bin/sh

find . -name "*.cmd"|xargs -i rm -rf {}
find . -name "*.o"|xargs -i rm -rf {}
find . -name "*.o.cmd"|xargs -i rm -rf {}
find . -name "*.ko"|xargs -i rm -rf {}
find . -name "*.ko.cmd"|xargs -i rm -rf {}
find . -name "*.mod"|xargs -i rm -rf {}
find . -name "*.mod.c"|xargs -i rm -rf {}
find . -name "*.mod.cmd"|xargs -i rm -rf {}
find . -name "*.symvers"|xargs -i rm -rf {}
find . -name "*.o modules.*"|xargs -i rm -rf {}
find . -name "*modules.order"|xargs -i rm -rf {}
find . -name "*.tmp_versions"|xargs -i rm -rf {}
#find . -name "*.bak"|xargs -i rm -rf {}