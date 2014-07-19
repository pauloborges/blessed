#!/usr/bin/env python2
# encoding: utf-8

from __future__ import print_function

import requests
import json
import sys

if len(sys.argv) < 3:
	print('Invalid number of arguments', file=sys.stderr)
	print('Usage: %s SERVER /path/to/file.tar.gz' % sys.argv[0],
		file=sys.stderr)
	sys.exit(1)

server = sys.argv[1]
code = open(sys.argv[2], 'rb')

response = requests.post(server, timeout=60, files={ 'code': code })

if response.status_code != 200:
	print(response.text, file=sys.stderr)
	sys.exit(1)

data = response.json()

print(data['message'], file=sys.stderr if data['status'] else sys.stdout)

sys.exit(data['status'])
