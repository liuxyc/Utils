import sys
import time
dict_data = dict()
for x in xrange(4000):
    dict_data[x] = "*" * 1024 * 1024
    for i in xrange(10000):
        a = i
