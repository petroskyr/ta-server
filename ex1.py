#!/usr/bin/env python3

import sys

print ("***Here is a python3.x example.")

if 1 >= len(sys.argv):
    print("(no args provided)")
else:
    for i in range(1,len(sys.argv)):
        print("argv[%d] = %s" % (i, sys.argv[i]))
    
s = input ("What is your name? ")
print ("WELL, " + s + " you have a lovely name")
