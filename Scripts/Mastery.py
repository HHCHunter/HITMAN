#This script gets mastery information.

import json
import urllib2
JD = json.JSONDecoder()
JE = json.JSONEncoder()

with open("token.json", "r") as f:
    data = f.read()
    token = JD.decode(data)

locs = [
    "LOCATION_BANGKOK_ZIKA",
    "LOCATION_BANGKOK",
    "LOCATION_COASTALTOWN_EBOLA",
    "LOCATION_COASTALTOWN_MAMBA",
    "LOCATION_COASTALTOWN_MOVIESET",
    "LOCATION_COASTALTOWN_NIGHT",
    "LOCATION_COASTALTOWN",
    "LOCATION_COLORADO_RABIES",
    "LOCATION_COLORADO",
    "LOCATION_HOKKAIDO_FLU",
    "LOCATION_HOKKAIDO",
    "LOCATION_ICA_FACILITY",
    "LOCATION_MARRAKECH_NIGHT",
    "LOCATION_MARRAKECH",
    "LOCATION_PARENT_BANGKOK",
    "LOCATION_PARENT_COASTALTOWN",
    "LOCATION_PARENT_COLORADO",
    "LOCATION_PARENT_HOKKAIDO",
    "LOCATION_PARENT_MARRAKECH",
    "LOCATION_PARENT_PARIS",
    "LOCATION_PARIS",
    "LOCATION_SIBERIA",
]

gData = {}

def doLocation(loc):
    path = "https://pc-service.hitman.io/profiles/page/MasteryLocation?locationId=%s" % loc
    req = urllib2.Request(
        url=path
    )
    req.add_header('Authorization', 'bearer %s' % token["access_token"])
    req.add_header('Cache-Control', 'no-cache')
    req.add_header('Pragma','no-cache')
    req.add_header('Accept','application/json')
    req.add_header('Content-Type', 'application/json')
    req.add_header('User-Agent','G2 Http/1.0')
    req.add_header('Version','6.70.0')

    r = urllib2.urlopen(req,
        timeout=5
    )

    rv = r.read()

    try:
        data = JD.decode(rv)
        if "data" in data:
            gData[loc] = data["data"]
    except:
        pass
    with open("mastery/%s.json" % loc, "w") as f:
        f.write(rv)

for loc in locs:
    doLocation(loc)

o = JE.encode(gData)

with open("got_mastery.json", "w") as f:
    f.write(o)

print(o)
