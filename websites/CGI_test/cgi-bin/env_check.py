#!/usr/bin/env python3
import os
print("Content-Type: text/plain\r\n")
print(os.environ["REQUEST_METHOD"])
print(os.environ["QUERY_STRING"])
print(os.environ["CONTENT_LENGTH"])
print(os.environ["SCRIPT_FILENAME"])
print(os.environ["SCRIPT_NAME"])
print(os.environ["SERVER_PROTOCOL"])
