#Token Script

import json
import urllib2

with open("token.json", "r") as f:
    data = f.read()
    JD = json.JSONDecoder()
    token = JD.decode(data)

body = "grant_type=refresh_token&refresh_token=%s" % token["refresh_token"]

req = urllib2.Request(
    url="https://auth.hitman.io/oauth/token"
)
req.add_header('Authorization', 'bearer %s' % token["access_token"])
req.add_header('Cache-Control', 'no-cache')
req.add_header('Content-Length', '%d' % len(body))
req.add_header('Content-Type','application/json')
req.add_header('Pragma','no-cache')
req.add_header('Accept','application/json')
req.add_header('Content-Type', 'application/json')
req.add_header('User-Agent','G2 Http/1.0')
req.add_header('Version','6.61.0')
req.add_data(body)

r = urllib2.urlopen(req,
    timeout=5
)

rv = r.read()

with open("token.json", "w") as f:
    f.write(rv)
