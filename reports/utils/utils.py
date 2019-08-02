#! /usr/bin/env python
# -*- coding: utf-8 -*-

def find_domain_file(filenames, dir='.'):
    for filename in filenames:
        path = os.path.join(dir, filename)
        if os.path.exists(path):
            return path
    raise IOError('none found in %r: %r' % (dir, filenames))
