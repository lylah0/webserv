#!/usr/bin/env python3
import sys, os

body = sys.stdin.read()

print("Content-Type: text/plain\r\n")
print("=== POST BODY ===")
print(body)
