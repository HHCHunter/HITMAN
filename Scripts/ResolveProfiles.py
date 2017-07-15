import json
import urllib.request

JD = json.JSONDecoder()
JE = json.JSONEncoder()

PROFILE_FILE = "resolved_profiles.json"

profiles = {}

# Assuming this follows swixels old code for tokens.
with open("token.json", "r") as f:
	data = f.read()
	token = JD.decode(data)

# This gets manipulated somehow
profileIDs =  ["f35e43c2-0756-4501-a0ce-2bfc9f6bb9fe", "8ca61d83-b6ef-4af5-bc8b-e72e7069fa5e"]

# Body
body = JE.encode({"profileIDs": profileIDs})

path = "https://pc-service.hitman.io/authentication/api/userchannel/ProfileService/ResolveProfiles"

req = urllib.Request(
	url = path,
	data = body
)

req.add_header('Authorization', 'bearer %s' % token["access_token"])
req.add_header('Connection', 'Keep-Alive')
req.add_header('Content-Type', 'application/json')
req.add_header('Cache-Control', 'no-cache')
req.add_header('Content-Length', '%d' % len(body))
req.add_header('Content-Type','application/json')
req.add_header('Pragma','no-cache')
req.add_header('Accept','application/json')
req.add_header('Content-Type', 'application/json')
req.add_header('User-Agent','G2 Http/1.0')
req.add_header('Version','6.70.0')
req.add_data(body)

r = urllib.request.urlopen()(req,
	timeout=5
)

rv = r.read()

# For now we dump it here, but you should use a database and iterate the response.
with open(PROFILE_FILE, "w") as f:
f.write(rv)