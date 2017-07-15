#This script lets you download things from the blob.  It has its own expiry.

import json
import urllib.request
JD = json.JSONDecoder()
JE = json.JSONEncoder()

with open("token.json", "r") as f:
    data = f.read()
    token = JD.decode(data)

gData = {}

path = "https://pc-service.hitman.io/authentication/api/configuration/Init?configName=pc-prod&lockedContentDisabled=false"
req = urllib.request(
    url=path
)
req.add_header('Authorization', 'bearer %s' % token["access_token"])
req.add_header('Cache-Control', 'no-cache')
req.add_header('Pragma','no-cache')
req.add_header('Accept','application/json')
req.add_header('Content-Type', 'application/json')
req.add_header('User-Agent','G2 Http/1.0')
req.add_header('Version','6.70.0')

r = urllib.request(req,
    timeout=5
)

rv = r.read()

with open("blobToken.json", "w") as f:
    f.write(rv)