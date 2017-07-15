#This script should download leaderboard entries for contract ID's

import json
import urllib.request
JD = json.JSONDecoder()
JE = json.JSONEncoder()

with open("token.json", "r") as f:
    data = f.read()
    token = JD.decode(data)
	
#Need to figure out how to make the contract ID change-able?	
path = "https://pc-service.hitman.io/profiles/page//leaderboardentries?contractid=[Contract ID]&page=0 HTTP/1.1"
req = urllib.request(
    url=path
)
req.add_header('Authorization', 'bearer %s' % token["access_token"])
req.add_header('Connection', 'Keep-Alive')
req.add_header('Cache-Control', 'no-cache')
req.add_header('Content-Length', '%d' % len(body))
req.add_header('Content-Type','application/json')
req.add_header('Pragma','no-cache')
req.add_header('Accept','application/json')
req.add_header('Content-Type', 'application/json')
req.add_header('User-Agent','G2 Http/1.0')
req.add_header('Version','6.70.0')
req.add_data(body)

r = urllib.request (req,
    timeout=5
)

rv = r.read()

with open("ContractID-Leaderboard.json", "w") as f:
    f.write(rv)