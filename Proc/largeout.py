import sys

print "a" * 64 * 1024 * 1024
print "b" * 32

print >>sys.stderr, "c" * 10
