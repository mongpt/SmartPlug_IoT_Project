import requests
import os
import json
import hashlib

url = ('http://10.72.131.86:5000/grn?segNum=0&segSize=10240')


##This redirects over HTTP so isn't actually a HTTP request

data = {
    'segSize': 500, 
    'segNum': 5
}


for i in range(0, 8):
    data['segNum'] = i
    print("############################################")
    print("Segment %d"%i)


    x = requests.get(url, params=data)
    
    if (not x.ok):
        print("HTTP ERROR Code %d"%x.status_code)
    else:
        md = hashlib.md5(x.content)
        print("MD5 %s"%(md.hexdigest()))
        print("Length %d"%len(x.content))
        print(x.content)