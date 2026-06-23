#!/usr/bin/env python3
import time
print("Content-Type: text/plain\r\n")
for i in range(10):
    print(f"Line {i}")
    time.sleep(1)
